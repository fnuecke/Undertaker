#include <assert.h>
#include <malloc.h>
#include <string.h>

#include "jobs.h"
#include "selection.h"
#include "config.h"

/** List of all current jobs, per player */
static DK_Job** jobs[DK_PLAYER_COUNT] = {0};

/** Capacity of the workplace lists, and actual load */
static unsigned int jobs_capacity[DK_PLAYER_COUNT] = {0};
static unsigned int jobs_count[DK_PLAYER_COUNT] = {0};

static DK_Job* get_job(DK_Player player) {
    if (jobs_count[player] + 1 > jobs_capacity[player]) {
        jobs_capacity[player] = jobs_capacity[player] * 2 + 1;
        jobs[player] = realloc(jobs[player], jobs_capacity[player] * sizeof (DK_Job*));
    }
    DK_Job* job = calloc(1, sizeof (DK_Job));
    jobs[player][jobs_count[player]++] = job;
    return job;
}

static void delete_job(DK_Player player, int idx) {
    free(jobs[player][idx]);
    memmove(&jobs[player][idx], &jobs[player][idx + 1], (--jobs_count[player] - idx) * sizeof (DK_Job*));
}

void DK_init_jobs() {
    int i;
    for (i = 0; i < DK_PLAYER_COUNT; ++i) {
        jobs_count[i] = 0;
    }
}

void DK_render_jobs() {
#if DK_D_DRAW_JOBS
    int i;
    for (i = 0; i < jobs_count[DK_PLAYER_RED]; ++i) {
        const DK_Job* job = jobs[DK_PLAYER_RED][i];

        if (job->worker) {
            glColor3f(0.4f, 0.8f, 0.4f);
        } else {
            glColor3f(0.4f, 0.4f, 0.4f);
        }
        glDisable(GL_LIGHTING);

        glBegin(GL_QUADS);
        {
            glVertex3f((job->x - 0.2f) * DK_BLOCK_SIZE, (job->y - 0.2f) * DK_BLOCK_SIZE, 0.75f);
            glVertex3f((job->x + 0.2f) * DK_BLOCK_SIZE, (job->y - 0.2f) * DK_BLOCK_SIZE, 0.75f);
            glVertex3f((job->x + 0.2f) * DK_BLOCK_SIZE, (job->y + 0.2f) * DK_BLOCK_SIZE, 0.75f);
            glVertex3f((job->x - 0.2f) * DK_BLOCK_SIZE, (job->y + 0.2f) * DK_BLOCK_SIZE, 0.75f);
        }
        glEnd();

        glEnable(GL_LIGHTING);
    }
#endif
}

static inline int owns_adjacent(DK_Player player, unsigned int x, unsigned int y) {
    DK_Block* block;
    return ((block = DK_block_at(x - 1, y)) && block->owner == player) ||
            ((block = DK_block_at(x + 1, y)) && block->owner == player) ||
            ((block = DK_block_at(x, y - 1)) && block->owner == player) ||
            ((block = DK_block_at(x, y + 1)) && block->owner == player);
}

static enum {
    SAME = 1,
    NORTH = 2,
    EAST = 4,
    SOUTH = 8,
    WEST = 16
} JobNeighbors;

