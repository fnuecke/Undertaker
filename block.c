#include "block.h"
#include "room.h"
#include "units.h"

DK_Passability DK_GetBlockPassability(const DK_Block* block) {
    return block && (block->room ? block->room->meta->passability : block->meta->passability);
}

DK_BlockLevel DK_GetBlockLevel(const DK_Block* block) {
    return block && (block->room ? block->room->meta->level : block->meta->level);
}

bool DK_IsBlockPassable(const DK_Block* block) {
    return DK_GetBlockPassability(block) > 0;
}

bool DK_IsBlockPassableBy(const DK_Block* block, const DK_Unit* unit) {
    return unit && (DK_GetBlockPassability(block) & unit->meta->passability) != 0;
}

bool DK_IsBlockDestructible(const DK_Block* block) {
    return block && block->meta->durability > 0;
}

bool DK_IsBlockConvertible(const DK_Block* block) {
    return block && block->meta->strength > 0;
}
