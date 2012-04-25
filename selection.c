#include "bitset.h"
#include "jobs.h"
#include "map.h"
#include "players.h"
#include "selection.h"

/**
 * Selected blocks, per player.
 * This is a bitset, where each bit represents a block in the map.
 */
static BitSet selection[DK_PLAYER_COUNT] = {0};

/** Point where our selection started */
static int selection_x = 0, selection_y = 0;

/** Current selection mode */
static enum {
    MODE_NONE,
    MODE_SELECT,
    MODE_DESELECT
} mode = MODE_NONE;

inline static int block_index_valid(int x, int y) {
    return x >= 0 && y >= 0 && x < DK_map_size && y < DK_map_size;
}

void DK_init_selection(void) {
    int i;
    for (i = 0; i < DK_PLAYER_COUNT; ++i) {
        BS_Delete(selection[i]);
        selection[i] = BS_New(DK_map_size * DK_map_size);
    }
}

void DK_selection(int* start_x, int* start_y, int* end_x, int* end_y) {
    DK_block_under_cursor(end_x, end_y);
    if (mode == MODE_NONE) {
        *start_x = *end_x;
        *start_y = *end_y;
    } else {
        *start_x = selection_x;
        *start_y = selection_y;
        if (*start_x > *end_x) {
            int tmp = *end_x;
            *end_x = *start_x;
            *start_x = tmp;
        }
        if (*start_y > *end_y) {
            int tmp = *end_y;
            *end_y = *start_y;
            *start_y = tmp;
        }
    }
}

void DK_selection_begin(void) {
    DK_block_under_cursor(&selection_x, &selection_y);
    if (DK_block_is_selectable(DK_PLAYER_RED, selection_x, selection_y)) {
        // OK, if it's selectable, start selection.
        if (DK_block_is_selected(DK_PLAYER_RED, selection_x, selection_y)) {
            mode = MODE_DESELECT;
        } else {
            mode = MODE_SELECT;
        }
    } else {
        mode = MODE_NONE;
    }
}

int DK_selection_cancel(void) {
    if (mode != MODE_NONE) {
        // Reset mode.
        mode = MODE_NONE;
        return 1;
    }
    return 0;
}

void DK_selection_end(void) {
    if (mode != MODE_NONE) {
        // Find the block we released over.
        int x, y, end_x = selection_x, end_y = selection_y;
        DK_block_under_cursor(&end_x, &end_y);

        // Swap corners if necessary.
        if (end_x < selection_x) {
            const int tmp = end_x;
            end_x = selection_x;
            selection_x = tmp;
        }
        if (end_y < selection_y) {
            const int tmp = end_y;
            end_y = selection_y;
            selection_y = tmp;
        }

        // Set selection for the drawn area.
        for (x = selection_x; x <= end_x; ++x) {
            for (y = selection_y; y <= end_y; ++y) {
                if (mode == MODE_SELECT) {
                    DK_block_select(DK_PLAYER_RED, x, y);
                } else if (mode == MODE_DESELECT) {
                    DK_block_deselect(DK_PLAYER_RED, x, y);
                }
            }
        }

        // Reset mode.
        mode = MODE_NONE;
    }
}

int DK_block_is_selectable(DK_Player player, int x, int y) {
    const DK_Block* block = DK_block_at(x, y);
    return block &&
            (block->type == DK_BLOCK_DIRT ||
            block->type == DK_BLOCK_GOLD ||
            block->type == DK_BLOCK_GEM) &&
            (block->owner == DK_PLAYER_NONE || block->owner == player);
}

void DK_block_select(DK_Player player, unsigned short x, unsigned short y) {
    if (DK_block_is_selectable(player, x, y)) {
        const unsigned int idx = y * DK_map_size + x;
        BS_Set(selection[player], idx);

        DK_FindJobs(player, x, y);
    }
}

void DK_block_deselect(DK_Player player, unsigned short x, unsigned short y) {
    if (block_index_valid(x, y)) {
        const unsigned int idx = y * DK_map_size + x;
        BS_Unset(selection[player], idx);

        DK_FindJobs(player, x, y);
    }
}

int DK_block_is_selected(DK_Player player, unsigned short x, unsigned short y) {
    return BS_Test(selection[player], y * DK_map_size + x);
}
