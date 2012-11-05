/* 
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
    int MP_resolutionX;

    /** Screen resolution along the y axis */
    int MP_resolutionY;

    /** The field of view to use */
    int MP_fieldOfView;

    /** Whether to use anti aliasing or not */
    bool MP_antialiasing;

    /** The stream to write log messages to */
    FILE* MP_logTarget;

    ///////////////////////////////////////////////////////////////////////////////
    // Camera
    ///////////////////////////////////////////////////////////////////////////////

    /** Scroll speed of the camera */
    float MP_scrollSpeed;

    ///////////////////////////////////////////////////////////////////////////////
    // Debugging
    ///////////////////////////////////////////////////////////////////////////////

    /** Whether the AI is enabled (units are being updated) */
    bool MP_DBG_isAIEnabled;

    /** Use test texture instead of actual textures */
    bool MP_DBG_drawTestTexture;

    /** Render unit paths */
    bool MP_DBG_drawPaths;

    /** Render job slots for player red */
    bool MP_DBG_drawJobs;

    /** Possible steps of the deferred rendering that can be rendered */
    typedef enum {
        MP_DBG_BUFFER_FINAL,
        MP_DBG_BUFFER_DIFFUSE,
        MP_DBG_BUFFER_POSITION,
        MP_DBG_BUFFER_NORMALS,
        MP_DBG_BUFFER_DEPTH,
        MP_DBG_BUFFER_COUNT /* Number of possibilities, for bounding */
    } MP_DBG_DisplayBuffer;

    /** What part of the deferred rendering process to output */
    MP_DBG_DisplayBuffer MP_DBG_deferredBuffer;

    /** Show what the picking matrix sees */
    bool MP_DBG_drawPickingMode;

    /** Render using the deferred shading pipeline (shaders)? */
    bool MP_DBG_useDeferredShader;

    /** Visualize the number of lights processed per pixel */
    bool MP_DBG_drawLightVolumes;

    ///////////////////////////////////////////////////////////////////////////////
    // Saving / loading
    ///////////////////////////////////////////////////////////////////////////////

    /** Load configuration from disk */
    void MP_LoadConfig(void);

    /** Save configuration to disk */
    void MP_SaveConfig(void);

    ///////////////////////////////////////////////////////////////////////////////
    // Macros
    ///////////////////////////////////////////////////////////////////////////////

#define EXIT_ON_OPENGL_ERROR()\
    { \
        GLenum error = glGetError(); \
        if (error != GL_NO_ERROR) { \
            MP_log_fatal("OpenGL broke:\n%s\n", gluErrorString(error)); \
        } \
    }

    ///////////////////////////////////////////////////////////////////////////////
    // Defines (not user-changeable)
    ///////////////////////////////////////////////////////////////////////////////

    ///////////////////////////////////////////////////////////////////////////////
    // AI
    ///////////////////////////////////////////////////////////////////////////////

    /** Maximum path length a unit can keep track of */
#define MP_AI_PATH_DEPTH 32

    /** Whether to use interpolation for estimating path segment lengths */
#define MP_AI_PATH_INTERPOLATE 1

    /** Factor to use for catmull rom path smoothing */
#define MP_AI_CATMULL_ROM_T 0.25f

    /** Interpolation steps to compute when estimating a path segment's length */
#define MP_AI_PATH_INTERPOLATION 5

    /** Bonus accounted to a worker that's already on a job when checking if closer */
#define MP_AI_ALREADY_WORKING_BONUS 0.5f

    ///////////////////////////////////////////////////////////////////////////////
    // Camera
    ///////////////////////////////////////////////////////////////////////////////

    /** Slowing factor when stopping camera movement */
#define MP_CAMERA_FRICTION 0.5f

    /** Default distance of the camera from the ground */
#define MP_CAMERA_HEIGHT (MP_BLOCK_SIZE * 6)

    /** Distance of the look-at target to the camera */
#define MP_CAMERA_TARGET_DISTANCE (MP_BLOCK_SIZE * 2)

    /** How far to zoom in (Z axis) for one zoom step */
#define MP_CAMERA_ZOOM_STEP (1.0f / 3.0f)

    /** Maximum camera zoom amount */
#define MP_CAMERA_MAX_ZOOM (3 * MP_BLOCK_SIZE)

    ///////////////////////////////////////////////////////////////////////////////
    // Display
    ///////////////////////////////////////////////////////////////////////////////

    /** The screen aspect ratio */
#define MP_ASPECT_RATIO ((float)MP_resolutionX / (float)MP_resolutionY)

    /** Near clip plane for rendering */
#define MP_CLIP_NEAR 10.0f

    /** Far clip plane for rendering */
#define MP_CLIP_FAR 600.0f

    /** Target framerate */
#define MP_FRAMERATE 60

    ///////////////////////////////////////////////////////////////////////////////
    // Debugging
    ///////////////////////////////////////////////////////////////////////////////

    /** Use terrain noise */
