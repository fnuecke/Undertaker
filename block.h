/* 
 * Author: fnuecke
 *
 * Created on May 2, 2012, 4:46 PM
 */

#ifndef BLOCK_H
#define	BLOCK_H

#include "block_type.h"

#ifdef	__cplusplus
extern "C" {
#endif

    ///////////////////////////////////////////////////////////////////////////
    // Types
    ///////////////////////////////////////////////////////////////////////////

    /** Description of a single block instance */
    struct MP_Block {
        /** Info on the block type */
        const MP_BlockType* type;

        /** The room node on this block */
        MP_RoomNode* roomNode;

        /** The player owning the block */
        MP_Player player;

        /** The actual, current durability of the block */
        float durability;

        /** The actual, current strength of the block */
        float strength;

        /** The remaining gold in this block */
        unsigned int gold;
    };

    ///////////////////////////////////////////////////////////////////////////
    // Accessors
    ///////////////////////////////////////////////////////////////////////////

    MP_Passability MP_GetBlockPassability(const MP_Block* block);

    MP_BlockLevel MP_GetBlockLevel(const MP_Block* block);

    bool MP_IsBlockPassable(const MP_Block* block);

    bool MP_IsBlockPassableBy(const MP_Block* block, const MP_UnitType* unitType);

    bool MP_IsBlockDestructible(const MP_Block* block);

    bool MP_IsBlockConvertible(const MP_Block* block);

    ///////////////////////////////////////////////////////////////////////////
    // Modifiers
    ///////////////////////////////////////////////////////////////////////////

    /**
     * Change the type of a block.
     */
    void MP_SetBlockType(MP_Block* block, const MP_BlockType* type);

    /**
     * Change the owner of a block.
     */
    void MP_SetBlockOwner(MP_Block* block, MP_Player player);

    /**
     * Apply damage to a block (dirt, gold or gem); return true if destroyed.
     */
    bool MP_DamageBlock(MP_Block* block, unsigned int damage);

    /**
     * Apply conversion to a block (dirt, wall, empty); return true if successful.
     */
    bool MP_ConvertBlock(MP_Block* block, MP_Player player, float strength);

#ifdef	__cplusplus
}
#endif

#endif
