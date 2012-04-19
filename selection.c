#include "bitset.h"
#include "map.h"
#include "players.h"
#include "jobs.h"

/**
 * Selected blocks, per player.
 * This is a bitset, where each bit represents a block in the map.
 */
static char* selection[DK_PLAYER_COUNT] = {0};

inline static int block_index_valid(int x, int y) {
    return x >= 0 && y >= 0 && x < DK_map_size && y < DK_map_size;
}

void DK_init_selection() {
    int i;
    for (i = 0; i < DK_PLAYER_COUNT; ++i) {
        BS_free(selection[i]);
        selection[i] = BS_alloc(DK_map_size * DK_map_size);
    }
}

int DK_block_is_selectable(DK_Player player, int x, int y) {
    const DK_Block* block = DK_block_at(x, y);
    return block &&
            block->type != DK_BLOCK_ROCK &&
            !DK_block_is_passable(block) &&
            (block->owner == DK_PLAYER_NONE || block->owner == player);
}

void DK_block_select(DK_Player player, unsigned short x, unsigned short y) {
    if (DK_block_is_selectable(player, x, y)) {
        const unsigned int idx = y * DK_map_size + x;
        BS_set(selection[player], idx);

        DK_jobs_destroy(player, x, y);
    }
}

void DK_block_deselect(DK_Player player, unsigned short x, unsigned short y) {
    if (block_index_valid(x, y)) {
        const unsigned int idx = y * DK_map_size + x;
        BS_unset(selection[player], idx);

        DK_jobs_destroy(player, x, y);
    }
}

int DK_block_is_selected(DK_Player player, unsigned short x, unsigned short y) {
    return BS_test(selection[player], y * DK_map_size + x);
}
