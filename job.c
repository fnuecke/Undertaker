#include <assert.h>
#include <float.h>
#include <stdlib.h>

#include "astar_mp.h"
#include "job.h"
#include "job_type.h"
#include "log.h"
#include "map.h"
#include "script.h"
#include "unit.h"

///////////////////////////////////////////////////////////////////////////////
// Globals
///////////////////////////////////////////////////////////////////////////////

/** List of all current jobs, per player, per type */
static MP_Job** gJobs[MP_PLAYER_COUNT][MP_TYPE_ID_MAX];

/** Number of used entries in the job list */
static unsigned int gJobsCount[MP_PLAYER_COUNT][MP_TYPE_ID_MAX];

/** Capacity of the workplace lists */
static unsigned int gJobsCapacity[MP_PLAYER_COUNT][MP_TYPE_ID_MAX];

///////////////////////////////////////////////////////////////////////////////
// Allocation
///////////////////////////////////////////////////////////////////////////////

static void ensureListCapacity(MP_Player player, unsigned int index) {
    if (gJobsCount[player][index] >= gJobsCapacity[player][index]) {
        gJobsCapacity[player][index] = gJobsCapacity[player][index] * 2 + 1;
        if (!(gJobs[player][index] =
            realloc(gJobs[player][index], gJobsCapacity[player][index] * sizeof (MP_Job**)))) {
            MP_log_fatal("Out of memory while resizing job list.\n");
        }
    }
}

static void deleteJob(MP_Player player, unsigned int index, unsigned int number) {
    assert(gJobsCount[player][index] > number);

    // Notify worker that it's no longer needed.
    MP_StopJob(gJobs[player][index][number]);

    // Free the actual memory.
    free(gJobs[player][index][number]);

    // Move up the list entries to close the gap.
    --gJobsCount[player][index];
    memmove(&gJobs[player][index][number], &gJobs[player][index][number + 1],
            (gJobsCount[player][index] - number) * sizeof (MP_Job*));
}

/** Allocate a job and track it in our list */
MP_Job* MP_NewJob(const MP_JobType* type, MP_Player player) {
    MP_Job* job;
    unsigned int index;

    assert(player > MP_PLAYER_NONE && player < MP_PLAYER_COUNT);
    assert(type);

    // Allocate the actual job.
    if (!(job = calloc(1, sizeof (MP_Job)))) {
        MP_log_fatal("Out of memory while allocating a job.\n");
    }
    job->type = type;
    job->player = player;

    // Store it in our list. Ensure we have the capacity to do so.
    index = type->info.id - 1;
    ensureListCapacity(player, index);
    gJobs[player][index][gJobsCount[player][index]++] = job;

    return job;
}

/** Delete a job that is no longer used */
void MP_DeleteJob(MP_Job* job) {
    unsigned int index, player, count;

    assert(job);

    index = job->type->info.id - 1;
    player = job->player;
    count = gJobsCount[player][index];

    // Find the job.
    for (unsigned int number = 0; number < count; ++number) {
        if (gJobs[player][index][number] == job) {
            // Found it.
            deleteJob(player, index, number);
            return;
        }
    }
}

static void deleteJobsTargeting(MP_Player player, const MP_JobType* type, MP_JobTargetType targetType, const void* target) {
    unsigned int index;

    assert(player > MP_PLAYER_NONE && player < MP_PLAYER_COUNT);
    assert(type);
    assert(target);

    // Run back to front so that deletions don't matter.
    index = type->info.id - 1;
    for (unsigned int number = gJobsCount[player][index]; number > 0; --number) {
        MP_Job* job = gJobs[player][index][number - 1];
        if (job->targetType == targetType && job->target == target) {
            deleteJob(player, index, number - 1);
        }
    }
}

void MP_DeleteJobsTargetingBlock(MP_Player player, const MP_JobType* type, const MP_Block* block) {
    deleteJobsTargeting(player, type, MP_JOB_TARGET_BLOCK, block);
}

void MP_DeleteJobsTargetingRoom(MP_Player player, const MP_JobType* type, const MP_Room* room) {
    deleteJobsTargeting(player, type, MP_JOB_TARGET_ROOM, room);
}

