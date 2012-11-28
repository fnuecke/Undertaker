#include "cursor.h"

#include <SDL/SDL.h>
#include <GL/glew.h>

#include "camera.h"
#include "config.h"
#include "events.h"
#include "graphics.h"
#include "vmath.h"

///////////////////////////////////////////////////////////////////////////////
// Globals
///////////////////////////////////////////////////////////////////////////////

/**
 * The current cursor position.
 */
static vec2 gCursorPosition[MP_CURSOR_LEVEL_COUNT];

///////////////////////////////////////////////////////////////////////////////
// Utility methods
///////////////////////////////////////////////////////////////////////////////

/**
 * Updates cursor position at a specified height.
 * @param level the entry in the array.
 * @param z the actual z level.
 * @param mouseX mouse x position in window.
 * @param mouseY mouse y position in window.
 */
static void updateLevel(MP_CursorLevel level, float z, int mouseX, int mouseY) {
    float nearX, nearY, nearZ, tX, tY, tZ;

    // Get near and far plane.
    if (!MP_UnProject(mouseX, mouseY, 0, &nearX, &nearY, &nearZ)) {
        return;
    }
    if (!MP_UnProject(mouseX, mouseY, 1, &tX, &tY, &tZ)) {
        return;
    }

    // Build direction vector.
    tX -= nearX;
    tY -= nearY;
    tZ -= nearZ;

    // Intersection with xy plane -- at given height.
    tZ = (z - nearZ) / tZ;
    gCursorPosition[level].v[0] = nearX + tX * tZ;
    gCursorPosition[level].v[1] = nearY + tY * tZ;
}

/**
 * Updates cursor positions; muse be called after camera was set up.
 */
static void onPreRender(void) {
    // Get environment info.
    int mouseX, mouseY;

    // Get window mouse coordinates.
    SDL_GetMouseState(&mouseX, &mouseY);
    mouseY = MP_resolutionY - mouseY;

    updateLevel(MP_CURSOR_LEVEL_FLOOR, 0, mouseX, mouseY);
    updateLevel(MP_CURSOR_LEVEL_TOP, MP_BLOCK_HEIGHT, mouseX, mouseY);
}

///////////////////////////////////////////////////////////////////////////////
// Header implementation
///////////////////////////////////////////////////////////////////////////////

const vec2* MP_GetCursor(MP_CursorLevel level) {
    return &gCursorPosition[level];
}

void MP_InitCursor(void) {
    MP_AddPreRenderEventListener(onPreRender);
}
