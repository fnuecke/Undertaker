#include "block.h"

#include "room.h"
#include "unit.h"
#include "map.h"

MP_Passability MP_GetBlockPassability(const MP_Block* block) {
    return block && block->type && (block->room ? block->room->meta->passability : block->type->passability);
}

MP_BlockLevel MP_GetBlockLevel(const MP_Block* block) {
    return block && block->type && (block->room ? block->room->meta->level : block->type->level);
}

bool MP_IsBlockPassable(const MP_Block* block) {
    return MP_GetBlockPassability(block) > 0;
}

bool MP_IsBlockPassableBy(const MP_Block* block, const MP_UnitType* meta) {
    return meta && (MP_GetBlockPassability(block) & meta->canPass) != 0;
}

bool MP_IsBlockDestructible(const MP_Block* block) {
    return block && block->type && block->type->durability > 0;
}

bool MP_IsBlockConvertible(const MP_Block* block) {
    return block && block->type && block->type->strength > 0;
}
