/* 
 * File:   map.h
 * Author: fnuecke
 *
 * Created on April 14, 2012, 9:44 AM
 */

#ifndef MAP_H
#define	MAP_H

#include "types.h"

#ifdef	__cplusplus
extern "C" {
#endif

    ///////////////////////////////////////////////////////////////////////////
    // Accessors
    ///////////////////////////////////////////////////////////////////////////

    /**
     * Size (width and height, maps are always squared) of the current map.
     */
    unsigned short DK_GetMapSize(void);

    /**
     * Get the map block at the specified coordinate.
     */
    DK_Block* DK_GetBlockAt(int x, int y);

    /**
     * Get the coordinates of the specified block.
     */
    int DK_GetBlockCoordinates(unsigned short* x, unsigned short* y, const DK_Block* block);

    /**
     * Gets the block currently hovered by the mouse, and its coordinates.
     */
    DK_Block* DK_GetBlockUnderCursor(int* x, int* y);

    ///////////////////////////////////////////////////////////////////////////
    // Modifiers
    ///////////////////////////////////////////////////////////////////////////

    /**
     * Change the size of the map data structures. This will trigger a resize
     * event, which should invalidate all other map size dependent data (and
     * generally means that the old map has been unloaded -> cleanup).
     */
    void DK_SetMapSize(unsigned short size,  const DK_BlockMeta* fillWith);

    /**
     * Change the type of a block.
     */
    void DK_SetBlockMeta(DK_Block* block, const DK_BlockMeta* meta);

    /**
     * Change the owner of a block.
     */
    void DK_SetBlockOwner(DK_Block* block, DK_Player player);

    /**
     * Apply damage to a block (dirt, gold or gem); return 1 if destroyed.
     */
    int DK_DamageBlock(DK_Block* block, unsigned int damage);

    /**
     * Apply conversion to a block (dirt, wall, empty); return 1 if successful.
     */
    int DK_ConvertBlock(DK_Block* block, unsigned int strength, DK_Player player);

    ///////////////////////////////////////////////////////////////////////////
    // Initialization / Events
    ///////////////////////////////////////////////////////////////////////////

    /**
     * Generates OpenGL resources used by the map.
     */
    void DK_GL_GenerateMap(void);

    /**
     * Delete OpenGL resources used by the map.
     */
    void DK_GL_DeleteMap(void);

    /**
     * Initialize map related event logic.
     */
    void DK_InitMap(void);

#ifdef	__cplusplus
}
#endif

#endif	/* MAP_H */

