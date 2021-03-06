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
    void MP_GetBlockCoordinates(const MP_Block* block, unsigned short* x, unsigned short* y);

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
