#include "jobs.h"

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

/** List of all current jobs, per player */
static DK_Job** gJobs[DK_PLAYER_COUNT][DK_JOB_TYPE_COUNT];

/** Number of used entries in the lists */
static unsigned int gJobsCount[DK_PLAYER_COUNT][DK_JOB_TYPE_COUNT];

/** Capacity of the workplace lists */
static unsigned int gJobsCapacity[DK_PLAYER_COUNT][DK_JOB_TYPE_COUNT];

///////////////////////////////////////////////////////////////////////////////
// Allocation
///////////////////////////////////////////////////////////////////////////////

/** Allocate a job and track it in our list */
static DK_Job* newJob(DK_Player player, DK_JobType type) {
    DK_Job* job;

    // Ensure we have the capacity to add the job.
    if (gJobsCount[player][type] + 1 > gJobsCapacity[player][type]) {
        gJobsCapacity[player][type] = DK_AI_JOB_CAPACITY_GROWTH(gJobsCapacity[player][type]);
        if (!(gJobs[player][type] = realloc(gJobs[player][type], gJobsCapacity[player][type] * sizeof (DK_Job*)))) {
            fprintf(stderr, "Out of memory while resizing job list.\n");
            exit(EXIT_FAILURE);
        }
    }

    // Allocate the actual job.
    job = calloc(1, sizeof (DK_Job));
    if (!job) {
        fprintf(stderr, "Out of memory while allocating a job.\n");
        exit(EXIT_FAILURE);
    }

    // Store it in our list and return it.
    return gJobs[player][type][gJobsCount[player][type]++] = job;
}