void MP_DeleteJobsTargetingUnit(MP_Player player, const MP_JobType* type, const MP_Unit* unit) {
    deleteJobsTargeting(player, type, MP_JOB_TARGET_UNIT, unit);
}

///////////////////////////////////////////////////////////////////////////////
// Utility
///////////////////////////////////////////////////////////////////////////////

vec2 MP_GetJobPosition(const MP_Job* job) {
    vec2 result;

    assert(job);
    assert(job->targetType == MP_JOB_TARGET_NONE || job->target);

    // Get position based on job type.
    switch (job->targetType) {
        case MP_JOB_TARGET_BLOCK:
        {
            unsigned short x, y;
            MP_GetBlockCoordinates((MP_Block*) job->target, &x, &y);
            result.d.x = x + 0.5f;
            result.d.y = y + 0.5f;
            break;
        }
        case MP_JOB_TARGET_ROOM:
            // TODO
            break;
        case MP_JOB_TARGET_UNIT:
            result = ((MP_Unit*) job->target)->position;
            break;
        default:
            result.d.x = 0;
            result.d.y = 0;
            break;
    }

    // Add offset declared in job.
    v2iadd(&result, &job->offset);

    return result;
}

///////////////////////////////////////////////////////////////////////////////
// Execution
///////////////////////////////////////////////////////////////////////////////

bool MP_RunJob(MP_Unit* unit, MP_Job* job, unsigned int* delay) {
    lua_State* L = MP_Lua();

    assert(unit);
    assert(job);

    if (job->type->runMethod == LUA_REFNIL) {
        MP_log_warning("Trying to run job '%s', which has no 'run' method.\n", job->type->info.name);
        return false;
    }

    // Try to get the callback.
    lua_rawgeti(L, LUA_REGISTRYINDEX, job->type->runMethod);
    if (lua_isfunction(L, -1)) {
        // Call it with the unit that we want to execute the script for.
        MP_Lua_PushUnit(L, unit);
        MP_Lua_PushJob(L, job);
        if (MP_Lua_pcall(L, 2, 2) == LUA_OK) {
            // We may have gotten a delay (in seconds) to wait, and an
            // indication of whether the job is active or not.
            float timeToWait = 0;
            bool active = false;
            if (lua_isnumber(L, -2)) {
                timeToWait = lua_tonumber(L, -2);
            }
            if (lua_isboolean(L, -1)) {
                active = lua_toboolean(L, -1);
            }
            lua_pop(L, 2); // pop results

            // Validate and return results.
            if (delay) {
                if (timeToWait < 0) {
                    *delay = 0;
                } else {
                    // OK, multiply with frame rate to get tick count.
                    *delay = (unsigned int) (MP_FRAMERATE * timeToWait);
                }
            }
            return active;
        } else {
            // Something went wrong.
            MP_log_error("In 'run' for job '%s': %s\n", job->type->info.name, lua_tostring(L, -1));
        }
    } else {
        MP_log_error("'run' for job '%s' isn't a function anymore.\n", job->type->info.name);
    }

    // Pop function or error message.
    lua_pop(L, 1);

    // We get here only on failure. In that case disable the run callback,
    // so we don't try this again.
    MP_DisableJobRunMethod(job->type);

    return false;
}

float MP_GetDynamicPreference(MP_Unit* unit, const MP_JobType* type) {
    lua_State* L = MP_Lua();

    assert(unit);
    assert(type);

    if (type->dynamicPreference == LUA_REFNIL) {
        MP_log_warning("Trying to get dynamic preference for job '%s', which has no 'preference' callback.\n", type->info.name);
        return 0;
    }

    // Try to get the callback.
    lua_rawgeti(L, LUA_REGISTRYINDEX, type->dynamicPreference);
    if (lua_isfunction(L, -1)) {
        // Call it.
        MP_Lua_PushUnit(L, unit);
        if (MP_Lua_pcall(L, 1, 1) == LUA_OK) {
            // OK, try to get the result as a float.
            if (lua_isnumber(L, -1)) {
                float preference = lua_tonumber(L, -1);
                lua_pop(L, 1);
                return preference;
            } else {
                MP_log_error("'preference' for job '%s' returned something that's not a number.\n", type->info.name);
            }
        } else {
            // Something went wrong.
            MP_log_error("In 'preference' for job '%s': %s\n", type->info.name, lua_tostring(L, -1));
        }
    } else {
        MP_log_error("'preference' for job '%s' isn't a function anymore.\n", type->info.name);
    }

    // Pop function or error message or result.
    lua_pop(L, 1);

    // We get here only on failure. In that case disable the dynamic preference,
    // so we don't try this again.
    MP_DisableDynamicPreference(type);

    return 0;
}

