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
    struct MP_Block {
        /** Info on the block type */
        const MP_BlockMeta* meta;

        /** The type of room on this block */
        const MP_Room* room;

        /** The player owning the block */
        MP_Player owner;

        /** The actual, current durability of the block */
        float durability;

        /** The actual, current strength of the block */
        float strength;

        /** The remaining gold in this block */
        unsigned int gold;
    };

    MP_Passability MP_GetBlockPassability(const MP_Block* block);

    MP_BlockLevel MP_GetBlockLevel(const MP_Block* block);

    bool MP_IsBlockPassable(const MP_Block* block);

    bool MP_IsBlockPassableBy(const MP_Block* block, const MP_Unit* unit);

    bool MP_IsBlockDestructible(const MP_Block* block);

    bool MP_IsBlockConvertible(const MP_Block* block);

#ifdef	__cplusplus
}
#endif

#endif	/* BLOCK_H */

