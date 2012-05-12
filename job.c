#include "job.h"

#include <assert.h>
#include <float.h>
#include <malloc.h>
#include <stdio.h>
#include <string.h>

#include "astar_mp.h"
#include "block.h"
#include "config.h"
#include "meta_job.h"
#include "map.h"
#include "render.h"
#include "selection.h"
#include "unit.h"
#include "vmath.h"

///////////////////////////////////////////////////////////////////////////////
// Globals
///////////////////////////////////////////////////////////////////////////////

/** List of all current jobs, per player, per type */
static MP_Job*** gJobs[MP_PLAYER_COUNT];

/** Number of used entries in the job list */
static unsigned int* gJobsCount[MP_PLAYER_COUNT];

/** Capacity of the workplace lists */
static unsigned int* gJobsCapacity[MP_PLAYER_COUNT];

/** Capacity for job types, per player; this is also the count */
static unsigned int gJobTypeCapacity[MP_PLAYER_COUNT];

///////////////////////////////////////////////////////////////////////////////
// Allocation
///////////////////////////////////////////////////////////////////////////////

static void ensureJobListSize(MP_Player player, unsigned int metaId) {
    if (metaId >= gJobTypeCapacity[player]) {
        unsigned int newCapacity = metaId + 1;

        if (!(gJobs[player] = realloc(gJobs[player], newCapacity * sizeof (MP_Job***)))) {
            MP_log_fatal("Out of memory while resizing job list.\n");
        }

        if (!(gJobsCount[player] = realloc(gJobsCount[player], newCapacity * sizeof (unsigned int)))) {
            MP_log_fatal("Out of memory while resizing job count list.\n");
        }

        if (!(gJobsCapacity[player] = realloc(gJobsCapacity[player], newCapacity * sizeof (unsigned int)))) {
            MP_log_fatal("Out of memory while resizing job capacity list.\n");
        }

        // Clear the new slots.
        for (unsigned int i = gJobTypeCapacity[player]; i < newCapacity; ++i) {
            gJobs[player][i] = NULL;
            gJobsCount[player][i] = 0;
            gJobsCapacity[player][i] = 0;
        }

        gJobTypeCapacity[player] = newCapacity;
    }
}

static void ensureJobTypeListSize(MP_Player player, unsigned int metaId) {
    ensureJobListSize(player, metaId);

    if (gJobsCount[player][metaId] + 1 > gJobsCapacity[player][metaId]) {
        gJobsCapacity[player][metaId] = gJobsCapacity[player][metaId] * 2 + 1;
        if (!(gJobs[player][metaId] = realloc(gJobs[player][metaId], gJobsCapacity[player][metaId] * sizeof (MP_Job**)))) {
            MP_log_fatal("Out of memory while resizing job list.\n");
        }
    }
}

/** Allocate a job and track it in our list */
MP_Job* MP_NewJob(MP_Player player, const MP_JobMeta* meta) {
    MP_Job* job;

    if (!meta) {
        return NULL;
    }

    // Ensure we have the capacity to add the job.
    ensureJobTypeListSize(player, meta->id);

    // Allocate the actual job.
    if (!(job = calloc(1, sizeof (MP_Job)))) {
        MP_log_fatal("Out of memory while allocating a job.\n");
    }
    job->meta = meta;

    // Store it in our list and return it.
    return gJobs[player][meta->id][gJobsCount[player][meta->id]++] = job;
}

static void deleteJob(MP_Player player, unsigned int metaId, unsigned int number) {
    // Remove this one. Notify worker that it's no longer needed.
    MP_StopJob(gJobs[player][metaId][number]);

    // Free the actual memory.
    free(gJobs[player][metaId][number]);

    // Move up the list entries to close the gap.
    --gJobsCount[player][metaId];
    memmove(&gJobs[player][metaId][number], &gJobs[player][metaId][number + 1], (gJobsCount[player][metaId] - number) * sizeof (MP_Job*));
}

