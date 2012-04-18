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
#define DK_USE_ANTIALIASING 1

///////////////////////////////////////////////////////////////////////////////
// Units
///////////////////////////////////////////////////////////////////////////////

/** Maximum number of units a single player have at a time */
#define DK_UNITS_MAX_PER_PLAYER 100

/**
 * How fine the overlayed block grid for A* is (higher = finer); also sets how
 * many imps can work on one wall at a time */
#define DK_ASTAR_GRANULARITY 3

/**
 * Use jumps point search, skipping distances where possible.
 * D. Harabor and A. Grastien. Online Graph Pruning for Pathfinding on Grid Maps.
 * In National Conference on Artificial Intelligence (AAAI), 2011. */
#define DK_ASTAR_JPS 1

///////////////////////////////////////////////////////////////////////////////
// AI
///////////////////////////////////////////////////////////////////////////////

/** Delay in updates to wait before doing an actual update when idling */
#define DK_AI_IDLE_DELAY 16

/** Maximum number of stacked jobs */
#define DK_AI_JOB_STACK_MAX 8

/** Maximum path length a unit can keep track of */
#define DK_AI_PATH_MAX 32

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
#define DK_CAMERA_ZOOM_STEP (1.0f / 3.0f)

/** Maximum camera zoom amount */
#define DK_CAMERA_MAX_ZOOM 4 * DK_BLOCK_SIZE

///////////////////////////////////////////////////////////////////////////////
// Debugging
///////////////////////////////////////////////////////////////////////////////

/** Use test texture instead of actual textures */
#define DK_D_DRAW_TEST_TEXTURE 0

/** Use terrain noise */
#define DK_D_TERRAIN_NOISE 1

/** Factor in surroundings for terrain noise (empty blocks, owned blocks) */
#define DK_D_USE_NOISE_OFFSET 1

/** Highlight vertices that use offsetting due to nearby blocks */
#define DK_D_DRAW_NOISE_FACTOR 0

/** Cache the global noise instead of recomputing it on the fly */
#define DK_D_CACHE_NOISE 1

/** Render unit paths */
#define DK_D_DRAW_PATHS 1

/** Height at which to render paths */
#define DK_D_PATH_HEIGHT 1

#endif	/* CONFIG_H */

