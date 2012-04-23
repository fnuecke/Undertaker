/* 
 * File:   map.h
 * Author: fnuecke
 *
 * Created on April 14, 2012, 9:44 AM
 */

#ifndef MAP_H
#define	MAP_H

#include "players.h"
#include "units.h"

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
    DK_ROOM_TRAINING,
    DK_ROOM_DOOR
} DK_RoomType;

/** A single block (cell) of a map */
typedef struct {
    /** The type of the block */
    DK_BlockType type;

    /** The player that owns the block */
    DK_Player owner;

    /** The room type, if any */
    DK_RoomType room;
    
    /** Health of the block (damage until break down) */
    unsigned int health;
    
    /** Strength of the block (damage until converted, per player) */
    unsigned int strength;
    
    /** Whether this block is 'closed', i.e. the owner own units cannot pass */
    int closed;
} DK_Block;

/** Size (width and height) of the current map */
unsigned short DK_map_size;

#ifdef	__cplusplus
extern "C" {
#endif

/** Clears the current map and initializes a new one with the specified size */
void DK_init_map(unsigned short size);

/** Update map data such as currently hovered block */
void DK_update_map(void);

/** Render the current map (blocks) */
void DK_render_map(void);

/** Load the current map from a file with the specified name */
void DK_map_load(const char* filename);

/** Save the current map to a file with the specified name */
void DK_map_save(const char* filename);

/** Get the map block at the specified coordinate */
DK_Block* DK_block_at(int x, int y);

/** Get the coordinates of the specified block */
int DK_block_coordinates(unsigned short* x, unsigned short* y, const DK_Block* block);

/** Test whether the specified block contains a fluid */
int DK_block_is_fluid(const DK_Block* block);

/** Test whether the specified block is a wall owned by the specified player */
int DK_block_is_wall(const DK_Block* block, DK_Player player);

/** Test whether the specified block is open, i.e. air */
int DK_block_is_open(const DK_Block* block);

/** Test whether the specified block is passable by the specified unit */
int DK_block_is_passable(const DK_Block* block, const DK_Unit* unit);

/** Gets the block currently hovered by the mouse, and its coordinates */
DK_Block* DK_block_under_cursor(int* block_x, int* block_y);

/** Apply damage to a block (dirt, gold or gem); return 1 if destroyed */
int DK_block_damage(DK_Block* block, unsigned int damage);

/** Apply conversion to a block (dirt, wall, empty); return 1 if successful */
int DK_block_convert(DK_Block* block, unsigned int strength, DK_Player player);

/** Change the type of a block */
void DK_block_set_type(DK_Block* block, DK_BlockType type);

/** Change the owner of a block */
void DK_block_set_owner(DK_Block* block, DK_Player player);

/** Update the map model for the block at the specified coordinates */
void DK_block_update(unsigned short x, unsigned short y);

#ifdef	__cplusplus
}
#endif

#endif	/* MAP_H */