/** Delete a job that is no longer used */
void MP_DeleteJob(MP_Player player, MP_Job* job) {
    if (!job) {
        return;
    }

    if (job->meta->id >= gJobTypeCapacity[player]) {
        return;
    }

    // Find the job.
    for (unsigned int number = 0; number < gJobsCount[player][job->meta->id]; ++number) {
        if (gJobs[player][job->meta->id][number] == job) {
            // Found it.
            deleteJob(player, job->meta->id, number);
            return;
        }
    }
}

void MP_DeleteJobsTargetingBlock(MP_Player player, const MP_JobMeta* meta, const MP_Block* block) {
    if (meta->id >= gJobTypeCapacity[player]) {
        return;
    }

    // Run back to front so that deletions don't matter.
    for (unsigned int number = gJobsCount[player][meta->id]; number > 0; --number) {
        MP_Job* job = gJobs[player][meta->id][number - 1];
        if (job->block == block) {
            deleteJob(player, job->meta->id, number - 1);
        }
    }
}

void MP_DeleteJobsTargetingRoom(MP_Player player, const MP_JobMeta* meta, const MP_Room* room) {
    if (meta->id >= gJobTypeCapacity[player]) {
        return;
    }

    // Run back to front so that deletions don't matter.
    for (unsigned int number = gJobsCount[player][meta->id]; number > 0; --number) {
        MP_Job* job = gJobs[player][meta->id][number - 1];
        if (job->room == room) {
            deleteJob(player, job->meta->id, number - 1);
        }
    }
}

