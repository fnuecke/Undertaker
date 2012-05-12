#include "unit_ai.h"
#include "job_script.h"

#include <assert.h>
#include <float.h>
#include <math.h>
#include <stdlib.h>

#include "astar_mp.h"
#include "block.h"
#include "job.h"
#include "map.h"
#include "meta_job.h"
#include "unit.h"

///////////////////////////////////////////////////////////////////////////////
// Utility methods
///////////////////////////////////////////////////////////////////////////////

/** Computes additional job weight based on saturation */
static float weightedPreference(float saturation, float preference, const MP_UnitJobSaturationMeta* meta) {
    // Map saturation to interval between unsatisfied and satisfied thresh so
    // that unsatisfiedThreshold = 1 and satisfiedThreshold = 0.
    const float delta = meta->satisfiedThreshold - meta->unsatisfiedThreshold;
    if (delta > 0.0f && preference > 0.0f) {
        return preference * (meta->satisfiedThreshold - saturation) / delta;
    }

    // Handle preference <= 0 as a special case where it's only considered
    // if there's nothing else to do. We do this by adding half of the max
    // value for floats, because we can probably pretty safely assume that
    // such distances will never occur in 'normal' cases. This allows for
    // proper relative distance comparison for all preference <= 0 jobs.
    return preference - FLT_MAX / 2;
}

/** Catmull-rom interpolation */
inline static float cr(float p0, float p1, float p2, float p3, float t) {
    const float m1 = MP_AI_CATMULL_ROM_T * (p2 - p0);
    const float m2 = MP_AI_CATMULL_ROM_T * (p3 - p1);
    return (2 * p1 - 2 * p2 + m1 + m2) * t * t * t +
            (-3 * p1 + 3 * p2 - 2 * m1 - m2) * t * t +
            m1 * t +
            p1;
}

/** Moves a unit along its current path */
static void updateMove(MP_Unit* unit) {
    AI_Path* path = &unit->ai->pathing;

    // Are we even moving?
    if (!MP_IsUnitMoving(unit)) {
        return;
    }

    // AI is moving. Apply movement.
    path->traveled += unit->meta->moveSpeed / MP_FRAMERATE;

    // Test if we reached the way point. Do this in a loop to allow skipping
    // way points when moving really fast.
    while (path->traveled > path->distance) {
        // Yes, try to advance to the next one.
        ++path->index;
        if (!MP_IsUnitMoving(unit)) {
            // Reached final node, we're done.
            unit->position.d.x = path->nodes[path->depth].d.x;
            unit->position.d.y = path->nodes[path->depth].d.y;
            return;
        } else {
            // Subtract length of previous to carry surplus movement.
            path->traveled -= path->distance;

            // Do a direct check for distance, to allow skipping equal
            // nodes.
            {
                const float dx = path->nodes[path->index].d.x -
                        path->nodes[path->index - 1].d.x;
                const float dy = path->nodes[path->index].d.y -
                        path->nodes[path->index - 1].d.y;
                path->distance = sqrtf(dx * dx + dy * dy);
            }
            // If there is a distance, estimate the actual path length.
            if (path->distance > 0 && MP_AI_PATH_INTERPOLATE) {
                int e;
                float x, y, dx, dy;
                float lx = path->nodes[path->index - 1].d.x;
                float ly = path->nodes[path->index - 1].d.y;
                path->distance = 0;
                for (e = 1; e <= MP_AI_PATH_INTERPOLATION; ++e) {
                    const float t = e / (float) MP_AI_PATH_INTERPOLATION;
                    x = cr(path->nodes[path->index - 2].d.x,
                            path->nodes[path->index - 1].d.x,
                            path->nodes[path->index].d.x,
                            path->nodes[path->index + 1].d.x, t);
                    y = cr(path->nodes[path->index - 2].d.y,
                            path->nodes[path->index - 1].d.y,
                            path->nodes[path->index].d.y,
                            path->nodes[path->index + 1].d.y, t);
                    dx = x - lx;
                    dy = y - ly;
                    lx = x;
                    ly = y;
                    path->distance += sqrtf(dx * dx + dy * dy);
                }
            }
        }
    }

    // Compute actual position of the unit.
    if (path->distance > 0) {
        const float t = path->traveled / path->distance;
        unit->position.d.x = cr(
                path->nodes[path->index - 2].d.x,
                path->nodes[path->index - 1].d.x,
                path->nodes[path->index].d.x,
                path->nodes[path->index + 1].d.x, t);
        unit->position.d.y = cr(
                path->nodes[path->index - 2].d.y,
                path->nodes[path->index - 1].d.y,
                path->nodes[path->index].d.y,
                path->nodes[path->index + 1].d.y, t);
    }
}

/** Updates unit desire saturation based on currently performed job and such */
static void updateSaturation(MP_Unit* unit) {
    const AI_State* state = &unit->ai->state;
    const MP_JobMeta* activeJob = state->active ? state->job->meta : NULL;
    const MP_UnitJobSaturationMeta* metaSaturation = unit->meta->satisfaction.jobSaturation;
    float* saturation = unit->satisfaction.jobSaturation;

    // Update job saturation values.
    for (int number = unit->meta->jobCount - 1; number >= 0; --number) {
        // Yes, check it's currently performing it.
        if (state->active && activeJob == unit->meta->jobs[number]) {
            // Yes, it's active, update for the respective delta.
            saturation[number] += metaSaturation[number].performingDelta;
        } else {
            // Inactive, update for the respective delta.
            saturation[number] += metaSaturation[number].notPerformingDelta;
        }

        // Make sure it's in bounds.
        if (saturation[number] < 0) {
            saturation[number] = 0;
        } else if (saturation[number] > 1.0f) {
            saturation[number] = 1.0f;
        }
    }
}

