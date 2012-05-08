#include "units_ai.h"

#include <assert.h>
#include <float.h>
#include <math.h>
#include <stdlib.h>

#include "block.h"
#include "jobs.h"
#include "map.h"
#include "units.h"
#include "jobs_meta.h"

///////////////////////////////////////////////////////////////////////////////
// Utility methods
///////////////////////////////////////////////////////////////////////////////

/** Catmull-rom interpolation */
static float cr(float p0, float p1, float p2, float p3, float t) {
    const float m1 = DK_AI_CATMULL_ROM_T * (p2 - p0);
    const float m2 = DK_AI_CATMULL_ROM_T * (p3 - p1);
    return (2 * p1 - 2 * p2 + m1 + m2) * t * t * t +
            (-3 * p1 + 3 * p2 - 2 * m1 - m2) * t * t +
            m1 * t +
            p1;
}

/** Moves a unit along its current path */
static void updateMove(DK_Unit* unit) {
    AI_Path* path = &unit->ai->pathing;

    // Are we even moving?
    if (path->index > path->depth) {
        return;
    }

    // AI is moving. Apply movement.
    path->traveled += unit->meta->moveSpeed;

    // Test if we reached the way point.
    if (path->traveled > path->distance) {
        // Yes, try to advance to the next one.
        ++path->index;
        if (path->index > path->depth) {
            // Reached final node, we're done.
            unit->position.d.x = path->nodes[path->index - 1].d.x;
            unit->position.d.y = path->nodes[path->index - 1].d.y;
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
            if (path->distance > 0 && DK_AI_PATH_INTERPOLATE) {
                int e;
                float x, y, dx, dy;
                float lx = path->nodes[path->index - 1].d.x;
                float ly = path->nodes[path->index - 1].d.y;
                path->distance = 0;
                for (e = 1; e <= DK_AI_PATH_INTERPOLATION; ++e) {
                    const float t = e / (float) DK_AI_PATH_INTERPOLATION;
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
static void updateSaturation(DK_Unit* unit) {
    const DK_UnitSatisfactionMeta* meta = &unit->meta->satisfaction;
    DK_UnitSaturation* state = &unit->saturation;
    const AI_State* ai = unit->ai->current;

    // Update job saturation values. Remember if there are any jobs we're not
    // satisfied with (except the one we're currently doing).
    bool someUnsatisfied = false;
    for (unsigned int number; number < unit->meta->jobCount; ++number) {
        // Yes, check it's currently performing it.
        if (ai->jobNumber == number) {
            // Yes, update for the respective delta.
            state->jobSaturation[number] +=
                    meta->jobSaturation[number].performingDelta;
        } else {
            // No, update for the respective delta.
            state->jobSaturation[number] +=
                    meta->jobSaturation[number].notPerformingDelta;

            // Are we unsatisfied with this job?
            if (state->jobSaturation[number] <
                    meta->jobSaturation[number].unsatisfiedThreshold) {
                someUnsatisfied = true;
            }
        }

        // Make sure it's in bounds.
        if (state->jobSaturation[number] < 0) {
            state->jobSaturation[number] = 0;
        } else if (state->jobSaturation[number] > 1.0f) {
            state->jobSaturation[number] = 1.0f;
        }
    }

    // TODO check if unit is held in hand, apply delta if so.

    // Check if we are not unsatisfied with our current job and there's another
    // one we are. If so, cancel the current job to allow starting the one we're
    // unsatisfied with.
    if (someUnsatisfied && state->jobSaturation[ai->jobNumber] >=
            meta->jobSaturation[ai->jobNumber].unsatisfiedThreshold) {
        // Indeed, so pop the current job.
        DK_StopJob(ai->job);
    }
}

static float getDynamicPreference(DK_Unit* unit, DK_Job* job) {
    lua_State* L = job->meta->L;
    // Try to get the preference callback.
    lua_getglobal(L, "preference");
    if (lua_isfunction(L, -1)) {
        // Call it.
        // TODO parameters; at least the unit this concerns.
        if (lua_pcall(L, 0, 1, 0) == LUA_OK) {
            // OK, try to get the result as a float.
            if (lua_isnumber(L, -1)) {
                float preference = lua_tonumber(L, -1);
                lua_pop(L, 1);
                return preference;
            } else {
                lua_pop(L, 1);
                fprintf(DK_log_target, "ERROR: Dynamic preference for '%s' returned something that's not a number.\n", job->meta->name);
            }
        } else {
            // Something went wrong.
            fprintf(DK_log_target, "ERROR: Something bad happened evaluating dynamic preference for '%s': %s\n", job->meta->name, lua_tostring(L, -1));
            lua_pop(L, 1);
        }
    } else {
        fprintf(DK_log_target, "ERROR: Dynamic preference function for '%s' isn't a function anymore.\n", job->meta->name);
    }

    // We get here only on failure. In that case disable the dynamic preference,
    // so we don't try this again.
    fprintf(DK_log_target, "INFO: Disabling dynamic preference for '%s'.\n", job->meta->name);
    DK_DisableDynamicPreference(job->meta);

    return 0;
}

/** Computes additional job weight based on saturation */
static float weightedPreference(float saturation, float preference, const DK_UnitJobSaturationMeta* meta) {
    // Map saturation to interval between unsatisfied and satisfied thresh so
    // that unsatisfiedThreshold = 1 and satisfiedThreshold = 0 and ensure it's
    // larger or equal to zero.
    saturation = (meta->satisfiedThreshold - saturation) /
            (meta->satisfiedThreshold - meta->unsatisfiedThreshold);
    if (saturation < 0.0f) {
        saturation = 0.0f;
    } // It's OK if it's larger than one, in that case the unit is unsatisfied.
    return preference * saturation;
}

/** Looks for the most desirable job for the unit */
static void findJob(DK_Unit* unit) {
    // Shortcuts.
    const DK_UnitSatisfactionMeta* meta = &unit->meta->satisfaction;
    const DK_UnitSaturation* state = &unit->saturation;

    // For closest job search.
    DK_Job* bestJob = 0;
    unsigned int bestNumber = 0;
    float bestWeightedDistance = FLT_MAX;

    // Do a quick scan to see if the unit has any unsatisfied job desires.
    bool someUnsatisfied = false;
    for (unsigned int number = 0; number < unit->meta->jobCount; ++number) {
        // Check if the unit is unsatisfied.
        if (state->jobSaturation[number] <
                meta->jobSaturation[number].unsatisfiedThreshold) {
            someUnsatisfied = true;
        }
    }

    // Find the closest job, weighted based on the unit's preferences.
    for (unsigned int number = 0; number < unit->meta->jobCount; ++number) {
        float distance;
        DK_Job* job;

        // Check if the unit is unsatisfied or doesn't have others unsatisfied.
        if (someUnsatisfied && state->jobSaturation[number] >=
                meta->jobSaturation[number].unsatisfiedThreshold) {
            // Unit isn't unsatisfied with this job, but some other, so skip it.
            continue;
        }

        // Find closest job opening.
        if (!(job = DK_FindJob(unit, unit->meta->jobs[number], &distance))) {
            // Didn't find a job of this type.
            continue;
        }

        // Weigh the distance based on the saturation and preference.
        if (job->meta->hasDynamicPreference) {
            distance -= weightedPreference(state->jobSaturation[number], getDynamicPreference(unit, job), &meta->jobSaturation[number]);
        } else {
            distance -= weightedPreference(state->jobSaturation[number], meta->jobSaturation[number].preference, &meta->jobSaturation[number]);
        }

        // Better than other jobs we have found?
        if (distance < bestWeightedDistance) {
            bestJob = job;
            bestNumber = number;
            bestWeightedDistance = distance;
        }
    }

    // Check if we found something to do.
    if (bestJob) {
        // Fire previous workers.
        DK_StopJob(bestJob);

        // Push that job onto the stack.
        {
            AI_State* ai = --unit->ai->current;
            ai->job = bestJob;
            ai->jobNumber = bestNumber;
            ai->delay = 0;
            ai->shouldCancel = false;
        }

        // We found work, reserve it for ourself.
        bestJob->worker = unit;
    }
}

/** Makes a unit discard its current movement and begin moving to the specified location */
static int moveTo(const DK_Unit* unit, const vec2* position) {
    AI_Path* pathing = &unit->ai->pathing;

    // Find a path to it. Use temporary output data to avoid
    // overriding existing path that may be shorter.
    unsigned int depth = DK_AI_PATH_DEPTH;
    if (DK_AStar(unit, position, &pathing->nodes[1], &depth, NULL)) {
        pathing->depth = depth;
        pathing->index = 0;
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
        return 1;
    }

    // Could not find a path.
    return 0;
}

///////////////////////////////////////////////////////////////////////////////
// Header implementation
///////////////////////////////////////////////////////////////////////////////

void DK_UpdateAI(DK_Unit* unit) {
    AI_State* state = unit->ai->current;

    // Make the unit move. Units move independently of their current AI state.
    // This is to allow for units attacking while moving, or training while
    // wandering, etc.
    updateMove(unit);

    // Update a unit's job desire saturation values. This will also make the
    // unit lose its current job, if it's unsatisfied with another one (and
    // isn't with this one, yet).
    updateSaturation(unit);

    // See if we should pop the current state.
    if (state < &unit->ai->stack[DK_AI_STACK_DEPTH] && state->shouldCancel) {
        ++unit->ai->current;
    }

    // Get a job if we don't have one.
    if (unit->ai->current >= &unit->ai->stack[DK_AI_STACK_DEPTH]) {
        findJob(unit);
        // Update state shortcut.
        state = unit->ai->current;
    }

    // See if we have an update method for it or have to wait before running the
    // job logic again.
    if (!unit->meta->jobs[state->jobNumber]->hasRunMethod) {
        // Pop it.
        ++unit->ai->current;
    } else if (state->delay) {
        // Wait some more.
        --state->delay;
    } else {
        // Otherwise update the unit based on its current job.
        DK_RunJob(unit, unit->meta->jobs[state->jobNumber]);
    }
}
