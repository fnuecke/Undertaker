#include "jobs.h"

#include <assert.h>
#include <float.h>
#include <malloc.h>
#include <stdio.h>
#include <string.h>

#include "astar.h"
#include "block.h"
#include "config.h"
#include "map.h"
#include "render.h"
#include "selection.h"
#include "units.h"
#include "vmath.h"
#include "jobs_meta.h"

///////////////////////////////////////////////////////////////////////////////
// Globals
///////////////////////////////////////////////////////////////////////////////

/** List of all current jobs, per player, per type */
static DK_Job*** gJobs[DK_PLAYER_COUNT];

/** Number of used entries in the job list */
static unsigned int* gJobsCount[DK_PLAYER_COUNT];

/** Capacity of the workplace lists */
static unsigned int* gJobsCapacity[DK_PLAYER_COUNT];

/** Capacity for job types, per player; this is also the count */
static unsigned int gJobTypeCapacity[DK_PLAYER_COUNT];

///////////////////////////////////////////////////////////////////////////////
// Allocation
///////////////////////////////////////////////////////////////////////////////

static void ensureJobListSize(DK_Player player, unsigned int metaId) {
    if (metaId >= gJobTypeCapacity[player]) {
        unsigned int newCapacity = metaId + 1;

        if (!(gJobs[player] = realloc(gJobs[player], newCapacity * sizeof (DK_Job***)))) {
            fprintf(stderr, "Out of memory while resizing job list.\n");
            exit(EXIT_FAILURE);
        }

        if (!(gJobsCount[player] = realloc(gJobsCount[player], newCapacity * sizeof (unsigned int)))) {
            fprintf(stderr, "Out of memory while resizing job count list.\n");
            exit(EXIT_FAILURE);
        }

        if (!(gJobsCapacity[player] = realloc(gJobsCapacity[player], newCapacity * sizeof (unsigned int)))) {
            fprintf(stderr, "Out of memory while resizing job capacity list.\n");
            exit(EXIT_FAILURE);
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

static void ensureJobTypeListSize(DK_Player player, unsigned int metaId) {
    ensureJobListSize(player, metaId);

    if (gJobsCount[player][metaId] + 1 > gJobsCapacity[player][metaId]) {
        gJobsCapacity[player][metaId] = gJobsCapacity[player][metaId] * 2 + 1;
        if (!(gJobs[player][metaId] = realloc(gJobs[player][metaId], gJobsCapacity[player][metaId] * sizeof (DK_Job**)))) {
            fprintf(stderr, "Out of memory while resizing job list.\n");
            exit(EXIT_FAILURE);
        }
    }
}

/** Allocate a job and track it in our list */
static DK_Job* newJob(DK_Player player, const DK_JobMeta* meta) {
    DK_Job* job;

    assert(meta);

    // Ensure we have the capacity to add the job.
    ensureJobTypeListSize(player, meta->id);

    // Allocate the actual job.
    if (!(job = calloc(1, sizeof (DK_Job)))) {
        fprintf(stderr, "Out of memory while allocating a job.\n");
        exit(EXIT_FAILURE);
    }
    job->meta = meta;

    // Store it in our list and return it.
    return gJobs[player][meta->id][gJobsCount[player][meta->id]++] = job;
}

/** Delete a job that is no longer used */
static void deleteJob(DK_Player player, unsigned int metaId, unsigned int number) {
    assert(gJobsCount[player][metaId] > number);
    assert(gJobs[player][metaId][number]);

    // Free the actual memory.
    free(gJobs[player][metaId][number]);

    // Move up the list entries to close the gap.
    --gJobsCount[player][metaId];
    memmove(&gJobs[player][metaId][number], &gJobs[player][metaId][number + 1], (gJobsCount[player][metaId] - number) * sizeof (DK_Job*));
}

///////////////////////////////////////////////////////////////////////////////
// Utility
///////////////////////////////////////////////////////////////////////////////

static void getJobPosition(vec2* position, const DK_Job* job) {
    if (job->block) {
        unsigned short x, y;
        DK_GetBlockCoordinates(&x, &y, job->block);
        position->d.x = x + 0.5f;
        position->d.y = y + 0.5f;
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
// Header implementation
///////////////////////////////////////////////////////////////////////////////

/** Debug rendering of where jobs are */
static void onRender(void) {
    if (DK_d_draw_jobs) {
        DK_Material material;
        DK_InitMaterial(&material);
        material.emissivity = 1.0f;

        for (unsigned int metaId = 0; metaId < gJobTypeCapacity[DK_PLAYER_ONE]; ++metaId) {
            for (unsigned int number = 0; number < gJobsCount[DK_PLAYER_ONE][metaId]; ++number) {
                const DK_Job* job = gJobs[DK_PLAYER_ONE][metaId][number];

                // Pick a color for the job.
                material.diffuseColor.c.r = 0.4f;
                material.diffuseColor.c.g = 0.4f;
                material.diffuseColor.c.b = 0.4f;
                /*
                                switch (type) {
                                    case DK_JOB_DIG:
                                        material.diffuseColor.c.r = 0.4f;
                                        material.diffuseColor.c.g = 0.8f;
                                        material.diffuseColor.c.b = 0.4f;
                                        break;
                                    case DK_JOB_CONVERT_TILE:
                                    case DK_JOB_CONVERT_WALL:
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
                DK_SetMaterial(&material);
                glBegin(GL_QUADS);
                {
                    vec2 position;
                    getJobPosition(&position, job);
                    glVertex3f((position.d.x - 0.1f) * DK_BLOCK_SIZE, (position.d.y - 0.1f) * DK_BLOCK_SIZE, DK_D_DRAW_PATH_HEIGHT + 0.1f);
                    glVertex3f((position.d.x + 0.1f) * DK_BLOCK_SIZE, (position.d.y - 0.1f) * DK_BLOCK_SIZE, DK_D_DRAW_PATH_HEIGHT + 0.1f);
                    glVertex3f((position.d.x + 0.1f) * DK_BLOCK_SIZE, (position.d.y + 0.1f) * DK_BLOCK_SIZE, DK_D_DRAW_PATH_HEIGHT + 0.1f);
                    glVertex3f((position.d.x - 0.1f) * DK_BLOCK_SIZE, (position.d.y + 0.1f) * DK_BLOCK_SIZE, DK_D_DRAW_PATH_HEIGHT + 0.1f);
                }
                glEnd();
            }
        }
    }
}

static void DK_UpdateJobsForBlock(DK_Player player, unsigned short x, unsigned short y) {
    DK_Block* block = DK_GetBlockAt(x, y);

    // Remove all old jobs that had to do with this block.
    for (unsigned int metaId = 0; metaId < gJobTypeCapacity[DK_PLAYER_ONE]; ++metaId) {
        for (unsigned int number = gJobsCount[player][metaId]; number > 0; --number) {
            DK_Job* job = gJobs[player][metaId][number - 1];
            if (job->block == block) {
                // Remove this one. Notify worker that it's no longer needed.
                DK_StopJob(job);

                // Free memory.
                deleteJob(player, metaId, number - 1);
            }
        }
    }

    // Update / recreate jobs.
    /*
        addJobOpenings(player, x, y);
        if (x > 0) {
            addJobOpenings(player, x - 1, y);
        }
        if (x < DK_GetMapSize() - 1) {
            addJobOpenings(player, x + 1, y);
        }
        if (y > 0) {
            addJobOpenings(player, x, y - 1);
        }
        if (y < DK_GetMapSize() - 1) {
            addJobOpenings(player, x, y + 1);
        }
     */
}

DK_Job* DK_FindJob(const DK_Unit* unit, const DK_JobMeta* type, float* distance) {
    DK_Job* closestJob = NULL;
    float closestDistance = FLT_MAX;

    if (!unit || !type) {
        return NULL;
    }

    ensureJobTypeListSize(unit->owner, type->id);

    // Loop through all jobs of the specified type for the owner of the unit.
    for (unsigned int number = 0; number < gJobsCount[unit->owner][type->id]; ++number) {
        float currentDistance;
        vec2 position;
        DK_Job* job = gJobs[unit->owner][type->id][number];

        // Based on the job type we use the target's position.
        getJobPosition(&position, job);

        // Test direct distance; if that's longer than our best we
        // can safely skip this one.
        currentDistance = v2distance(&unit->position, &position);
        if (currentDistance >= closestDistance) {
            continue;
        }

        // Find a path to it. We're not really interested in the actual path,
        // though, so pass null for that.
        if (DK_AStar(unit, &position, NULL, NULL, &currentDistance)) {
            // Got a path, check its length.
            if (currentDistance < closestDistance) {
                // Got a new best. Check if it's occupied, and if so
                // only take it if our path is better than the
                // direct distance to the occupant.
                if (job->worker) {
                    // This is not fail-safe, e.g.  if we're on the other side
                    // of a very long wall, but it should be good enough in most
                    // cases, and at least guarantees that *when* we steal the
                    // job, we're really closer.
                    const float workerDistance = v2distance(&job->worker->position, &position);
                    if (workerDistance <= currentDistance + DK_AI_ALREADY_WORKING_BONUS) {
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

void DK_RunJob(DK_Unit* unit, const DK_JobMeta* job) {

}

void DK_ClearJobs(void) {
    for (unsigned int player = 0; player < DK_PLAYER_COUNT; ++player) {
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

void DK_InitJobs(void) {
    DK_OnRender(onRender);

    memset(gJobs, 0, DK_PLAYER_COUNT * sizeof (DK_Job***));
    memset(gJobsCount, 0, DK_PLAYER_COUNT * sizeof (unsigned int*));
    memset(gJobsCapacity, 0, DK_PLAYER_COUNT * sizeof (unsigned int*));
    memset(gJobTypeCapacity, 0, DK_PLAYER_COUNT * sizeof (unsigned int));
}