void DK_jobs_create(DK_Player player, unsigned short x, unsigned short y) {
    DK_Block* block = DK_block_at(x, y);

    // Check which jobs are already taken care of. There are 5 slots: the block
    // itself (converting) and the four non-diagonal neighbors (dig/convert).
    char existing_jobs = 0, i;
    for (i = 0; i < jobs_count[player]; ++i) {
        const DK_Job* job = jobs[player][i];
        if (job->block != block) {
            continue;
        }
        if (job->x < x) {
            // Left.
            existing_jobs |= WEST;
        } else if (job->x > x + 1) {
            // Right.
            existing_jobs |= EAST;
        } else if (job->y < y) {
            // Top.
            existing_jobs |= NORTH;
        } else if (job->y > y + 1) {
            // Bottom.
            existing_jobs |= SOUTH;
        } else {
            // Same block.
            existing_jobs |= SAME;
        }
    }


    if (DK_block_is_selectable(player, x, y)) {
        // Valid block for working on.
        if (DK_block_is_selected(player, x, y)) {
            // It's selected, start digging.
            // Check if a neighboring tile is passable.
            DK_Block* b;
            if (!(existing_jobs & WEST) &&
                    (b = DK_block_at(x - 1, y)) &&
                    DK_block_is_passable(b)) {
                // Left is valid.
                DK_Job* job = get_job(player);
                job->block = block;
                job->x = (x - 0.25f);
                job->y = (y + 0.5f);
                job->type = DK_JOB_DIG;
                job->worker = 0;
            }
            if (!(existing_jobs & EAST) &&
                    (b = DK_block_at(x + 1, y)) &&
                    DK_block_is_passable(b)) {
                // Right is valid.
                DK_Job* job = get_job(player);
                job->block = block;
                job->x = (x + 1.25f);
                job->y = (y + 0.5f);
                job->type = DK_JOB_DIG;
                job->worker = 0;
            }
            if (!(existing_jobs & NORTH) &&
                    (b = DK_block_at(x, y - 1)) &&
                    DK_block_is_passable(b)) {
                // Top is valid.
                DK_Job* job = get_job(player);
                job->block = block;
                job->x = (x + 0.5f);
                job->y = (y - 0.25f);
                job->type = DK_JOB_DIG;
                job->worker = 0;
            }
            if (!(existing_jobs & SOUTH) &&
                    (b = DK_block_at(x, y + 1)) &&
                    DK_block_is_passable(b)) {
                // Bottom is valid.
                DK_Job* job = get_job(player);
                job->block = block;
                job->x = (x + 0.5f);
                job->y = (y + 1.25f);
                job->type = DK_JOB_DIG;
                job->worker = 0;
            }
        } else if (block->owner == DK_PLAYER_NONE) {
            // Not selected, and not owned, start conquering.
            // Check if a neighboring tile is owned by the same player.
            DK_Block* b;
            if (!(existing_jobs & WEST) &&
                    (b = DK_block_at(x - 1, y)) &&
                    DK_block_is_passable(b) &&
                    b->owner == player) {
                // Left is valid.
                DK_Job* job = get_job(player);
                job->block = block;
                job->x = (x - 0.25f);
                job->y = (y + 0.5f);
                job->type = DK_JOB_CONVERT;
                job->worker = 0;
            }
            if (!(existing_jobs & EAST) &&
                    (b = DK_block_at(x + 1, y)) &&
                    DK_block_is_passable(b) &&
                    b->owner == player) {
                // Right is valid.
                DK_Job* job = get_job(player);
                job->block = block;
                job->x = (x + 1.25f);
                job->y = (y + 0.5f);
                job->type = DK_JOB_CONVERT;
                job->worker = 0;
            }
            if (!(existing_jobs & NORTH) &&
                    (b = DK_block_at(x, y - 1)) &&
                    DK_block_is_passable(b) &&
                    b->owner == player) {
                // Top is valid.
                DK_Job* job = get_job(player);
                job->block = block;
                job->x = (x + 0.5f);
                job->y = (y - 0.25f);
                job->type = DK_JOB_CONVERT;
                job->worker = 0;
            }
            if (!(existing_jobs & SOUTH) &&
                    (b = DK_block_at(x, y + 1)) &&
                    DK_block_is_passable(b) &&
                    b->owner == player) {
                // Bottom is valid.
                DK_Job* job = get_job(player);
                job->block = block;
                job->x = (x + 0.5f);
                job->y = (y - 0.25f);
                job->type = DK_JOB_CONVERT;
                job->worker = 0;
            }
        }
    } else if (!(existing_jobs & SAME) &&
            block->type == DK_BLOCK_NONE &&
            block->owner != player) {
        // Block is empty tile and not already owned.
        // Check if a neighboring tile is owned by the same player.
        if (owns_adjacent(player, x, y)) {
            DK_Job* job = get_job(player);
            job->block = block;
            job->x = (x + 0.5f);
            job->y = (y + 0.5f);
            job->type = DK_JOB_CONVERT;
            job->worker = 0;
        }
    }
}

void DK_jobs_destroy(DK_Player player, unsigned short x, unsigned short y) {
    DK_Block* block = DK_block_at(x, y);
    int i;
    for (i = jobs_count[player]; i > 0; --i) {
        const DK_Job* job = jobs[player][i - 1];
        if (job->block == block) {
            // Remove this one. Notify worker that it's no longer needed.
            if (job->worker) {
                DK_unit_cancel_job(job->worker);
            }

            // Free memory.
            delete_job(player, i - 1);
        }
    }

    // Update / recreate jobs.
    DK_jobs_create(player, x, y);
    if (x > 0) {
        DK_jobs_create(player, x - 1, y);
    }
    if (x < DK_map_size - 1) {
        DK_jobs_create(player, x + 1, y);
    }
    if (y > 0) {
        DK_jobs_create(player, x, y - 1);
    }
    if (y < DK_map_size - 1) {
        DK_jobs_create(player, x, y + 1);
    }
}

DK_Job** DK_jobs(DK_Player player, unsigned int* count) {
    *count = jobs_count[player];
    return jobs[player];
}
