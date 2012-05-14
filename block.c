#include "block.h"

#include "room.h"
#include "unit.h"
#include "map.h"

MP_Passability MP_GetBlockPassability(const MP_Block* block) {
    return block && block->meta && (block->room ? block->room->meta->passability : block->meta->passability);
}

MP_BlockLevel MP_GetBlockLevel(const MP_Block* block) {
    return block && block->meta && (block->room ? block->room->meta->level : block->meta->level);
}

bool MP_IsBlockPassable(const MP_Block* block) {
    return MP_GetBlockPassability(block) > 0;
}

bool MP_IsBlockPassableBy(const MP_Block* block, const MP_UnitMeta* meta) {
    return meta && (MP_GetBlockPassability(block) & meta->canPass) != 0;
}

bool MP_IsBlockDestructible(const MP_Block* block) {
    return block && block->meta && block->meta->durability > 0;
}

bool MP_IsBlockConvertible(const MP_Block* block) {
    return block && block->meta && block->meta->strength > 0;
}
