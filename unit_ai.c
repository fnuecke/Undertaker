#include "unit_ai.h"
#include "script.h"

#include <assert.h>
#include <float.h>
#include <math.h>
#include <stdlib.h>

#include "astar_mp.h"
#include "block.h"
#include "graphics.h"
#include "job.h"
#include "map.h"
#include "job_type.h"
#include "render.h"
#include "unit.h"

///////////////////////////////////////////////////////////////////////////////
// Utility methods
///////////////////////////////////////////////////////////////////////////////

/** Computes additional job weight based on saturation */
static float weightedPreference(float saturation, float preference, const MP_UnitJobType* type) {
    // Map saturation to interval between unsatisfied and satisfied thresh so
    // that unsatisfiedThreshold = 1 and satisfiedThreshold = 0.
    const float delta = type->satisfiedThreshold - type->unsatisfiedThreshold;
    if (delta > 0.0f && preference > 0.0f) {
        return preference * (type->satisfiedThreshold - saturation) / delta;
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
    if (!MP_IsUnitMoving(unit) || unit->ai->isInHand) {
        return;
    }

    // AI is moving. Apply movement.
    path->traveled += unit->type->moveSpeed / MP_FRAMERATE;

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
    const MP_JobType* activeJob = (state->active && state->job) ? state->job->type : NULL;
    const MP_UnitJobType* jobTypes = unit->type->jobs;
    float* saturation = unit->jobSaturation;

    // Update job saturation values.
    for (int number = unit->type->jobCount - 1; number >= 0; --number) {
        // Yes, check it's currently performing it.
        if (activeJob == unit->type->jobs[number].type) {
            // Yes, it's active, update for the respective delta.
            saturation[number] += jobTypes[number].performingDelta;
        } else {
            // Inactive, update for the respective delta.
            saturation[number] += jobTypes[number].notPerformingDelta;
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
    const MP_UnitJobType* jobTypes;
    const float* saturation;

    MP_Job* bestJob;
    float bestWeightedDistance;

    // Skip if the unit is in the player's hand.
    if (unit->ai->isInHand) {
        return;
    }

    // Make sure our job still has a run method.
    if (state->job && state->job->type->runMethod == LUA_REFNIL) {
        MP_StopJob(state->job);
    }

    // Skip if we already have a job.
    if (state->jobSearchDelay > 0) {
        --state->jobSearchDelay;
        return;
    }

    // Initialize.
    jobTypes = unit->type->jobs;
    saturation = unit->jobSaturation;

    bestJob = NULL;
    bestWeightedDistance = FLT_MAX;

    // Find the closest job, weighted based on the unit's preferences.
    for (int number = unit->type->jobCount - 1; number >= 0; --number) {
        float distance;
        MP_Job* job;

        // Skip jobs that cannot be executed.
        if (jobTypes[number].type->runMethod == LUA_REFNIL) {
            continue;
        }

        // Find closest job opening.
        if (!(job = MP_FindJob(unit, jobTypes[number].type, &distance))) {
            // Didn't find a job of this type.
            continue;
        }

        // Weigh the distance based on the saturation and preference.
        if (jobTypes[number].type->dynamicPreference != LUA_REFNIL) {
            distance -= weightedPreference(saturation[number], MP_GetDynamicPreference(unit, jobTypes[number].type), &jobTypes[number]);
        } else {
            distance -= weightedPreference(saturation[number], jobTypes[number].preference, &jobTypes[number]);
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
        state->jobRunDelay = 0;
        state->job = bestJob;
        state->active = false;

        // We found work, reserve it for ourself.
        bestJob->worker = unit;
    }

    // Wait a bit before looking for a new job again.
    state->jobSearchDelay = MP_FRAMERATE;
}

/** Runs job logic, if possible */
static void updateJob(MP_Unit* unit) {
    AI_State* state = &unit->ai->state;
    // Only if we have a job and we're not in the player's hand.
    if (state->job && !unit->ai->isInHand) {
        // See if we have an update method for it or have to wait before running the
        // job logic again.
        if (state->jobRunDelay > 0) {
            // Wait some more.
            --state->jobRunDelay;
        } else {
            // Otherwise update the unit based on its current job.
            assert(state->job->type->runMethod != LUA_REFNIL);
            // Set active only if the job still exists after execution!
            state->active = MP_RunJob(unit, state->job, &state->jobRunDelay) && state->job;
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
// Render
///////////////////////////////////////////////////////////////////////////////

/** Used for debug rendering of units and paths */
static GLUquadric* quadratic = 0;

static void renderPathing(const MP_Unit* unit) {
    MP_Material material;

    // If we display pathing render the units current path.
    if (MP_DBG_drawPaths && MP_IsUnitMoving(unit)) {
        const vec2* path = unit->ai->pathing.nodes;
        const unsigned int depth = unit->ai->pathing.depth;

        if (!quadratic) {
            quadratic = gluNewQuadric();
        }

        // Draw a line along the path.
        MP_InitMaterial(&material);
        material.emissivity = 1.0f;
        material.diffuseColor.c.r = 0.8f;
        material.diffuseColor.c.g = 0.8f;
        material.diffuseColor.c.b = 0.9f;
        MP_SetMaterial(&material);
        glLineWidth(1.75f);

        glBegin(GL_LINES);
        {
            unsigned int j = 0;
            glVertex3f(path[1].d.x * MP_BLOCK_SIZE, path[1].d.y * MP_BLOCK_SIZE, MP_D_DRAW_PATH_HEIGHT);
            for (j = 2; j <= depth; ++j) {
                // Somewhere in the middle, smooth the path.
                for (unsigned int k = 1; k < 20; ++k) {
                    const float t = k / 20.0f;
                    const float x = cr(path[j - 2].d.x, path[j - 1].d.x, path[j].d.x, path[j + 1].d.x, t);
                    const float y = cr(path[j - 2].d.y, path[j - 1].d.y, path[j].d.y, path[j + 1].d.y, t);
                    glVertex3f(x * MP_BLOCK_SIZE, y * MP_BLOCK_SIZE, MP_D_DRAW_PATH_HEIGHT);
                    glVertex3f(x * MP_BLOCK_SIZE, y * MP_BLOCK_SIZE, MP_D_DRAW_PATH_HEIGHT);
                }
            }
            glVertex3f(path[j - 1].d.x * MP_BLOCK_SIZE, path[j - 1].d.y * MP_BLOCK_SIZE, MP_D_DRAW_PATH_HEIGHT);
        }
        glEnd();

        // Draw points marking the waypoints.
        material.diffuseColor.c.r = 0.8f;
        material.diffuseColor.c.g = 0.4f;
        material.diffuseColor.c.b = 0.4f;
        MP_SetMaterial(&material);

        for (unsigned int j = 1; j <= depth; ++j) {
            MP_PushModelMatrix();
            MP_TranslateModelMatrix(path[j].d.x * MP_BLOCK_SIZE, path[j].d.y * MP_BLOCK_SIZE, MP_D_DRAW_PATH_HEIGHT);
            gluSphere(quadratic, 0.5f, 8, 8);
            MP_PopModelMatrix();
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
// Header implementation
///////////////////////////////////////////////////////////////////////////////

float MP_MoveTo(const MP_Unit* unit, const vec2* position) {
    AI_Path* pathing;
    unsigned int depth = MP_AI_PATH_DEPTH;
    float distance = 0;

    assert(unit);
    assert(position);

    // Find a path to it. Use temporary output data to avoid
    // overriding existing path that may be shorter.
    pathing = &unit->ai->pathing;
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
        return distance / unit->type->moveSpeed;
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

void MP_RenderPathing(const MP_Unit* unit) {
    // Render pathing debug information for the unit.
    renderPathing(unit);
}
