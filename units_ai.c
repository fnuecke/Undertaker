#include <assert.h>
#include <float.h>
#include <math.h>
#include <stdlib.h>

#include "block.h"
#include "jobs.h"
#include "map.h"
#include "units.h"
#include "units_ai.h"

///////////////////////////////////////////////////////////////////////////////
// Globals
///////////////////////////////////////////////////////////////////////////////

/** Lookup table for update methods */
static void(*update[DK_JOB_TYPE_COUNT])(DK_Unit*);

///////////////////////////////////////////////////////////////////////////////
// Utility methods
///////////////////////////////////////////////////////////////////////////////

/** Catmull-rom interpolation */
static float cr(float p0, float p1, float p2, float p3, float t) {
    const float m1 = DK_AI_CATMULL_ROM_T * (p2 - p0);
    const float m2 = DK_AI_CATMULL_ROM_T * (p3 - p1);
    return (2 * p1 - 2 * p2 + m1 + m2) * t * t * t + (-3 * p1 + 3 * p2 - 2 * m1 - m2) * t * t + m1 * t + p1;
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
            unit->position.d.x = path->nodes[path->index - 1].x;
            unit->position.d.y = path->nodes[path->index - 1].y;
            return;
        } else {
            // Subtract length of previous to carry surplus movement.
            path->traveled -= path->distance;

            // Do a direct check for distance, to allow skipping equal
            // nodes.
            {
                const float dx = path->nodes[path->index].x - path->nodes[path->index - 1].x;
                const float dy = path->nodes[path->index].y - path->nodes[path->index - 1].y;
                path->distance = sqrtf(dx * dx + dy * dy);
            }
            // If there is a distance, estimate the actual path length.
            if (path->distance > 0 && DK_AI_PATH_INTERPOLATE) {
                int e;
                float x, y, dx, dy;
                float lx = path->nodes[path->index - 1].x;
                float ly = path->nodes[path->index - 1].y;
                path->distance = 0;
                for (e = 1; e <= DK_AI_PATH_INTERPOLATION; ++e) {
                    const float t = e / (float) DK_AI_PATH_INTERPOLATION;
                    x = cr(path->nodes[path->index - 2].x, path->nodes[path->index - 1].x, path->nodes[path->index].x, path->nodes[path->index + 1].x, t);
                    y = cr(path->nodes[path->index - 2].y, path->nodes[path->index - 1].y, path->nodes[path->index].y, path->nodes[path->index + 1].y, t);
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
        unit->position.d.x = cr(path->nodes[path->index - 2].x, path->nodes[path->index - 1].x, path->nodes[path->index].x, path->nodes[path->index + 1].x, t);
        unit->position.d.y = cr(path->nodes[path->index - 2].y, path->nodes[path->index - 1].y, path->nodes[path->index].y, path->nodes[path->index + 1].y, t);
    }
}

/** Updates unit satisfaction based on currently performed job and other states */
static void updateSatisfaction(DK_Unit* unit) {
    const DK_UnitSatisfactionMeta* meta = &unit->meta->satisfaction;
    DK_UnitSatisfaction* state = &unit->satisfaction;
    const AI_State* ai = unit->ai->current;

    // Update job satisfaction values. Remember if there are any jobs we're not
    // satisfied with (except the one we're currently doing).
    unsigned char someUnsatisfied = 0;
    for (unsigned int jobType; jobType < DK_JOB_TYPE_COUNT; ++jobType) {
        // Check if the unit even does this job.
        if (!unit->meta->jobs[jobType]) {
            continue;
        }

        // Yes, check it's currently performing it.
        if (ai->jobType == jobType) {
            // Yes, update for the respective delta.
            state->jobSatisfaction[jobType] += meta->jobs[jobType].performingDelta;
        } else {
            // No, update for the respective delta.
            state->jobSatisfaction[jobType] += meta->jobs[jobType].notPerformingDelta;
        }

        // Make sure it's in bounds.
        if (state->jobSatisfaction[jobType] < 0) {
            state->jobSatisfaction[jobType] = 0;
        } else if (state->jobSatisfaction[jobType] > 1.0f) {
            state->jobSatisfaction[jobType] = 1.0f;
        }

        // Are we unsatisfied with this job (and it's not the current one)?
        if (ai->jobType != jobType &&
                state->jobSatisfaction[jobType] < meta->jobs[jobType].unsatisfiedThreshold) {
            someUnsatisfied = 1;
        }
    }

    // TODO check if unit is held in hand, apply delta if so.

    // Check if we are not unsatisfied with our current job and there's another
    // one we are. If so, cancel the current job to allow starting the one we're
    // unsatisfied with.
    if (someUnsatisfied &&
            state->jobSatisfaction[ai->jobType] >= meta->jobs[ai->jobType].unsatisfiedThreshold) {
        // Indeed, so pop the current job.
        DK_StopJob(unit, ai->jobType);
    }
}

/** Computes additional job weight based on satisfaction */
static float satisfactionWeight(float satisfaction, const DK_UnitJobSatisfactionMeta* meta) {
    // Map satisfaction to interval between unsatisfied and satisfied thresh.
    satisfaction = (satisfaction - meta->unsatisfiedThreshold) / (meta->satisfiedThreshold - meta->unsatisfiedThreshold);
    if (satisfaction < 0.0f) {
        satisfaction = 0.0f;
    } else if (satisfaction > 1.0f) {
        satisfaction = 1.0f;
    }
    return meta->preference * satisfaction;
}

/** Looks for the most desirable job for the unit */
static void findJob(DK_Unit* unit) {
    // Shortcuts.
    const DK_UnitSatisfactionMeta* meta = &unit->meta->satisfaction;
    const DK_UnitSatisfaction* state = &unit->satisfaction;

    // For closest job search.
    DK_JobType bestJobType = DK_JOB_NONE;
    DK_Job* bestJob = 0;
    float bestWeightedDistance = FLT_MAX;

    // Do a quick scan to see if the unit has any unsatisfied job desires.
    unsigned char someUnsatisfied = 0;
    for (unsigned int jobType = 0; jobType < DK_JOB_TYPE_COUNT; ++jobType) {
        // Check if the unit can handle this job type.
        if (!unit->meta->jobs[jobType]) {
            continue;
        }

        // Check if the unit is unsatisfied.
        if (state->jobSatisfaction[jobType] < meta->jobs[jobType].unsatisfiedThreshold) {
            someUnsatisfied = 1;
        }
    }

    // Find the closest job, weighted based on the unit's preferences.
    for (unsigned int jobType = 0; jobType < DK_JOB_TYPE_COUNT; ++jobType) {
        float distance;
        DK_Job* job;

        // Check if the unit can handle this job type.
        if (!unit->meta->jobs[jobType]) {
            continue;
        }

        // Check if the unit is unsatisfied or doesn't have others unsatisfied.
        if (someUnsatisfied &&
                state->jobSatisfaction[jobType] >= meta->jobs[jobType].unsatisfiedThreshold) {
            // Unit isn't unsatisfied with this job, but some other, so skip
            // this one.
            continue;
        }

        // Find closest job opening.
        if (!(job = DK_FindJob(unit, jobType, &distance))) {
            // Didn't find a job of this type.
            continue;
        }

        // Weigh the distance based on the satisfaction and preference.
        distance -= satisfactionWeight(state->jobSatisfaction[jobType], &meta->jobs[jobType]);

        // Better than other jobs we have found?
        if (distance < bestWeightedDistance) {
            bestJob = job;
            bestJobType = jobType;
            bestWeightedDistance = distance;
        }
    }

    // Check if we found something to do.
    if (bestJob) {
        // Fire previous workers.
        if (bestJob->worker) {
            DK_StopJob(bestJob->worker, bestJobType);
        }

        // Push that job onto the stack.
        {
            AI_State* ai = --unit->ai->current;
            ai->jobType = bestJobType;
            ai->jobInfo = bestJob;
            ai->delay = 0;
            ai->shouldCancel = 0;
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
            const float dlx = pathing->nodes[1].x - pathing->nodes[2].x;
            const float dly = pathing->nodes[1].y - pathing->nodes[2].y;
            const float l = sqrtf(dlx * dlx + dly * dly);
            pathing->nodes[0].x = pathing->nodes[1].x;
            pathing->nodes[0].y = pathing->nodes[1].y;
            if (l > 0) {
                pathing->nodes[0].x += dlx / l;
                pathing->nodes[0].y += dly / l;
            }
        }
        {
            const float dlx = pathing->nodes[depth].x - pathing->nodes[depth - 1].x;
            const float dly = pathing->nodes[depth].y - pathing->nodes[depth - 1].y;
            const float l = sqrtf(dlx * dlx + dly * dly);
            pathing->nodes[depth + 1].x = pathing->nodes[depth].x;
            pathing->nodes[depth + 1].y = pathing->nodes[depth].y;
            if (l > 0) {
                pathing->nodes[depth + 1].x += dlx / l;
                pathing->nodes[depth + 1].y += dly / l;
            }
        }

        // Success.
        return 1;
    }

    // Could not find a path.
    return 0;
}

///////////////////////////////////////////////////////////////////////////////
// Job implementations
///////////////////////////////////////////////////////////////////////////////

static void updateWander(DK_Unit* unit) {
    AI_State* state = unit->ai->current;

    // Just walk around dumbly.
    vec2 position = {
        {
            unit->position.d.x + DK_AI_WANDER_RANGE * (rand() / (float) RAND_MAX - 0.5f),
            unit->position.d.y + DK_AI_WANDER_RANGE * (rand() / (float) RAND_MAX - 0.5f)
        }
    };

    // TODO Make sure the unit doesn't wander too close to a wall.
    /*
        if (position.d.x < 0.2f) {
            position.d.x = 0.2f;
        }
        if (position.d.x > DK_GetMapSize() - 0.2f) {
            position.d.x = DK_GetMapSize() - 0.2f;
        }
        if (position.d.y < 0.2f) {
            position.d.y = 0.2f;
        }
        if (position.d.y > DK_GetMapSize() - 0.2f) {
            position.d.y = DK_GetMapSize() - 0.2f;
        }
     */

    // Check if the block is valid for the unit.
    if (DK_IsBlockPassableBy(DK_GetBlockAt((int) floorf(position.d.x), (int) floorf(position.d.y)), unit)) {
        // OK go.
        moveTo(unit, &position);

        // Update delay. Wait some before wandering again.
        state->delay = (DK_AI_WANDER_DELAY / 2) + (rand() * (DK_AI_WANDER_DELAY / 2) / RAND_MAX);
    }
}

static void updateDig(DK_Unit* unit) {
    AI_State* state = unit->ai->current;

    assert(state->jobInfo);

    // Else attack the dirt! Update cooldown and apply damage.
/*
    unit->cooldowns[DK_ABILITY_IMP_ATTACK] =
            cooldowns[DK_UNIT_IMP][DK_ABILITY_IMP_ATTACK];
    if (DK_DamageBlock(state->job->block, damage[DK_UNIT_IMP][DK_ABILITY_IMP_ATTACK])) {
        // Block was destroyed! Job done.
        --unit->ai_count;
    }
*/
}

static void updateConvert(DK_Unit* unit) {
    AI_State* state = unit->ai->current;

    assert(state->jobInfo);

    // Else hoyoyo! Update cooldown and apply conversion.
/*
    unit->cooldowns[DK_ABILITY_IMP_CONVERT] =
            cooldowns[DK_UNIT_IMP][DK_ABILITY_IMP_CONVERT];
    if (DK_ConvertBlock(state->job->block, damage[DK_UNIT_IMP][DK_ABILITY_IMP_CONVERT], unit->owner)) {
        // Block was destroyed! Job done.
        --unit->ai_count;
    }
*/
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

    // Update a unit's satisfaction values. This will also make the unit lose
    // its current job, if it's unsatisfied with another one (and isn't with
    // this one, yet).
    updateSatisfaction(unit);

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
    if (!update[state->jobType]) {
        // Pop it.
        ++unit->ai->current;
    } else if (state->delay) {
        // Wait some more.
        --state->delay;
    } else {
        // Otherwise update the unit based on its current job.
        switch (state->jobType) {
            case DK_JOB_WANDER:
                updateWander(unit);
                break;
            case DK_JOB_DIG:
                updateDig(unit);
                break;
            case DK_JOB_CONVERT_TILE:
                updateConvert(unit);
                break;
            case DK_JOB_CONVERT_WALL:
                updateConvert(unit);
                break;
            default:
                break;
        }
    }
}
