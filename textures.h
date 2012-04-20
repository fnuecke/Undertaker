/* 
 * File:   textures.h
 * Author: fnuecke
 *
 * Created on April 14, 2012, 9:48 AM
 */

#ifndef TEXTURES_H
#define	TEXTURES_H

#include "GLee.h"

/** Textures used for rendering the map */
typedef enum {
    DK_TEX_DIRT_FLOOR,
    DK_TEX_DIRT_SIDE,
    DK_TEX_DIRT_TOP,
    DK_TEX_FLOOR,
    DK_TEX_FLUID_LAVA,
    DK_TEX_FLUID_SIDE,
    DK_TEX_FLUID_WATER,
    DK_TEX_OWNER_BLUE,
    DK_TEX_OWNER_GREEN,
    DK_TEX_OWNER_RED,
    DK_TEX_OWNER_WHITE,
    DK_TEX_OWNER_YELLOW,
    DK_TEX_ROCK_SIDE,
    DK_TEX_ROCK_TOP,
    DK_TEX_WALL_TOP_N,
    DK_TEX_WALL_TOP_NE,
    DK_TEX_WALL_TOP_NE_CORNER,
    DK_TEX_WALL_TOP_NES,
    DK_TEX_WALL_TOP_NESW,
    DK_TEX_WALL_TOP_NS,

    DK_TEX_COUNT /**< Number of textures, used internally */
} DK_Texture;

#ifdef	__cplusplus
extern "C" {
#endif

/** Loads textures into the textures array */
void DK_load_textures();

/** Initializes textures for openGL after video reset */
void DK_opengl_textures();

/** Get a specific texture; the variant is determined by the given hash */
GLuint DK_opengl_texture(DK_Texture texture, unsigned int hash);

#ifdef	__cplusplus
}
#endif

#endif	/* TEXTURES_H */

