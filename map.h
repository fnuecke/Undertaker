/* 
 * File:   map.h
 * Author: fnuecke
 *
 * Created on April 14, 2012, 9:44 AM
 */

#ifndef MAP_H
#define	MAP_H

#include <GL/gl.h>

#include "players.h"

/** Possible block types */
typedef enum {
    DK_BLOCK_NONE,
    DK_BLOCK_WATER,
    DK_BLOCK_LAVA,
    DK_BLOCK_DIRT,
    DK_BLOCK_GOLD,
    DK_BLOCK_GEM,
    DK_BLOCK_ROCK
} DK_BlockType;

/** Possible rooms */
typedef enum {
    DK_ROOM_NONE,
    DK_ROOM_HEART,
    DK_ROOM_ENTRANCE,
    DK_ROOM_DORMITORY,
    DK_ROOM_FARM,
    DK_ROOM_LIBRARY,
    DK_ROOM_TRAINING
} DK_RoomType;

/** A single block (cell) of a map */
typedef struct {
    /** The type of the block */
    DK_BlockType type;

    /** The player that owns the block */
    DK_Player owner;

    /** The room type, if any */
    DK_RoomType room;
    
    /** Multipurpose health of the block (hits to break down or convert ) */
    unsigned int health;
} DK_Block;

/** Size (width and height) of the current map */
unsigned short DK_map_size;

#ifdef	__cplusplus
extern "C" {
#endif

/** Clears the current map and initializes a new one with the specified size */
void DK_init_map(unsigned short size);

/** Get the map block at the specified coordinate */
DK_Block* DK_block_at(int x, int y);

/** Utility method for checking if a block contains a fluid */
int DK_block_is_fluid(const DK_Block* block);

/** Utility method for checking if a block is passable */
int DK_block_is_passable(const DK_Block* block);

/** Render the current map (blocks) */
void DK_render_map();

/** Gets the block currently hovered by the mouse, and its coordinates */
DK_Block* DK_block_under_cursor(int* block_x, int* block_y, int mouse_x, int mouse_y);

/** Checks if the specified block is selectable by the specified player */
int DK_block_is_selectable(DK_Player player, int x, int y);

/** Select the block at the given coordinates for the specified player */
void DK_block_select(DK_Player player, unsigned short x, unsigned short y);

/** Deselect the block at the given coordinates for the specified player */
void DK_block_deselect(DK_Player player, unsigned short x, unsigned short y);

/** Checks if the specified block is selected by the specified player */
int DK_block_is_selected(DK_Player player, unsigned short x, unsigned short y);

#ifdef	__cplusplus
}
#endif

#endif	/* MAP_H */

