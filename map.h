/* 
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
    unsigned short MP_GetMapSize(void);

    /**
     * Get the map block at the specified coordinate.
     */
    MP_Block* MP_GetBlockAt(int x, int y);

    /**
     * Get the coordinates of the specified block.
     */
    bool MP_GetBlockCoordinates(unsigned short* x, unsigned short* y, const MP_Block* block);

    /**
     * Gets the block currently hovered by the mouse.
     */
    MP_Block* MP_GetBlockUnderCursor(void);

    /**
     * Gets the coordinates of the block currently hovered by the mouse.
     */
    void MP_GetBlockCoordinatesUnderCursor(int* x, int* y);

    /**
     * Gets the depth (distance to camera) of the block currently under the
     * cursor.
     */
    float MP_GetBlockDepthUnderCursor(void);

    ///////////////////////////////////////////////////////////////////////////
    // Modifiers
    ///////////////////////////////////////////////////////////////////////////

    /**
     * Change the size of the map data structures. This will trigger a resize
     * event, which should invalidate all other map size dependent data (and
     * generally means that the old map has been unloaded -> cleanup).
     */
    void MP_SetMapSize(unsigned short size, const MP_BlockType* fillWith);

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
    bool MP_ConvertBlock(MP_Block* block, unsigned int strength, MP_Player player);

    ///////////////////////////////////////////////////////////////////////////
    // Initialization / Events
    ///////////////////////////////////////////////////////////////////////////

    /**
     * Generates OpenGL resources used by the map.
     */
    void MP_GL_GenerateMap(void);

    /**
     * Delete OpenGL resources used by the map.
     */
    void MP_GL_DeleteMap(void);

    /**
     * Initialize map related event logic.
     */
    void MP_InitMap(void);

#ifdef	__cplusplus
}
#endif

#endif
