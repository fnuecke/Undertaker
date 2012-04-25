#include <malloc.h>
#include <stdio.h>
#include <string.h>

#include "jobs.h"
#include "selection.h"
#include "config.h"

///////////////////////////////////////////////////////////////////////////////
// Globals
///////////////////////////////////////////////////////////////////////////////

/** List of all current jobs, per player */
static DK_Job** jobs[DK_PLAYER_COUNT] = {0};

/** Capacity of the workplace lists */
static unsigned int jobs_capacity[DK_PLAYER_COUNT] = {0};

/** Number of used entries in the lists */
static unsigned int jobs_count[DK_PLAYER_COUNT] = {0};

///////////////////////////////////////////////////////////////////////////////
// Allocation
///////////////////////////////////////////////////////////////////////////////

/** Allocate a job and track it in our list */
static DK_Job* get_job(DK_Player player) {
    DK_Job* job;

    // Ensure we have the capacity to add the job.
    if (jobs_count[player] + 1 > jobs_capacity[player]) {
        jobs_capacity[player] = DK_AI_JOB_CAPACITY_GROWTH(jobs_capacity[player]);
        if (!(jobs[player] = realloc(jobs[player], jobs_capacity[player] * sizeof (DK_Job*)))) {
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
    return jobs[player][jobs_count[player]++] = job;
}

/** Delete a job that is no longer used */
static void delete_job(DK_Player player, unsigned int job_index) {
    // Free the actual memory.
    free(jobs[player][job_index]);

    // Move up the list entries to close the gap.
    --jobs_count[player];
    memmove(&jobs[player][job_index], &jobs[player][job_index + 1], (jobs_count[player] - job_index) * sizeof (DK_Job*));
}

typedef enum {
    SAME = 1,
    NORTH = 2,
    EAST = 4,
    SOUTH = 8,
    WEST = 16
} JobNeighbors;

static void jobs_add(DK_Player player, unsigned short x, unsigned short y) {
    DK_Block* block = DK_GetBlockAt(x, y);

    // Check which jobs are already taken care of. There are 5 slots: the block
    // itself (converting) and the four non-diagonal neighbors (dig/convert).
    JobNeighbors existing_jobs = 0, i;
    for (i = 0; i < jobs_count[player]; ++i) {
        const DK_Job* job = jobs[player][i];
        if (job->block != block) {
            continue;
        }
        if (job->position.v[0] < x) {
            // Left.
            existing_jobs |= WEST;
        } else if (job->position.v[0] > x + 1) {
            // Right.
            existing_jobs |= EAST;
        } else if (job->position.v[1] < y) {
            // Top.
            existing_jobs |= NORTH;
        } else if (job->position.v[1] > y + 1) {
            // Bottom.
            existing_jobs |= SOUTH;
        } else {
            // Same block.
            existing_jobs |= SAME;
        }
    }


    if (DK_IsBlockSelectable(player, x, y)) {
        // Valid block for working on.
        if (DK_IsBlockSelected(player, x, y)) {
            // It's selected, start digging.
            // Check if a neighboring tile is passable.
            DK_Block* b;
            if (!(existing_jobs & WEST) &&
                    (b = DK_GetBlockAt(x - 1, y)) &&
                    DK_IsBlockOpen(b)) {
                // Left is valid.
                DK_Job* job = get_job(player);
                job->block = block;
                job->position.v[0] = (x - 0.25f);
                job->position.v[1] = (y + 0.5f);
                job->type = DK_JOB_DIG;
                job->worker = 0;
            }
            if (!(existing_jobs & EAST) &&
                    (b = DK_GetBlockAt(x + 1, y)) &&
                    DK_IsBlockOpen(b)) {
                // Right is valid.
                DK_Job* job = get_job(player);
                job->block = block;
                job->position.v[0] = (x + 1.25f);
                job->position.v[1] = (y + 0.5f);
                job->type = DK_JOB_DIG;
                job->worker = 0;
            }
            if (!(existing_jobs & NORTH) &&
                    (b = DK_GetBlockAt(x, y - 1)) &&
                    DK_IsBlockOpen(b)) {
                // Top is valid.
                DK_Job* job = get_job(player);
                job->block = block;
                job->position.v[0] = (x + 0.5f);
                job->position.v[1] = (y - 0.25f);
                job->type = DK_JOB_DIG;
                job->worker = 0;
            }
            if (!(existing_jobs & SOUTH) &&
                    (b = DK_GetBlockAt(x, y + 1)) &&
                    DK_IsBlockOpen(b)) {
                // Bottom is valid.
                DK_Job* job = get_job(player);
                job->block = block;
                job->position.v[0] = (x + 0.5f);
                job->position.v[1] = (y + 1.25f);
                job->type = DK_JOB_DIG;
                job->worker = 0;
            }
        } else if (block->owner == DK_PLAYER_NONE) {
            // Not selected, and not owned, start conquering.
            // Check if a neighboring tile is owned by the same player.
            DK_Block* b;
            if (!(existing_jobs & WEST) &&
                    (b = DK_GetBlockAt(x - 1, y)) &&
                    DK_IsBlockOpen(b) &&
                    b->owner == player) {
                // Left is valid.
                DK_Job* job = get_job(player);
                job->block = block;
                job->position.v[0] = (x - 0.25f);
                job->position.v[1] = (y + 0.5f);
                job->type = DK_JOB_CONVERT;
                job->worker = 0;
            }
            if (!(existing_jobs & EAST) &&
                    (b = DK_GetBlockAt(x + 1, y)) &&
                    DK_IsBlockOpen(b) &&
                    b->owner == player) {
                // Right is valid.
                DK_Job* job = get_job(player);
                job->block = block;
                job->position.v[0] = (x + 1.25f);
                job->position.v[1] = (y + 0.5f);
                job->type = DK_JOB_CONVERT;
                job->worker = 0;
            }
            if (!(existing_jobs & NORTH) &&
                    (b = DK_GetBlockAt(x, y - 1)) &&
                    DK_IsBlockOpen(b) &&
                    b->owner == player) {
                // Top is valid.
                DK_Job* job = get_job(player);
                job->block = block;
                job->position.v[0] = (x + 0.5f);
                job->position.v[1] = (y - 0.25f);
                job->type = DK_JOB_CONVERT;
                job->worker = 0;
            }
            if (!(existing_jobs & SOUTH) &&
                    (b = DK_GetBlockAt(x, y + 1)) &&
                    DK_IsBlockOpen(b) &&
                    b->owner == player) {
                // Bottom is valid.
                DK_Job* job = get_job(player);
                job->block = block;
                job->position.v[0] = (x + 0.5f);
                job->position.v[1] = (y + 1.25f);
                job->type = DK_JOB_CONVERT;
                job->worker = 0;
            }
        }
    } else if (!(existing_jobs & SAME) &&
            block->type == DK_BLOCK_NONE &&
            block->owner != player) {
        // Block is empty tile and not already owned.
        // Check if a neighboring tile is owned by the same player.
        DK_Block* b;
        if (((b = DK_GetBlockAt(x - 1, y)) &&
                DK_IsBlockOpen(b) &&
                b->owner == player) ||

                ((b = DK_GetBlockAt(x + 1, y)) &&
                DK_IsBlockOpen(b) &&
                b->owner == player) ||

                ((b = DK_GetBlockAt(x, y - 1)) &&
                DK_IsBlockOpen(b) &&
                b->owner == player) ||

                ((b = DK_GetBlockAt(x, y + 1)) &&
                DK_IsBlockOpen(b) &&
                b->owner == player)) {
            DK_Job* job = get_job(player);
            job->block = block;
            job->position.v[0] = (x + 0.5f);
            job->position.v[1] = (y + 0.5f);
            job->type = DK_JOB_CONVERT;
            job->worker = 0;
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
// Header implementation
///////////////////////////////////////////////////////////////////////////////

void DK_InitJobs(void) {
    int player, job_index;
    for (player = 0; player < DK_PLAYER_COUNT; ++player) {
        for (job_index = jobs_count[player] - 1; job_index >= 0; --job_index) {
            free(jobs[player][job_index]);
        }
        jobs_count[player] = 0;
    }
}

void DK_FindJobs(DK_Player player, unsigned short x, unsigned short y) {
    DK_Block* block = DK_GetBlockAt(x, y);
    int i;
    for (i = jobs_count[player]; i > 0; --i) {
        const DK_Job* job = jobs[player][i - 1];
        if (job->block == block) {
            // Remove this one. Notify worker that it's no longer needed.
            if (job->worker) {
                DK_CancelJob(job->worker);
            }

            // Free memory.
            delete_job(player, i - 1);
        }
    }

    // Update / recreate jobs.
    jobs_add(player, x, y);
    if (x > 0) {
        jobs_add(player, x - 1, y);
    }
    if (x < DK_GetMapSize() - 1) {
        jobs_add(player, x + 1, y);
    }
    if (y > 0) {
        jobs_add(player, x, y - 1);
    }
    if (y < DK_GetMapSize() - 1) {
        jobs_add(player, x, y + 1);
    }
}

void DK_RenderJobs(void) {
    if (DK_d_draw_jobs) {
        unsigned int i;
        for (i = 0; i < jobs_count[DK_PLAYER_ONE]; ++i) {
            const DK_Job* job = jobs[DK_PLAYER_ONE][i];

            if (job->worker) {
                if (job->type == DK_JOB_DIG) {
                    glColor3f(0.4f, 0.8f, 0.4f);
                } else {
                    glColor3f(0.4f, 0.4f, 0.8f);
                }
            } else {
                glColor3f(0.4f, 0.4f, 0.4f);
            }
            glDisable(GL_LIGHTING);

            glBegin(GL_QUADS);
            {
                glVertex3f((job->position.v[0] - 0.1f) * DK_BLOCK_SIZE, (job->position.v[1] - 0.1f) * DK_BLOCK_SIZE, DK_D_DRAW_PATH_HEIGHT + 0.1f);
                glVertex3f((job->position.v[0] + 0.1f) * DK_BLOCK_SIZE, (job->position.v[1] - 0.1f) * DK_BLOCK_SIZE, DK_D_DRAW_PATH_HEIGHT + 0.1f);
                glVertex3f((job->position.v[0] + 0.1f) * DK_BLOCK_SIZE, (job->position.v[1] + 0.1f) * DK_BLOCK_SIZE, DK_D_DRAW_PATH_HEIGHT + 0.1f);
                glVertex3f((job->position.v[0] - 0.1f) * DK_BLOCK_SIZE, (job->position.v[1] + 0.1f) * DK_BLOCK_SIZE, DK_D_DRAW_PATH_HEIGHT + 0.1f);
            }
            glEnd();

            glEnable(GL_LIGHTING);
        }
    }
}

DK_Job** DK_GetJobs(DK_Player player, unsigned int* count) {
    *count = jobs_count[player];
    return jobs[player];
}