void MP_DeleteJobsTargetingUnit(MP_Player player, const MP_JobMeta* meta, const MP_Unit* unit) {
    if (meta->id >= gJobTypeCapacity[player]) {
        return;
    }

    // Run back to front so that deletions don't matter.
    for (unsigned int number = gJobsCount[player][meta->id]; number > 0; --number) {
        MP_Job* job = gJobs[player][meta->id][number - 1];
        if (job->unit == unit) {
            deleteJob(player, job->meta->id, number - 1);
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
// Utility
///////////////////////////////////////////////////////////////////////////////

void MP_GetJobPosition(vec2* position, const MP_Job* job) {
    if (job->block) {
        unsigned short x, y;
        if (MP_GetBlockCoordinates(&x, &y, job->block)) {
            position->d.x = x + 0.5f;
            position->d.y = y + 0.5f;
        }
    } else if (job->room) {
        // TODO
    } else if (job->unit) {
        *position = job->unit->position;
    } else {
        position->d.x = 0;
        position->d.y = 0;
    }

    // Add offset declared in job.
    v2iadd(position, &job->offset);
}

///////////////////////////////////////////////////////////////////////////////
// Rendering
///////////////////////////////////////////////////////////////////////////////

/** Debug rendering of where jobs are */
static void onRender(void) {
    if (MP_d_draw_jobs) {
        MP_Material material;
        MP_InitMaterial(&material);
        material.emissivity = 1.0f;

        for (unsigned int metaId = 0; metaId < gJobTypeCapacity[MP_PLAYER_ONE]; ++metaId) {
            for (unsigned int number = 0; number < gJobsCount[MP_PLAYER_ONE][metaId]; ++number) {
                const MP_Job* job = gJobs[MP_PLAYER_ONE][metaId][number];

                // Pick a color for the job.
                material.diffuseColor.c.r = 0.4f;
                material.diffuseColor.c.g = 0.4f;
                material.diffuseColor.c.b = 0.4f;
                /*
                                switch (type) {
                                    case MP_JOB_DIG:
                                        material.diffuseColor.c.r = 0.4f;
                                        material.diffuseColor.c.g = 0.8f;
                                        material.diffuseColor.c.b = 0.4f;
                                        break;
                                    case MP_JOB_CONVERT_TILE:
                                    case MP_JOB_CONVERT_WALL:
                                        material.diffuseColor.c.r = 0.4f;
                                        material.diffuseColor.c.g = 0.4f;
                                        material.diffuseColor.c.b = 0.8f;
                                        break;

                                    default:
                                        material.diffuseColor.c.r = 0.4f;
                                        material.diffuseColor.c.g = 0.4f;
                                        material.diffuseColor.c.b = 0.4f;
                                        break;
                                }
                 */

                // Highlight if it's being worked on.
                if (job->worker) {
                    material.diffuseColor.c.r += 0.2f;
                    material.diffuseColor.c.g += 0.2f;
                    material.diffuseColor.c.b += 0.2f;
                }

                // Apply material and draw quad.
                MP_SetMaterial(&material);
                glBegin(GL_QUADS);
                {
                    vec2 position;
                    MP_GetJobPosition(&position, job);
                    glVertex3f((position.d.x - 0.1f) * MP_BLOCK_SIZE, (position.d.y - 0.1f) * MP_BLOCK_SIZE, MP_D_DRAW_PATH_HEIGHT + 0.1f);
                    glVertex3f((position.d.x + 0.1f) * MP_BLOCK_SIZE, (position.d.y - 0.1f) * MP_BLOCK_SIZE, MP_D_DRAW_PATH_HEIGHT + 0.1f);
                    glVertex3f((position.d.x + 0.1f) * MP_BLOCK_SIZE, (position.d.y + 0.1f) * MP_BLOCK_SIZE, MP_D_DRAW_PATH_HEIGHT + 0.1f);
                    glVertex3f((position.d.x - 0.1f) * MP_BLOCK_SIZE, (position.d.y + 0.1f) * MP_BLOCK_SIZE, MP_D_DRAW_PATH_HEIGHT + 0.1f);
                }
                glEnd();
            }
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
// Accessors
///////////////////////////////////////////////////////////////////////////////

MP_Job * const* MP_GetJobs(MP_Player player, const MP_JobMeta* meta, unsigned int* count) {
    ensureJobTypeListSize(player, meta->id);
    *count = gJobsCount[player][meta->id];
    return gJobs[player][meta->id];
}

MP_Job* MP_FindJob(const MP_Unit* unit, const MP_JobMeta* meta, float* distance) {
    MP_Job* closestJob = NULL;
    float closestDistance = FLT_MAX;

    if (!unit || !meta) {
        return NULL;
    }

    ensureJobTypeListSize(unit->owner, meta->id);

    // Loop through all jobs of the specified type for the owner of the unit.
    for (unsigned int number = 0; number < gJobsCount[unit->owner][meta->id]; ++number) {
        float currentDistance;
        vec2 position;
        MP_Job* job = gJobs[unit->owner][meta->id][number];

        // Based on the job type we use the target's position.
        MP_GetJobPosition(&position, job);

        // Test direct distance; if that's longer than our best we
        // can safely skip this one.
        currentDistance = v2distance(&unit->position, &position);
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
        for (unsigned int metaId = 0; metaId < gJobTypeCapacity[player]; ++metaId) {
            for (unsigned int number = 0; number < gJobsCount[player][metaId]; ++number) {
                free(gJobs[player][metaId][number]);
                gJobs[player][metaId][number] = NULL;
            }
            free(gJobs[player][metaId]);
            gJobs[player][metaId] = NULL;
        }
        free(gJobs[player]);
        gJobs[player] = NULL;
        free(gJobsCount[player]);
        gJobsCount[player] = NULL;
        free(gJobsCapacity[player]);
        gJobsCapacity[player] = NULL;

        gJobTypeCapacity[player] = 0;
    }
}

void MP_InitJobs(void) {
    MP_OnRender(onRender);

    memset(gJobs, 0, MP_PLAYER_COUNT * sizeof (MP_Job***));
    memset(gJobsCount, 0, MP_PLAYER_COUNT * sizeof (unsigned int*));
    memset(gJobsCapacity, 0, MP_PLAYER_COUNT * sizeof (unsigned int*));
    memset(gJobTypeCapacity, 0, MP_PLAYER_COUNT * sizeof (unsigned int));
}