///////////////////////////////////////////////////////////////////////////////
// Accessors
///////////////////////////////////////////////////////////////////////////////

MP_JobList MP_GetJobs(const MP_JobType* type, MP_Player player, unsigned int* count) {
    unsigned int index;

    assert(player > MP_PLAYER_NONE && player < MP_PLAYER_COUNT);
    assert(type);
    assert(count);

    index = type->info.id - 1;
    *count = gJobsCount[player][index];
    return gJobs[player][index];
}

MP_Job* MP_FindJob(const MP_Unit* unit, const MP_JobType* type, float* distance) {
    MP_Job* closestJob = NULL;
    float closestDistance = FLT_MAX;
    unsigned int index;

    assert(unit);
    assert(type);

    index = type->info.id - 1;
    ensureListCapacity(unit->owner, index);

    // Loop through all jobs of the specified type for the owner of the unit.
    for (unsigned int number = 0; number < gJobsCount[unit->owner][index]; ++number) {
        MP_Job* job = gJobs[unit->owner][index][number];

        // Based on the job type we use the target's position.
        vec2 position = MP_GetJobPosition(job);

        // Test direct distance; if that's longer than our best we
        // can safely skip this one.
        float currentDistance = v2distance(&unit->position, &position);
        if (currentDistance >= closestDistance) {
            continue;
        }

        // Find a path to it. We're not really interested in the actual path,
        // though, so pass null for that.
        if (MP_AStar(unit, &position, NULL, NULL, &currentDistance)) {
            // Got a path, check its length.
            if (currentDistance < closestDistance) {
                // Got a new best. Check if it's occupied, and if so
                // only take it if our path is better than the
                // direct distance to the occupant.
                if (job->worker && job->worker != unit) {
                    // This is not fail-safe, e.g.  if we're on the other side
                    // of a very long wall, but it should be good enough in most
                    // cases, and at least guarantees that *when* we steal the
                    // job, we're really closer.
                    const float workerDistance = v2distance(&job->worker->position, &position);
                    if (workerDistance <= currentDistance + MP_AI_ALREADY_WORKING_BONUS) {
                        // The one that's on it is better suited, ignore job.
                        continue;
                    }
                }

                // Copy data.
                closestDistance = currentDistance;
                closestJob = job;
            }
        }
    }

    // Return distance if desired.
    if (distance && closestJob) {
        *distance = closestDistance;
    }

    return closestJob;
}

///////////////////////////////////////////////////////////////////////////////
// Init / Teardown
///////////////////////////////////////////////////////////////////////////////

void MP_ClearJobs(void) {
    for (unsigned int player = 0; player < MP_PLAYER_COUNT; ++player) {
        for (unsigned int typeId = 0; typeId < MP_TYPE_ID_MAX; ++typeId) {
            for (unsigned int number = 0; number < gJobsCount[player][typeId]; ++number) {
                free(gJobs[player][typeId][number]);
            }
            free(gJobs[player][typeId]);
            gJobs[player][typeId] = NULL;
            gJobsCount[player][typeId] = 0;
            gJobsCapacity[player][typeId] = 0;
        }
    }
}

void MP_InitJobs(void) {
    memset(gJobs, 0, MP_PLAYER_COUNT * MP_TYPE_ID_MAX * sizeof (MP_Job**));
    memset(gJobsCount, 0, MP_PLAYER_COUNT * MP_TYPE_ID_MAX * sizeof (unsigned int));
    memset(gJobsCapacity, 0, MP_PLAYER_COUNT * MP_TYPE_ID_MAX * sizeof (unsigned int));
}