/** Looks for the most desirable job for the unit */
static void updateCurrentJob(MP_Unit* unit) {
    AI_State* state = &unit->ai->state;
    const MP_JobMeta** jobs;
    const MP_UnitJobSaturationMeta* metaSaturation;
    const float* saturation;

    MP_Job* bestJob;
    float bestWeightedDistance;

    // Make sure our job still has a run method.
    if (state->job && !state->job->meta->hasRunMethod) {
        MP_StopJob(state->job);
    }

    // Skip if we already have a job.
    if (state->jobSearchDelay > 0) {
        --state->jobSearchDelay;
        return;
    }

    // Initialize.
    jobs = unit->meta->jobs;
    metaSaturation = unit->meta->satisfaction.jobSaturation;
    saturation = unit->satisfaction.jobSaturation;

    bestJob = NULL;
    bestWeightedDistance = FLT_MAX;

    // Find the closest job, weighted based on the unit's preferences.
    for (int number = unit->meta->jobCount - 1; number >= 0; --number) {
        float distance;
        MP_Job* job;

        // Skip jobs that cannot be executed.
        if (!jobs[number]->hasRunMethod) {
            continue;
        }

        // Find closest job opening.
        if (!(job = MP_FindJob(unit, jobs[number], &distance))) {
            // Didn't find a job of this type.
            continue;
        }

        // Weigh the distance based on the saturation and preference.
        if (jobs[number]->hasDynamicPreference) {
            distance -= weightedPreference(saturation[number],
                    MP_Lua_GetDynamicPreference(unit, jobs[number]),
                    &metaSaturation[number]);
        } else {
            distance -= weightedPreference(saturation[number],
                    metaSaturation[number].preference,
                    &metaSaturation[number]);
        }

        // Better than other jobs we have found?
        if (distance < bestWeightedDistance) {
            bestJob = job;
            bestWeightedDistance = distance;
        }
    }

    // Check if we found something (else) to do.
    if (bestJob && bestJob != state->job) {
        // Fire previous workers.
        MP_StopJob(bestJob);

        // Make self stop the active job (if any).
        MP_StopJob(state->job);

        // Pursue that job now.
        state->active = false;
        state->job = bestJob;
        state->jobRunDelay = 0;

        // We found work, reserve it for ourself.
        bestJob->worker = unit;
    }

    // Wait a bit before looking for a new job again.
    state->jobSearchDelay = MP_FRAMERATE;
}

/** Runs job logic, if possible */
static void updateJob(MP_Unit* unit) {
    AI_State* state = &unit->ai->state;
    // Only if we have a job.
    if (state->job) {
        // See if we have an update method for it or have to wait before running the
        // job logic again.
        if (state->jobRunDelay > 0) {
            // Wait some more.
            --state->jobRunDelay;
        } else {
            // Otherwise update the unit based on its current job.
            assert(state->job->meta->hasRunMethod);
            state->active = MP_Lua_RunJob(unit, state->job, &state->jobRunDelay);
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
// Header implementation
///////////////////////////////////////////////////////////////////////////////

float MP_MoveTo(const MP_Unit* unit, const vec2* position) {
    AI_Path* pathing = &unit->ai->pathing;

    // Find a path to it. Use temporary output data to avoid
    // overriding existing path that may be shorter.
    unsigned int depth = MP_AI_PATH_DEPTH;
    float distance = 0;
    if (MP_AStar(unit, position, &pathing->nodes[1], &depth, &distance)) {
        pathing->depth = depth;
        pathing->index = 1;
        pathing->distance = 0;
        pathing->traveled = 0;

        // Generate endpoints for catmull-rom spline; just
        // extend the path in the direction of the last two
        // nodes before that end.
        {
            const float dlx = pathing->nodes[1].d.x - pathing->nodes[2].d.x;
            const float dly = pathing->nodes[1].d.y - pathing->nodes[2].d.y;
            const float l = sqrtf(dlx * dlx + dly * dly);
            pathing->nodes[0].d.x = pathing->nodes[1].d.x;
            pathing->nodes[0].d.y = pathing->nodes[1].d.y;
            if (l > 0) {
                pathing->nodes[0].d.x += dlx / l;
                pathing->nodes[0].d.y += dly / l;
            }
        }
        {
            const float dlx = pathing->nodes[depth].d.x - pathing->nodes[depth - 1].d.x;
            const float dly = pathing->nodes[depth].d.y - pathing->nodes[depth - 1].d.y;
            const float l = sqrtf(dlx * dlx + dly * dly);
            pathing->nodes[depth + 1].d.x = pathing->nodes[depth].d.x;
            pathing->nodes[depth + 1].d.y = pathing->nodes[depth].d.y;
            if (l > 0) {
                pathing->nodes[depth + 1].d.x += dlx / l;
                pathing->nodes[depth + 1].d.y += dly / l;
            }
        }

        // Success.
        return distance / unit->meta->moveSpeed;
    }

    // Could not find a path.
    return -1.0f;
}

void MP_UpdateAI(MP_Unit* unit) {
    // Make the unit move. Units move independently of their current AI state.
    // This is to allow for units attacking while moving, or training while
    // wandering, etc.
    updateMove(unit);

    // Update a unit's job desire saturation values. This will also make the
    // unit lose its current job, if it's unsatisfied with another one (and
    // isn't with this one, yet).
    updateSaturation(unit);

    // Update the job we're currently doing. Get a job if we don't have one,
    // otherwise check if there's a better one.
    updateCurrentJob(unit);

    // Run job logic.
    updateJob(unit);
}
