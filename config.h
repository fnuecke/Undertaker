/* 
 * File:   config.h
 * Author: fnuecke
 *
 * Created on April 14, 2012, 9:46 AM
 */

#ifndef CONFIG_H
#define	CONFIG_H

#include "GLee.h"

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

/** Target framerate */
#define DK_FRAMERATE 60

/** Use fog in the distance, to fade out to black */
#define DK_USE_FOG 1

///////////////////////////////////////////////////////////////////////////////
// Units
///////////////////////////////////////////////////////////////////////////////

/** Maximum number of units a single player have at a time */
#define DK_UNITS_MAX_PER_PLAYER 100

/** Maximum number of abilities a unit may have (number of cooldowns) */
#define DK_UNITS_MAX_ABILITIES 4

///////////////////////////////////////////////////////////////////////////////
// Job priorities
///////////////////////////////////////////////////////////////////////////////
// These are linear to the distance to the job: rank = dist * prio

/** Priority for digging up stuff */
#define DK_JOB_DIG_PRIORITY 0

/** Priority for converting floor tiles */
#define DK_JOB_CONVERT_FLOOR_PRIORITY DK_BLOCK_SIZE * 8

/** Priority for converting walls */
#define DK_JOB_CONVERT_WALL_PRIORITY DK_BLOCK_SIZE * 16

///////////////////////////////////////////////////////////////////////////////
// Path finding
///////////////////////////////////////////////////////////////////////////////

/**
 * How fine the overlayed block grid for A* is (higher = finer); also sets how
 * many imps can work on one wall at a time */
#define DK_ASTAR_GRANULARITY 3

/**
 * Use jumps point search, skipping distances where possible.
 * D. Harabor and A. Grastien. Online Graph Pruning for Pathfinding on Grid Maps.
 * In National Conference on Artificial Intelligence (AAAI), 2011. */
#define DK_ASTAR_JPS 1

/** Defines how capacity of lists grows when exceeded */
#define DK_ASTAR_CAPACITY_GROWTH(old_capacity) (old_capacity * 2 + 16)

///////////////////////////////////////////////////////////////////////////////
// AI
///////////////////////////////////////////////////////////////////////////////

/** Delay in updates to wait before doing an actual update when idling */
#define DK_AI_IDLE_DELAY 16

/** Number of times the delay has to kick in before we try to wander again */
#define DK_AI_WANDER_DELAY 5

/** How often to try to find a point to wander to in one round (may fail) */
#define DK_AI_WANDER_TRIES 3

/** Maximum number of stacked jobs */
#define DK_AI_JOB_STACK_MAX 8

/** Maximum path length a unit can keep track of */
#define DK_AI_PATH_MAX 32

/** Bonus accounted to a worker that's already on a job when checking if closer */
#define DK_AI_ALREADY_WORKING_BONUS DK_BLOCK_SIZE / 2

/** Defines how capacity of lists grows when exceeded */
#define DK_AI_JOB_CAPACITY_GROWTH(old_capacity) (old_capacity * 2 + 16)

///////////////////////////////////////////////////////////////////////////////
// Map
///////////////////////////////////////////////////////////////////////////////

/** Damage a normal dirt block can take before breaking */
#define DK_BLOCK_DIRT_HEALTH 80

/** Damage a gold block can take before breaking */
#define DK_BLOCK_GOLD_HEALTH 160

/** Damage a gem block can take before breaking; broke one?... well, good job */
#define DK_BLOCK_GEM_HEALTH UINT32_MAX

/** Amount of gold that drops per damage done to a gold or gem block */
#define DK_BLOCK_GOLD_PER_HEALTH 10

/** The base strength of a normal dirt block */
#define DK_BLOCK_DIRT_STRENGTH 200

/** The base strength of a normal dirt block */
#define DK_BLOCK_DIRT_OWNED_STRENGTH 600

/** The base strength of an empty block */
#define DK_BLOCK_NONE_STRENGTH 80

/** The base strength of an owned empty block */
#define DK_BLOCK_NONE_OWNED_STRENGTH 250

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

/** The horizontal number of blocks to render around the camera position */
#define DK_RENDER_AREA_X 20

/** The vertical number of blocks to render around the camera position */
#define DK_RENDER_AREA_Y 16

/** Y offset of tiles to render (due to camera looking slightly forward) */
#define DK_RENDER_AREA_Y_OFFSET 3

/** Size of the border to allocate around the actual map vertices to render out of range area */
#define DK_MAP_BORDER (DK_RENDER_AREA_X * 2)

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

/** Use terrain noise */
#define DK_D_TERRAIN_NOISE 1

/** Factor in surroundings for terrain noise (empty blocks, owned blocks) */
#define DK_D_USE_NOISE_OFFSET 1

/** Use test texture instead of actual textures */
int DK_d_draw_test_texture;

/** Render unit paths */
int DK_d_draw_paths;

/** Height at which to render paths */
#define DK_D_DRAW_PATH_HEIGHT 1.1f

/** Render job slots for player red */
int DK_d_draw_jobs;

#endif	/* CONFIG_H */

