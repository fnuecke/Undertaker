#include <assert.h>

#include "block.h"
#include "events.h"
#include "map.h"
#include "room.h"
#include "unit_type.h"

MP_Passability MP_GetBlockPassability(const MP_Block* block) {
    assert(block);
    assert(block->type);

    return block->roomNode ? MP_GetRoomType(block->roomNode->room)->passability : block->type->passability;
}

MP_BlockLevel MP_GetBlockLevel(const MP_Block* block) {
    assert(block);
    assert(block->type);

    return block->roomNode ? MP_GetRoomType(block->roomNode->room)->level : block->type->level;
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

bool MP_DamageBlock(MP_Block* block, unsigned int damage) {
    assert(block);

    // Already destroyed (nothing to do)?
    if (block->durability <= 0) {
        return true;
    }

    // Check if this is the final blow.
    if (block->durability > damage) {
        block->durability -= damage;
        return false;
    }

    // Block is destroyed.
    block->player = MP_PLAYER_NONE;
    MP_SetBlockType(block, block->type->becomes);

    return true;
}

bool MP_ConvertBlock(MP_Block* block, MP_Player player, float strength) {
    assert(block);
    assert(player > MP_PLAYER_NONE && player < MP_PLAYER_COUNT);

    // First reduce any enemy influence.
    if (block->player != player) {
        // Not this player's, reduce strength.
        block->strength -= strength;
        if (block->strength > 0) {
            return false;
        }

        // Block is completely converted.
        MP_SetBlockOwner(block, player);
    } else {
        // Owned by this player, repair it.
        block->strength += strength;
        if (block->strength < block->type->strength) {
            return false;
        }

        // Completely repaired.
        block->strength = block->type->strength;
    }

    return true;
}

void MP_SetBlockType(MP_Block* block, const MP_BlockType* type) {
    assert(block);
    assert(type);

    // Remove any room on the block if it changes its type.
    if (block->roomNode) {
        MP_SetRoom(block, NULL);
    }

    block->type = type;
    block->durability = block->type->durability;
    block->strength = block->type->strength;
    block->gold = block->type->gold;

    MP_DispatchBlockTypeChangedEvent(block);
}

/** Check for tiles to automatically convert after a block conversion */
static void autoConvert(MP_Block* block, MP_Player player) {
    // Get the actual coordinates.
    unsigned short x, y;
    MP_GetBlockCoordinates(block, &x, &y);

    // See if we're now cornering a neutral block. In that case we
    // automatically convert that one, too.
    //
    // +---+---+---...
    // | o | b |
    // +---+---+---...
    // | b |   |
    // +---+---+---...
    // ...
    //
    // We must be one of the three marked blocks, and the ones marked 'b'
    // must be walls, the one marked be 'o' must be an open tile. An that
    // in all possible orientations.
    if (block->type->level == MP_BLOCK_LEVEL_HIGH) {
        // We're one of the 'b' blocks (possibly). Check our surroundings.
        for (int nx = -1; nx < 2; nx += 2) {
            for (int ny = -1; ny < 2; ny += 2) {
                // This double loop gives us the coordinates of our diagonal
                // neighbors, meaning they have to be owned blocks.
                if ((block = MP_GetBlockAt(x + nx, y + ny)) &&
                    block->player == player &&
                    block->type->level == MP_BLOCK_LEVEL_HIGH &&
                    // OK so far, now wee need to check if one of the
                    // blocks completing the square is open and the
                    // other un-owned.
                    ((
                    (block = MP_GetBlockAt(x, y + ny)) &&
                    block->player == player &&
                    block->type->level < MP_BLOCK_LEVEL_HIGH &&
                    (block = MP_GetBlockAt(x + nx, y)) &&
                    block->player == MP_PLAYER_NONE &&
                    block->type->level == MP_BLOCK_LEVEL_HIGH &&
                    MP_IsBlockConvertible(block)
                    ) || (
                    // If it didn't work that way around, check the
                    // other way.
                    (block = MP_GetBlockAt(x + nx, y)) &&
                    block->player == player &&
                    block->type->level < MP_BLOCK_LEVEL_HIGH &&
                    (block = MP_GetBlockAt(x, y + ny)) &&
                    block->player == MP_PLAYER_NONE &&
                    block->type->level == MP_BLOCK_LEVEL_HIGH &&
                    MP_IsBlockConvertible(block)
                    ))) {
                    // All conditions are met for converting the block.
                    MP_SetBlockOwner(block, player);
                }
            }
        }
    } else {
        // We're on a lower level tile, check the diagonal for un-owned high
        // blocks, and the inbetweens for owned high blocks.
        for (int nx = -1; nx < 2; nx += 2) {
            for (int ny = -1; ny < 2; ny += 2) {
                // This double loop gives us the coordinates of our diagonal
                // neighbors, meaning they have to be un-owned blocks.
                if ((block = MP_GetBlockAt(x + nx, y + ny)) &&
                    block->player == MP_PLAYER_NONE &&
                    block->type->level == MP_BLOCK_LEVEL_HIGH &&
                    // OK so far, now wee need to check if the blocks
                    // completing the square are high and owned.
                    (block = MP_GetBlockAt(x, y + ny)) &&
                    block->player == player &&
                    block->type->level == MP_BLOCK_LEVEL_HIGH &&
                    (block = MP_GetBlockAt(x + nx, y)) &&
                    block->player == player &&
                    block->type->level == MP_BLOCK_LEVEL_HIGH) {
                    // All conditions are met for converting the block.
                    MP_SetBlockOwner(block, player);
                }
            }
        }
    }
}

void MP_SetBlockOwner(MP_Block* block, MP_Player player) {
    assert(block);
    assert(player < MP_PLAYER_COUNT);

    block->player = player;
    block->durability = block->type->durability;
    block->strength = block->type->strength;

    MP_DispatchBlockOwnerChangedEvent(block);

    autoConvert(block, player);
}
