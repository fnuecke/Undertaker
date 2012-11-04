#include <assert.h>

#include "block.h"
#include "room.h"
#include "unit_type.h"

MP_Passability MP_GetBlockPassability(const MP_Block* block) {
    assert(block);
    assert(block->type);
    assert(!block->room || block->room->type);

    return block->room ? block->room->type->passability : block->type->passability;
}

MP_BlockLevel MP_GetBlockLevel(const MP_Block* block) {
    assert(block);
    assert(block->type);
    assert(!block->room || block->room->type);

    return block->room ? block->room->type->level : block->type->level;
}

bool MP_IsBlockPassable(const MP_Block* block) {
    return MP_GetBlockPassability(block) > 0;
}

bool MP_IsBlockPassableBy(const MP_Block* block, const MP_UnitType* unitType) {
    assert(block);
    assert(unitType);

    return (MP_GetBlockPassability(block) & unitType->canPass) != 0;
}

bool MP_IsBlockDestructible(const MP_Block* block) {
    assert(block);
    assert(block->type);

    return block->type->durability > 0;
}

bool MP_IsBlockConvertible(const MP_Block* block) {
    assert(block);
    assert(block->type);

    return block->type->strength > 0;
}
