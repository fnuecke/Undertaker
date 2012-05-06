/* 
 * File:   block.h
 * Author: fnuecke
 *
 * Created on May 2, 2012, 4:46 PM
 */

#ifndef BLOCK_H
#define	BLOCK_H

#include "block_meta.h"

#ifdef	__cplusplus
extern "C" {
#endif

    /** Description of a single block instance */
    struct DK_Block {
        /** Info on the block type */
        const DK_BlockMeta* meta;

        /** The type of room on this block */
        const DK_Room* room;

        /** The player owning the block */
        DK_Player owner;

        /** The actual, current durability of the block */
        float durability;

        /** The actual, current strength of the block */
        float strength;

        /** The remaining gold in this block */
        unsigned int gold;
    };

    DK_Passability DK_GetBlockPassability(const DK_Block* block);

    DK_BlockLevel DK_GetBlockLevel(const DK_Block* block);

    bool DK_IsBlockPassable(const DK_Block* block);

    bool DK_IsBlockPassableBy(const DK_Block* block, const DK_Unit* unit);

    bool DK_IsBlockDestructible(const DK_Block* block);

    bool DK_IsBlockConvertible(const DK_Block* block);

#ifdef	__cplusplus
}
#endif

#endif	/* BLOCK_H */

