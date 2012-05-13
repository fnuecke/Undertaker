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

#include "types.h"

#ifdef	__cplusplus
extern "C" {
#endif

    ///////////////////////////////////////////////////////////////////////////////
    // Display
    ///////////////////////////////////////////////////////////////////////////////

    /** Screen resolution along the x axis */
    int MP_resolution_x;

    /** Screen resolution along the y axis */
    int MP_resolution_y;

    /** The screen aspect ratio */
#define MP_ASPECT_RATIO ((float)MP_resolution_x / (float)MP_resolution_y)

    /** The field of view to use */
    int MP_field_of_view;

    /** Near clip plane for rendering */
#define MP_CLIP_NEAR 10.0f

    /** Far clip plane for rendering */
#define MP_CLIP_FAR 600.0f

    /** Whether to use anti aliasing or not */
    bool MP_use_antialiasing;

    /** Target framerate */
#define MP_FRAMERATE 60

    /** Use fog in the distance, to fade out to black */
    bool MP_use_fog;

    /** The stream to write log messages to */
    FILE* MP_log_target;

    ///////////////////////////////////////////////////////////////////////////////
    // Units
    ///////////////////////////////////////////////////////////////////////////////

    /** Maximum number of units a single player have at a time */
#define MP_UNITS_MAX_PER_PLAYER 100

    /** Maximum number of abilities a unit may have (number of cooldowns) */
#define MP_UNITS_MAX_ABILITIES 4

    ///////////////////////////////////////////////////////////////////////////////
    // Job priorities
    ///////////////////////////////////////////////////////////////////////////////
    // These are linear to the distance to the job: rank = dist * prio

    /** Priority for digging up stuff */
#define MP_JOB_DIG_PRIORITY 0

    /** Priority for converting floor tiles */
#define MP_JOB_CONVERT_FLOOR_PRIORITY 8

    /** Priority for converting walls */
#define MP_JOB_CONVERT_WALL_PRIORITY 16

    ///////////////////////////////////////////////////////////////////////////////
    // AI
    ///////////////////////////////////////////////////////////////////////////////

    /** Maximum number of stacked jobs */
#define MP_AI_STACK_DEPTH 8

    /** Maximum path length a unit can keep track of */
#define MP_AI_PATH_DEPTH 32

    /** Whether to use interpolation for estimating path segment lengths */
#define MP_AI_PATH_INTERPOLATE 1

    /** Factor to use for catmull rom path smoothing */
#define MP_AI_CATMULL_ROM_T 0.25f

    /** Interpolation steps to compute when estimating a path segment's length */
#define MP_AI_PATH_INTERPOLATION 5

    /** Bonus accounted to a worker that's already on a job when checking if closer */
#define MP_AI_ALREADY_WORKING_BONUS 0.5

    /** Defines how capacity of lists grows when exceeded */
#define MP_AI_JOB_CAPACITY_GROWTH(old_capacity) (old_capacity * 2 + 16)

    /** Distance the AI may wander, in blocks */
#define MP_AI_WANDER_RANGE 2

    /** Number of times the delay has to kick in before we try to wander again */
#define MP_AI_WANDER_DELAY 5

    ///////////////////////////////////////////////////////////////////////////////
    // Textures
    ///////////////////////////////////////////////////////////////////////////////

    /** Maximum number of variations for a single texture we support */
#define MP_TEX_MAX_VARIATIONS 8

    /** Base directory for textures */
#define MP_TEX_DIR "data/textures/"

    /** File extension used for textures */
#define MP_TEX_FILETYPE ".png"

    ///////////////////////////////////////////////////////////////////////////////
    // Map rendering
    ///////////////////////////////////////////////////////////////////////////////

    /** Size of a block in openGL units */
#define MP_BLOCK_SIZE 16

    /** Height of a block in openGL units */
#define MP_BLOCK_HEIGHT 24

    /** Depth of water */
#define MP_WATER_LEVEL 4

    /** The maximum amount a vertex is offset based on neighboring empty blocks */
#define MP_BLOCK_MAX_NOISE_OFFSET (MP_BLOCK_SIZE * 0.5)

    /** Multiplier applied to vertices reducing above offset based on nearby owned blocks */
#define MP_OWNED_NOISE_REDUCTION 0.8

    /** The horizontal number of blocks to render around the camera position */
#define MP_RENDER_AREA_X (((int)(22 * MP_ASPECT_RATIO)) + (((int)(22 * MP_ASPECT_RATIO)) & 1))

    /** The vertical number of blocks to render around the camera position */
#define MP_RENDER_AREA_Y 16

    /** Y offset of tiles to render (due to camera looking slightly forward) */
#define MP_RENDER_AREA_Y_OFFSET 3

    /** Size of the border to allocate around the actual map vertices to render out of range area */
#define MP_MAP_BORDER MP_RENDER_AREA_X

    /** The color of the selection outline */
#define MP_MAP_SELECTED_COLOR_R 0.3f
#define MP_MAP_SELECTED_COLOR_G 0.5f
#define MP_MAP_SELECTED_COLOR_B 0.7f
#define MP_MAP_SELECTED_COLOR_A_MIN 0.6f
#define MP_MAP_SELECTED_COLOR_A_MAX 0.9f
#define MP_MAP_SELECTED_PULSE_FREQUENCY 1 / 750.0f
#define MP_MAP_SELECTED_COLOR_A (MP_MAP_SELECTED_COLOR_A_MIN + sinf(SDL_GetTicks() * 3.14159265358979323846f * (MP_MAP_SELECTED_PULSE_FREQUENCY)) * (MP_MAP_SELECTED_COLOR_A_MAX - MP_MAP_SELECTED_COLOR_A_MIN))

    /** The color of the selection outline */
#define MP_MAP_OUTLINE_COLOR_R 0.4f
#define MP_MAP_OUTLINE_COLOR_G 0.5f
#define MP_MAP_OUTLINE_COLOR_B 0.9f
#define MP_MAP_OUTLINE_COLOR_A 0.5f

    /** Offset for selection outline to not intersect with map blocks */
#define MP_MAP_SELECTION_OFFSET 0.15f

    /** Color of the light at the cursor */
#define MP_HAND_LIGHT_COLOR_R 1.0f
#define MP_HAND_LIGHT_COLOR_G 1.0f
#define MP_HAND_LIGHT_COLOR_B 1.0f

    /** Height of the hand light */
#define MP_HAND_LIGHT_HEIGHT (MP_BLOCK_SIZE * 3)

    /** Brightness of the light at the cursor */
#define MP_HAND_LIGHT_POWER 24.0f

    /** Color of the light at the cursor */
#define MP_WALL_LIGHT_COLOR_R 1.0f
#define MP_WALL_LIGHT_COLOR_G 0.9f
#define MP_WALL_LIGHT_COLOR_B 0.8f

    /** Height of the hand light */
#define MP_WALL_LIGHT_HEIGHT (MP_BLOCK_SIZE * 0.8f)

    /** Brightness of the light at the cursor */
#define MP_WALL_LIGHT_POWER 8.0f

    ///////////////////////////////////////////////////////////////////////////////
    // Camera
    ///////////////////////////////////////////////////////////////////////////////

    /** Slowing factor when stopping camera movement */
#define MP_CAMERA_FRICTION 0.8f

    /** Scroll speed of the camera */
#define MP_CAMERA_SPEED 4.0f

    /** Default distance of the camera from the ground */
#define MP_CAMERA_HEIGHT (MP_BLOCK_SIZE * 6)

    /** Distance of the look-at target to the camera */
#define MP_CAMERA_TARGET_DISTANCE (MP_BLOCK_SIZE * 2)

    /** How far to zoom in (Z axis) for one zoom step */
#define MP_CAMERA_ZOOM_STEP (1.0f / 3.0f)

    /** Maximum camera zoom amount */
#define MP_CAMERA_MAX_ZOOM (3 * MP_BLOCK_SIZE)

    ///////////////////////////////////////////////////////////////////////////////
    // Debugging
    ///////////////////////////////////////////////////////////////////////////////

    /** Use terrain noise */
#define MP_D_TERRAIN_NOISE 1

    /** Factor in surroundings for terrain noise (empty blocks, owned blocks) */
#define MP_D_USE_NOISE_OFFSET 1

    /** Whether the AI is enabled (units are being updated) */
    bool MP_d_ai_enabled;

    /** Use test texture instead of actual textures */
    bool MP_d_draw_test_texture;

    /** Render unit paths */
    bool MP_d_draw_paths;

    /** Height at which to render paths */
#define MP_D_DRAW_PATH_HEIGHT 1.1f

    /** Render job slots for player red */
    bool MP_d_draw_jobs;

    /** Possible steps of the deferred rendering that can be rendered */
    typedef enum {
        MP_D_DEFERRED_FINAL,
        MP_D_DEFERRED_DIFFUSE,
        MP_D_DEFERRED_POSITION,
        MP_D_DEFERRED_NORMALS,
        MP_D_DEPTH_BUFFER,
        MP_D_DISPLAY_MODE_COUNT /* Number of possibilities, for bounding */
    } MP_DisplayMode;

    /** What part of the deferred rendering process to output */
    MP_DisplayMode MP_d_draw_deferred;

    /** Show what the picking matrix sees */
    bool MP_d_draw_picking_mode;

    /** Render using the deferred shading pipeline (shaders)? */
    bool MP_d_draw_deferred_shader;

    /** Visualize the number of lights processed per pixel */
    bool MP_d_draw_light_volumes;

    ///////////////////////////////////////////////////////////////////////////////
    // Macros
    ///////////////////////////////////////////////////////////////////////////////

#define EXIT_ON_OPENGL_ERROR()\
    { \
        GLenum error = glGetError(); \
        if (error != GL_NO_ERROR) { \
            MP_log("ERROR: OpenGL broke at %s:%d:\n%s\n", __FILE__, __LINE__, gluErrorString(error)); \
            exit(EXIT_FAILURE); \
        } \
    }

    ///////////////////////////////////////////////////////////////////////////////
    // Saving / loading
    ///////////////////////////////////////////////////////////////////////////////

    /** Load configuration from disk */
    void MP_load_config(void);

    /** Save configuration to disk */
    void MP_save_config(void);

#ifdef	__cplusplus
}
#endif

#endif	/* CONFIG_H */

