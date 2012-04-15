/* 
 * File:   config.h
 * Author: fnuecke
 *
 * Created on April 14, 2012, 9:46 AM
 */

#ifndef CONFIG_H
#define	CONFIG_H

#include <GL/gl.h>

///////////////////////////////////////////////////////////////////////////////
// Display
///////////////////////////////////////////////////////////////////////////////

/** Screen resolution along the x axis */
#define DK_RESOLUTION_X 1280

/** Screen resolution along the y axis */
#define DK_RESOLUTION_Y 1024

/** The screen aspect ratio */
#define DK_ASPECT_RATIO ((float)DK_RESOLUTION_X / (float)DK_RESOLUTION_Y)

/** Whether to use anti aliasing or not */
#define DK_USE_ANTIALIASING 0

///////////////////////////////////////////////////////////////////////////////
// Textures
///////////////////////////////////////////////////////////////////////////////

/** Maximum number of variations for a single texture we support */
#define DK_TEX_MAX_VARIATIONS 8

/** Base directory for textures */
#define DK_TEX_DIR "data/textures/"

/** File extension used for textures */
#define DK_TEX_FILETYPE ".png"

///////////////////////////////////////////////////////////////////////////////
// Map rendering
///////////////////////////////////////////////////////////////////////////////

/** Size of a block in openGL units */
#define DK_BLOCK_SIZE 16

/** Height of a block in openGL units */
#define DK_BLOCK_HEIGHT 24

/** Depth of water */
#define DK_WATER_LEVEL 4

/** The maximum amount a vertex is offset based on neighboring empty blocks */
#define DK_BLOCK_MAX_NOISE_OFFSET (DK_BLOCK_SIZE * 0.5)

/** Multiplier applied to vertices reducing above offset based on nearby owned blocks */
#define DK_OWNED_NOISE_REDUCTION 0.8

/** The size of the squared area of blocks to render based on the camera position */
#define DK_RENDER_AREA 20

/** Y offset of tiles to render (due to camera looking slightly forward) */
#define DK_RENDER_OFFSET 4

///////////////////////////////////////////////////////////////////////////////
// Camera
///////////////////////////////////////////////////////////////////////////////

/** Slowing factor when stopping camera movement */
#define DK_CAMERA_FRICTION 0.8f

/** Scroll speed of the camera */
#define DK_CAMERA_SPEED 4

/** Default distance of the camera from the ground */
#define DK_CAMERA_HEIGHT DK_BLOCK_SIZE * 6

/** Distance of the look-at target to the camera */
#define DK_CAMERA_TARGET_DISTANCE DK_BLOCK_SIZE * 2

/** How far to zoom in (Z axis) for one zoom step */
#define DK_CAMERA_ZOOM_STEP DK_BLOCK_SIZE

/** Maximum camera zoom amount */
#define DK_CAMERA_MAX_ZOOM 4 * DK_CAMERA_ZOOM_STEP

///////////////////////////////////////////////////////////////////////////////
// Debugging
///////////////////////////////////////////////////////////////////////////////

#define DK_D_DRAW_NORMALS 0
#define DK_D_DRAW_TEST_TEXTURE 0
#define DK_D_COMPUTE_NORMALS 1
#define DK_D_TERRAIN_NOISE 1
#define DK_D_FACTOR_NOISE 1
#define DK_D_DRAW_NOISE_FACTOR 0
#define DK_D_CACHE_NOISE 0

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

#endif	/* CONFIG_H */

