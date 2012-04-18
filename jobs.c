#include <assert.h>
#include <malloc.h>
#include <string.h>

#include "jobs.h"
#include "selection.h"
#include "config.h"

/** List of all current jobs, per player */
static DK_Job* jobs[DK_PLAYER_COUNT] = {0};

/** Capacity of the workplace lists, and actual load */
static unsigned int jobs_capacity[DK_PLAYER_COUNT] = {0};
static unsigned int jobs_count[DK_PLAYER_COUNT] = {0};

static DK_Job* get_job(DK_Player player) {
    if (jobs_count[player] + 1 > jobs_capacity[player]) {
        jobs_capacity[player] = jobs_capacity[player] * 2 + 1;
        jobs[player] = realloc(jobs[player], jobs_capacity[player] * sizeof (DK_Job));
    }
    return &jobs[player][jobs_count[player]++];
}

static void delete_job(DK_Player player, int idx) {
    memmove(&jobs[player][idx], &jobs[player][idx + 1], (--jobs_count[player] - idx) * sizeof (DK_Job));
}

void DK_init_jobs() {
    int i;
    for (i = 0; i < DK_PLAYER_COUNT; ++i) {
        jobs_count[i] = 0;
    }
}

static inline int owns_adjacent(DK_Player player, unsigned int x, unsigned int y) {
    DK_Block* block;
    return ((block = DK_block_at(x - 1, y)) && block->owner == player) ||
            ((block = DK_block_at(x + 1, y)) && block->owner == player) ||
            ((block = DK_block_at(x, y - 1)) && block->owner == player) ||
            ((block = DK_block_at(x, y + 1)) && block->owner == player);
}

void DK_jobs_create(DK_Player player, unsigned short x, unsigned short y) {
    if (DK_block_is_selectable(player, x, y)) {
        // Valid block for working on.
        if (DK_block_is_selected(player, x, y)) {
            // It's selected, start digging.
            // Check if a neighboring tile is passable.
            DK_Block* block;
            if ((block = DK_block_at(x - 1, y)) && DK_block_is_passable(block)) {
                // Left is valid.
                DK_Job* job = get_job(player);
                job->block = DK_block_at(x, y);
                job->x = (x - 0.25f);
                job->y = (y + 0.5f);
                job->type = DK_JOB_DIG;
                job->worker = 0;
            }
            if ((block = DK_block_at(x + 1, y)) && DK_block_is_passable(block)) {
                // Right is valid.
                DK_Job* job = get_job(player);
                job->block = DK_block_at(x, y);
                job->x = (x + 1.25f);
                job->y = (y + 0.5f);
                job->type = DK_JOB_DIG;
                job->worker = 0;
            }
            if ((block = DK_block_at(x, y - 1)) && DK_block_is_passable(block)) {
                // Top is valid.
                DK_Job* job = get_job(player);
                job->block = DK_block_at(x, y);
                job->x = (x + 0.5f);
                job->y = (y - 0.25f);
                job->type = DK_JOB_DIG;
                job->worker = 0;
            }
            if ((block = DK_block_at(x, y + 1)) && DK_block_is_passable(block)) {
                // Bottom is valid.
                DK_Job* job = get_job(player);
                job->block = DK_block_at(x, y);
                job->x = (x + 0.5f);
                job->y = (y + 1.25f);
                job->type = DK_JOB_DIG;
                job->worker = 0;
            }
        } else if (DK_block_at(x, y)->owner == DK_PLAYER_NONE) {
            // Not selected, and not owned, start conquering.
            // Check if a neighboring tile is owned by the same player.
            DK_Block* block;
            if ((block = DK_block_at(x - 1, y)) && block->owner == player) {
                // Left is valid.
                DK_Job* job = get_job(player);
                job->block = DK_block_at(x, y);
                job->x = (x - 0.25f);
                job->y = (y + 0.5f);
                job->type = DK_JOB_CONVERT;
                job->worker = 0;
            }
            if ((block = DK_block_at(x + 1, y)) && block->owner == player) {
                // Right is valid.
                DK_Job* job = get_job(player);
                job->block = DK_block_at(x, y);
                job->x = (x + 1.25f);
                job->y = (y + 0.5f);
                job->type = DK_JOB_CONVERT;
                job->worker = 0;
            }
            if ((block = DK_block_at(x, y - 1)) && block->owner == player) {
                // Top is valid.
                DK_Job* job = get_job(player);
                job->block = DK_block_at(x, y);
                job->x = (x + 0.5f);
                job->y = (y - 0.25f);
                job->type = DK_JOB_CONVERT;
                job->worker = 0;
            }
            if ((block = DK_block_at(x, y + 1)) && block->owner == player) {
                // Bottom is valid.
                DK_Job* job = get_job(player);
                job->block = DK_block_at(x, y);
                job->x = (x + 0.5f);
                job->y = (y - 0.25f);
                job->type = DK_JOB_CONVERT;
                job->worker = 0;
            }
        }
    } else if (DK_block_at(x, y)->type == DK_BLOCK_NONE &&
            DK_block_at(x, y)->owner != player) {
        // Block is empty tile and not already owned.
        // Check if a neighboring tile is owned by the same player.
        if (owns_adjacent(player, x, y)) {
            DK_Job* job = get_job(player);
            job->block = DK_block_at(x, y);
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
        if (jobs[player][i - 1].block == block) {
            // Remove this one.
            delete_job(player, i - 1);
        }
    }
}

DK_Job* DK_jobs(DK_Player player, unsigned int* count) {
    *count = jobs_count[player];
    return jobs[player];
}
