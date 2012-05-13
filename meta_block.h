/* 
 * File:   meta_block.h
 * Author: fnuecke
 *
 * Created on May 5, 2012, 5:15 PM
 */

#ifndef META_BLOCK_H
#define	META_BLOCK_H

#include "meta.h"
#include "types.h"

#ifdef	__cplusplus
extern "C" {
#endif

    struct MP_BlockMeta {
        /** The ID of this block type */
        unsigned int id;

        /** The name of this block type */
        const char* name;

        /** The level (height) at which to render this block type */
        MP_BlockLevel level;

        /** The required passability flag to traverse this block type */
        MP_Passability passability;

        /** Frequency at which lights are attached to a wall; double if owned */
        unsigned int lightFrequency;

        /** Durability of this block (digging resistance) */
        unsigned int durability;

        /** Strength of this block (conversion resistance) */
        unsigned int strength;

        /** The amount of gold this block holds */
        unsigned int gold;

        /** The type of block this one becomes upon destruction */
        const MP_BlockMeta* becomes;

        /** IDs of textures to use for rendering at different levels */
        MP_TextureID texturesTop[MP_BLOCK_TEXTURE_TOP_COUNT];
        MP_TextureID texturesSide[MP_BLOCK_LEVEL_COUNT][MP_BLOCK_TEXTURE_SIDE_COUNT];
    };

    META_header(MP_BlockMeta, Block);

#ifdef	__cplusplus
}
#endif

#endif	/* META_BLOCK_H */