/** Delete a job that is no longer used */
static void deleteJob(DK_Player player, DK_JobType type, unsigned int id) {
    // Free the actual memory.
    free(gJobs[player][type][id]);

    // Move up the list entries to close the gap.
    --gJobsCount[player][type];
    memmove(&gJobs[player][type][id], &gJobs[player][type][id + 1], (gJobsCount[player][type] - id) * sizeof (DK_Job*));
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
// Creation
///////////////////////////////////////////////////////////////////////////////

/** Sides of a block (for job openings) */
typedef enum Side {
    SIDE_NONE = 0,
    SIDE_SAME = 1,
    SIDE_NORTH = 2,
    SIDE_EAST = 4,
    SIDE_SOUTH = 8,
    SIDE_WEST = 16
} Side;

/** Check for block related jobs of a specified type for a specified block */
static Side getExistingJobs(const DK_Block* block, unsigned short x, unsigned short y, DK_Player player, DK_JobType type) {
    Side existingJobs = SIDE_NONE;
    for (unsigned int jobId = 0; jobId < gJobsCount[player][type]; ++jobId) {
        const DK_Job* job = gJobs[player][type][jobId];
        if (job->block != block) {
            continue;
        }
        if (job->offset.d.y > 0) {
            // Top.
            existingJobs |= SIDE_NORTH;
        } else if (job->offset.d.y < 0) {
            // Bottom.
            existingJobs |= SIDE_SOUTH;
        } else if (job->offset.d.x < 0) {
            // Left.
            existingJobs |= SIDE_WEST;
        } else if (job->offset.d.x > 0) {
            // Right.
            existingJobs |= SIDE_EAST;
        } else {
            // Same block.
            existingJobs |= SIDE_SAME;
        }
    }
    return existingJobs;
}

static void addJobOpenings(DK_Player player, unsigned short x, unsigned short y) {
    DK_Block* block = DK_GetBlockAt(x, y);

    // Check which jobs are already taken care of. There are 5 slots: the block
    // itself (converting) and the four non-diagonal neighbors (dig/convert).
    if (DK_IsBlockSelected(player, x, y)) {
        // It's selected, start digging if a neighboring tile is passable.
        Side existingJobs = getExistingJobs(block, x, y, player, DK_JOB_DIG);
        if (!(existingJobs & SIDE_NORTH) &&
                DK_IsBlockPassable(DK_GetBlockAt(x, y - 1))) {
            // Top is valid.
            DK_Job* job = newJob(player, DK_JOB_DIG);
            job->block = block;
            job->offset.d.y = 0.75f;
        }
        if (!(existingJobs & SIDE_SOUTH) &&
                DK_IsBlockPassable(DK_GetBlockAt(x, y + 1))) {
            // Bottom is valid.
            DK_Job* job = newJob(player, DK_JOB_DIG);
            job->block = block;
            job->offset.d.y = -0.75f;
        }
        if (!(existingJobs & SIDE_EAST) &&
                DK_IsBlockPassable(DK_GetBlockAt(x + 1, y))) {
            // Right is valid.
            DK_Job* job = newJob(player, DK_JOB_DIG);
            job->block = block;
            job->offset.d.x = 0.75f;
        }
        if (!(existingJobs & SIDE_WEST) &&
                DK_IsBlockPassable(DK_GetBlockAt(x - 1, y))) {
            // Left is valid.
            DK_Job* job = newJob(player, DK_JOB_DIG);
            job->block = block;
            job->offset.d.x = -0.75f;
        }
    } else if (DK_IsBlockConvertible(block) &&
            (block->owner != player ||
            block->strength < block->meta->strength)) {
        // Not selected, and either not owned by the player, so we want to start
        // conquering, or owned but below max strength (i.e. an attempt to
        // conquer it was made by another player), so we want to repair it.
        DK_Block* b;
        if (DK_IsBlockPassable(block)) {
            // Convert by standing in the middle of the tile.
            Side existingJobs = getExistingJobs(block, x, y, player, DK_JOB_CONVERT_TILE);
            if (!(existingJobs & SIDE_SAME) && (
                    // Check if a neighboring tile is owned by the player.
                    // North?
                    ((b = DK_GetBlockAt(x, y + 1)) &&
                    DK_IsBlockPassable(b) &&
                    b->owner == player) ||
                    // South?
                    ((b = DK_GetBlockAt(x, y - 1)) &&
                    DK_IsBlockPassable(b) &&
                    b->owner == player) ||
                    // East?
                    ((b = DK_GetBlockAt(x + 1, y)) &&
                    DK_IsBlockPassable(b) &&
                    b->owner == player) ||
                    // West?
                    ((b = DK_GetBlockAt(x - 1, y)) &&
                    DK_IsBlockPassable(b) &&
                    b->owner == player))) {
                // Valid for conversion.
                DK_Job* job = newJob(player, DK_JOB_CONVERT_TILE);
                job->block = block;
            }
        } else {
            // Check if a neighboring tile is owned by the same player.
            Side existingJobs = getExistingJobs(block, x, y, player, DK_JOB_CONVERT_WALL);
            if (!(existingJobs & SIDE_NORTH) &&
                    (b = DK_GetBlockAt(x, y - 1)) &&
                    DK_IsBlockPassable(b) &&
                    b->owner == player) {
                // Top is valid.
                DK_Job* job = newJob(player, DK_JOB_CONVERT_WALL);
                job->block = block;
                job->offset.d.x = (x + 0.5f);
                job->offset.d.y = (y - 0.25f);
            }
            if (!(existingJobs & SIDE_SOUTH) &&
                    (b = DK_GetBlockAt(x, y + 1)) &&
                    DK_IsBlockPassable(b) &&
                    b->owner == player) {
                // Bottom is valid.
                DK_Job* job = newJob(player, DK_JOB_CONVERT_WALL);
                job->block = block;
                job->offset.d.x = (x + 0.5f);
                job->offset.d.y = (y + 1.25f);
            }
            if (!(existingJobs & SIDE_EAST) &&
                    (b = DK_GetBlockAt(x + 1, y)) &&
                    DK_IsBlockPassable(b) &&
                    b->owner == player) {
                // Right is valid.
                DK_Job* job = newJob(player, DK_JOB_CONVERT_WALL);
                job->block = block;
                job->offset.d.x = (x + 1.25f);
                job->offset.d.y = (y + 0.5f);
            }
            if (!(existingJobs & SIDE_WEST) &&
                    (b = DK_GetBlockAt(x - 1, y)) &&
                    DK_IsBlockPassable(b) &&
                    b->owner == player) {
                // Left is valid.
                DK_Job* job = newJob(player, DK_JOB_CONVERT_WALL);
                job->block = block;
                job->offset.d.x = (x - 0.25f);
                job->offset.d.y = (y + 0.5f);
            }
        }
    }
    // TODO check for room, and if it offers a job
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

        for (unsigned int type = 0; type < DK_JOB_TYPE_COUNT; ++type) {
            for (unsigned int i = 0; i < gJobsCount[DK_PLAYER_ONE][type]; ++i) {
                const DK_Job* job = gJobs[DK_PLAYER_ONE][type][i];

                // Pick a color for the job.
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

/** Reset upon map change */
static void onMapChange(void) {
    for (unsigned int player = 0; player < DK_PLAYER_COUNT; ++player) {
        for (unsigned int jobType = 0; jobType < DK_JOB_TYPE_COUNT; ++jobType) {
            for (unsigned int jobIndex = gJobsCount[player][jobType]; jobIndex > 0; --jobIndex) {
                free(gJobs[player][jobType][jobIndex - 1]);
                gJobs[player][jobType][jobIndex - 1] = 0;
            }
            gJobsCount[player][jobType] = 0;
        }
    }
}

void DK_UpdateJobsForBlock(DK_Player player, unsigned short x, unsigned short y) {
    DK_Block* block = DK_GetBlockAt(x, y);
    for (unsigned int jobType = 0; jobType < DK_JOB_TYPE_COUNT; ++jobType) {
        for (unsigned int jobIndex = gJobsCount[player][jobType]; jobIndex > 0; --jobIndex) {
            const DK_Job* job = gJobs[player][jobType][jobIndex - 1];
            if (job->block == block) {
                // Remove this one. Notify worker that it's no longer needed.
                if (job->worker) {
                    DK_StopJob(job->worker, jobType);
                }

                // Free memory.
                deleteJob(player, jobType, jobIndex - 1);
            }
        }
    }

    // Update / recreate jobs.
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
}

DK_Job* DK_FindJob(const DK_Unit* unit, const DK_JobMeta* type, float* distance) {
    DK_Job* closestJob = 0;
    float closestDistance = FLT_MAX;

    // Loop through all jobs of the specified type for the owner of the unit.
    for (unsigned int jobId = gJobsCount[unit->owner][type->id]; jobId > 0; --jobId) {
        float currentDistance;
        vec2 position;
        DK_Job* job = gJobs[unit->owner][type->id][jobId];

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

    if (distance && closestJob) {
        *distance = closestDistance;
    }

    return closestJob;
}

void DK_RunJob(DK_Unit* unit, DK_JobMeta* job) {
    
}

void DK_InitJobs(void) {
    DK_OnMapSizeChange(onMapChange);
    DK_OnRender(onRender);

    memset(gJobs, 0, DK_PLAYER_COUNT * DK_JOB_TYPE_COUNT * sizeof (DK_Job**));
    memset(gJobsCount, 0, DK_PLAYER_COUNT * DK_JOB_TYPE_COUNT * sizeof (unsigned int));
    memset(gJobsCapacity, 0, DK_PLAYER_COUNT * DK_JOB_TYPE_COUNT * sizeof (unsigned int));
}
