/* 
 * File:   config.h
 * Author: fnuecke
 *
 * Created on April 14, 2012, 9:46 AM
 */

#ifndef CONFIG_H
#define	CONFIG_H

#include <GL/gl.h>

#define DK_RESOLUTION_X 1280
#define DK_RESOLUTION_Y 1024
#define DK_ASPECT_RATIO ((float)DK_RESOLUTION_X / (float)DK_RESOLUTION_Y)
#define DK_USE_ANTIALIASING 0

#define DK_UNIT_SCALING 1

#define DK_D_DRAW_NORMALS 0
#define DK_D_DRAW_TEST_TEXTURE 0
#define DK_D_COMPUTE_NORMALS 1
#define DK_D_TERRAIN_NOISE 1
#define DK_D_FACTOR_NOISE 1
#define DK_D_DRAW_NOISE_FACTOR 0
#define DK_D_CACHE_NOISE 0

/** Size of a block in openGL units */
#define DK_BLOCK_SIZE (DK_UNIT_SCALING * 16)

/** Height of a block in openGL units */
#define DK_BLOCK_HEIGHT (DK_UNIT_SCALING * 24)

/** Depth of water */
#define DK_WATER_LEVEL (DK_UNIT_SCALING * 4)

#define DK_BLOCK_MAX_NOISE_OFFSET (DK_BLOCK_SIZE * 0.5)
#define DK_OWNED_NOISE_REDUCTION 0.8

#define DK_CAMERA_FRICTION 0.8f
#define DK_CAMERA_SPEED (DK_UNIT_SCALING * 4)
#define DK_CAMERA_HEIGHT DK_BLOCK_SIZE * 6
#define DK_CAMERA_DISTANCE DK_BLOCK_SIZE * 2
#define DK_CAMERA_ZOOM_STEP DK_BLOCK_SIZE
#define DK_CAMERA_MAX_ZOOM 4 * DK_CAMERA_ZOOM_STEP

#define DK_RENDER_AREA 20
#define DK_RENDER_OFFSET 4

#define DK_LIGHT_AMBIENT_TOP 0.9f
#define DK_LIGHT_AMBIENT_NORTH 1.0f
#define DK_LIGHT_AMBIENT_SOUTH 0.2f
#define DK_LIGHT_AMBIENT_EAST 0.4f
#define DK_LIGHT_AMBIENT_WEST 0.7f

const GLfloat DK_global_ambient_top[4];
const GLfloat DK_global_ambient_north[4];
const GLfloat DK_global_ambient_south[4];
const GLfloat DK_global_ambient_east[4];
const GLfloat DK_global_ambient_west[4];

/** Maximum number of variations for a single texture we support */
#define DK_TEX_MAX_VARIATIONS 8

/** Base directory for textures */
#define DK_TEX_DIR "data/textures/"

/** File extension used for textures */
#define DK_TEX_FILETYPE ".png"

#endif	/* CONFIG_H */