#define MP_D_TERRAIN_NOISE 1

    /** Factor in surroundings for terrain noise (empty blocks, owned blocks) */
#define MP_D_USE_NOISE_OFFSET 1

    /** Height at which to render paths */
#define MP_D_DRAW_PATH_HEIGHT 1.1f

    ///////////////////////////////////////////////////////////////////////////////
    // Map rendering
    ///////////////////////////////////////////////////////////////////////////////

    /** Size of a block in OpenGL units */
#define MP_BLOCK_SIZE 16

    /** Height of a block in OpenGL units */
#define MP_BLOCK_HEIGHT 24

    /** Depth of water in OpenGL units */
#define MP_WATER_LEVEL 4

    /** The maximum amount a vertex is offset based on neighboring empty blocks */
#define MP_BLOCK_MAX_NOISE_OFFSET (MP_BLOCK_SIZE * 0.5f)

    /** Multiplier applied to vertices reducing above offset based on nearby owned blocks */
#define MP_OWNED_NOISE_REDUCTION 0.8f

    /** The horizontal number of blocks to render around the camera position */
#define MP_RENDER_AREA_X ((int)(MP_ASPECT_RATIO * MP_fieldOfView * 0.25f) + 1)

    /** The vertical number of blocks to render around the camera position */
#define MP_RENDER_AREA_Y ((int)(MP_fieldOfView * 0.2f))

    /** Y offset of tiles to render (due to camera looking slightly forward) */
#define MP_RENDER_AREA_Y_OFFSET (MP_RENDER_AREA_Y * 0.2f)

    /** Size of the border to allocate around the actual map vertices to render out of range area */
#define MP_MAP_BORDER (MP_RENDER_AREA_X + (MP_RENDER_AREA_X & 1))

    /** The color of the selection outline */
#define MP_MAP_SELECTED_COLOR_R 0.3f
#define MP_MAP_SELECTED_COLOR_G 0.5f
#define MP_MAP_SELECTED_COLOR_B 0.7f
#define MP_MAP_SELECTED_COLOR_A_MIN (0.6f - MP_AMBIENT_LIGHT_COLOR_POWER)
#define MP_MAP_SELECTED_COLOR_A_MAX (0.9f - MP_AMBIENT_LIGHT_COLOR_POWER)
#define MP_MAP_SELECTED_PULSE_FREQUENCY 1 / 750.0f
#define MP_MAP_SELECTED_COLOR_A (MP_MAP_SELECTED_COLOR_A_MIN + sinf(SDL_GetTicks() * 3.14159265358979323846f * (MP_MAP_SELECTED_PULSE_FREQUENCY)) * (MP_MAP_SELECTED_COLOR_A_MAX - MP_MAP_SELECTED_COLOR_A_MIN))

    /** Offset for selection outline to not intersect with map blocks */
#define MP_MAP_SELECTION_OFFSET 0.15f

    /** The color of the selection outline */
#define MP_MAP_OUTLINE_COLOR_R 0.4f
#define MP_MAP_OUTLINE_COLOR_G 0.5f
#define MP_MAP_OUTLINE_COLOR_B 0.9f
#define MP_MAP_OUTLINE_COLOR_A 0.5f

    /** Base lighting */
#define MP_AMBIENT_LIGHT_COLOR_POWER 0.2f

    /** Color of the light at the cursor */
#define MP_HAND_LIGHT_COLOR_R (1.0f - MP_AMBIENT_LIGHT_COLOR_POWER)
#define MP_HAND_LIGHT_COLOR_G (0.9f - MP_AMBIENT_LIGHT_COLOR_POWER)
#define MP_HAND_LIGHT_COLOR_B (0.8f - MP_AMBIENT_LIGHT_COLOR_POWER)

    /** Height of the hand light */
#define MP_HAND_LIGHT_HEIGHT (MP_BLOCK_SIZE * 3)

    /** Brightness of the light at the cursor */
#define MP_HAND_LIGHT_RANGE 6.0f * MP_BLOCK_SIZE

    /** Color of the light at the cursor */
#define MP_WALL_LIGHT_COLOR_R (0.9f - MP_AMBIENT_LIGHT_COLOR_POWER)
#define MP_WALL_LIGHT_COLOR_G (0.8f - MP_AMBIENT_LIGHT_COLOR_POWER)
#define MP_WALL_LIGHT_COLOR_B (0.7f - MP_AMBIENT_LIGHT_COLOR_POWER)

    /** Height of the hand light */
#define MP_WALL_LIGHT_HEIGHT (MP_BLOCK_SIZE * 0.8f)

    /** Brightness of the light at the cursor */
#define MP_WALL_LIGHT_RANGE 2.0f * MP_BLOCK_SIZE

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
    // Units
    ///////////////////////////////////////////////////////////////////////////////

    /** Maximum number of units a single player have at a time */
#define MP_UNITS_MAX_PER_PLAYER 100

#ifdef	__cplusplus
}
#endif

#endif
