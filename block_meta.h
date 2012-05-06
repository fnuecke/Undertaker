/* 
 * File:   block_meta.h
 * Author: fnuecke
 *
 * Created on May 5, 2012, 5:15 PM
 */

#ifndef BLOCK_META_H
#define	BLOCK_META_H

#include "types.h"

#ifdef	__cplusplus
extern "C" {
#endif

    struct DK_BlockMeta {
        /** The ID of this block type */
        unsigned int id;

        /** The name of this block type */
        const char* name;

        /** The required passability flag to traverse this block type */
        DK_Passability passability;

        /** Durability of this block (digging resistance) */
        unsigned int durability;

        /** Strength of this block (conversion resistance) */
        unsigned int strength;

        /** The amount of gold this block holds */
        unsigned int gold;

        /** The type of block this one becomes upon destruction */
        DK_BlockMeta* becomes;

        /** The level (height) at which to render this block type */
        DK_BlockLevel level;

        /** IDs of textures to use for rendering at different levels */
        DK_TextureID textures[DK_BLOCK_LEVEL_COUNT][DK_BLOCK_TEXTURE_COUNT];
    };

    const DK_BlockMeta* DK_GetBlockMeta(unsigned int id);

    const DK_BlockMeta* DK_GetBlockMetaByName(const char* name);

    void DK_AddBlockMeta(const DK_BlockMeta* meta);

    void DK_InitBlockMeta(void);

#ifdef	__cplusplus
}
#endif

#endif	/* BLOCK_META_H */

