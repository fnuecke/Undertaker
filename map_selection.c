#include <malloc.h>

#include "map.h"
#include "players.h"
#include "config.h"

// -1 because NONE cannot select (white can!).
// this is a bitset, where each bit represents a block in the map.
char* selection[DK_PLAYER_COUNT - 1] = {0};

void init_selection() {
    int i;
    for (i = 0; i < DK_PLAYER_COUNT - 1; ++i) {
        free(selection[i]);
        selection[i] = calloc((DK_map_size * DK_map_size) / 8 + 1, sizeof (char));
    }
}

void DK_block_select(DK_Player player, unsigned int x, unsigned int y) {
    const DK_Block* block = DK_block_at(x, y);
    if (block->type != DK_BLOCK_DIRT ||
            (block->owner != player && block->owner != DK_PLAYER_NONE)) {
        return;
    }
    const unsigned int idx = y * DK_map_size + x;
    selection[player][idx >> 3] |= (1 << (idx & 7));
}

void DK_block_deselect(DK_Player player, unsigned int x, unsigned int y) {
    const unsigned int idx = y * DK_map_size + x;
    selection[player][idx >> 3] &= ~(1 << (idx & 7));
}

int DK_block_selected(DK_Player player, unsigned int x, unsigned int y) {
    const unsigned int idx = y * DK_map_size + x;
    return (selection[player][idx >> 3] & (1 << (idx & 7))) != 0;
}
