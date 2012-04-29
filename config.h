/* 
 * File:   config.h
 * Author: fnuecke
 *
 * Created on April 14, 2012, 9:46 AM
 */

#ifndef CONFIG_H
#define	CONFIG_H

#include <stdio.h>

#include <GL/glew.h>

#ifdef	__cplusplus
extern "C" {
#endif

///////////////////////////////////////////////////////////////////////////////
// Display
///////////////////////////////////////////////////////////////////////////////

/** Screen resolution along the x axis */
int DK_resolution_x;

/** Screen resolution along the y axis */
int DK_resolution_y;

/** The screen aspect ratio */
#define DK_ASPECT_RATIO ((float)DK_resolution_x / (float)DK_resolution_y)

/** The field of view to use */
int DK_field_of_view;

/** Near clip plane for rendering */
#define DK_CLIP_NEAR 0.1f

/** Far clip plane for rendering */
#define DK_CLIP_FAR 500.0f

/** Whether to use anti aliasing or not */
char DK_use_antialiasing;

/** Target framerate */
#define DK_FRAMERATE 60

/** Use fog in the distance, to fade out to black */
char DK_use_fog;

/** The stream to write log messages to */
FILE* DK_log_target;

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
#define DK_JOB_CONVERT_FLOOR_PRIORITY 8

/** Priority for converting walls */
#define DK_JOB_CONVERT_WALL_PRIORITY 16

///////////////////////////////////////////////////////////////////////////////
// Path finding
///////////////////////////////////////////////////////////////////////////////

/**
 * How fine the overlayed block grid for A* is (higher = finer); also sets how
 * many imps can work on one wall at a time */
#define DK_ASTAR_GRANULARITY 2

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

/** Whether to use interpolation for estimating path segment lengths */
#define DK_AI_PATH_INTERPOLATE 1

/** Factor to use for catmull rom path smoothing */
#define DK_AI_CATMULL_ROM_T 0.25f

/** Interpolation steps to compute when estimating a path segment's length */
#define DK_AI_PATH_INTERPOLATION 5

/** Bonus accounted to a worker that's already on a job when checking if closer */
#define DK_AI_ALREADY_WORKING_BONUS 0.5

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
#define DK_RENDER_AREA_X (((int)(22 * DK_ASPECT_RATIO)) + (((int)(22 * DK_ASPECT_RATIO)) & 1))

/** The vertical number of blocks to render around the camera position */
#define DK_RENDER_AREA_Y 16

/** Y offset of tiles to render (due to camera looking slightly forward) */
#define DK_RENDER_AREA_Y_OFFSET 3

/** Size of the border to allocate around the actual map vertices to render out of range area */
#define DK_MAP_BORDER DK_RENDER_AREA_X

/** The color of the selection outline */
#define DK_MAP_SELECTED_COLOR_R 0.3f
#define DK_MAP_SELECTED_COLOR_G 0.5f
#define DK_MAP_SELECTED_COLOR_B 0.7f
#define DK_MAP_SELECTED_COLOR_A_MIN 0.6f
#define DK_MAP_SELECTED_COLOR_A_MAX 0.9f
#define DK_MAP_SELECTED_PULSE_FREQUENCY 1 / 750.0f
#define DK_MAP_SELECTED_COLOR_A (DK_MAP_SELECTED_COLOR_A_MIN + sinf(SDL_GetTicks() * 3.14159265358979323846f * (DK_MAP_SELECTED_PULSE_FREQUENCY)) * (DK_MAP_SELECTED_COLOR_A_MAX - DK_MAP_SELECTED_COLOR_A_MIN))

/** The color of the selection outline */
#define DK_MAP_OUTLINE_COLOR_R 0.4f
#define DK_MAP_OUTLINE_COLOR_G 0.5f
#define DK_MAP_OUTLINE_COLOR_B 0.9f
#define DK_MAP_OUTLINE_COLOR_A 0.5f

/** Offset for selection outline to not intersect with map blocks */
#define DK_MAP_SELECTION_OFFSET 0.15f

/** Color of the light at the cursor */
#define DK_HAND_LIGHT_COLOR_R 1.0f
#define DK_HAND_LIGHT_COLOR_G 1.0f
#define DK_HAND_LIGHT_COLOR_B 1.0f

/** Brightness of the light at the cursor */
#define DK_HAND_LIGHT_POWER 3000.0f

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
#define DK_CAMERA_MAX_ZOOM 3 * DK_BLOCK_SIZE

///////////////////////////////////////////////////////////////////////////////
// Debugging
///////////////////////////////////////////////////////////////////////////////

/** Use terrain noise */
#define DK_D_TERRAIN_NOISE 1

/** Factor in surroundings for terrain noise (empty blocks, owned blocks) */
#define DK_D_USE_NOISE_OFFSET 1

/** Whether the AI is enabled (units are being updated) */
char DK_d_ai_enabled;

/** Use test texture instead of actual textures */
char DK_d_draw_test_texture;

/** Render unit paths */
char DK_d_draw_paths;

/** Height at which to render paths */
#define DK_D_DRAW_PATH_HEIGHT 1.1f

/** Render job slots for player red */
char DK_d_draw_jobs;

/** Possible steps of the deferred rendering that can be rendered */
typedef enum {
    DK_D_DEFERRED_FINAL,
    DK_D_DEFERRED_DIFFUSE,
    DK_D_DEFERRED_POSITION,
    DK_D_DEFERRED_NORMALS,
    DK_D_DEPTH_BUFFER,
    DK_D_DISPLAY_MODE_COUNT /* Number of possibilities, for bounding */
} DK_DisplayMode;

/** What part of the deferred rendering process to output */
DK_DisplayMode DK_d_draw_deferred;

/** Show what the picking matrix sees */
char DK_d_draw_picking_mode;

/** Render using the deferred shading pipeline (shaders)? */
char DK_d_draw_deferred_shader;

///////////////////////////////////////////////////////////////////////////////
// Macros
///////////////////////////////////////////////////////////////////////////////

#define EXIT_ON_OPENGL_ERROR()\
    { \
        GLenum error = glGetError(); \
        if (error != GL_NO_ERROR) { \
            fprintf(DK_log_target, "ERROR: OpenGL broke at %s:%d:\n%s\n", __FILE__, __LINE__, gluErrorString(error)); \
            exit(EXIT_FAILURE); \
        } \
    }

///////////////////////////////////////////////////////////////////////////////
// Saving / loading
///////////////////////////////////////////////////////////////////////////////

/** Load configuration from disk */
void DK_load_config(void);

/** Save configuration to disk */
void DK_save_config(void);

#ifdef	__cplusplus
}
#endif

#endif	/* CONFIG_H */

