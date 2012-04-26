#include <SDL/SDL.h>
#include <GL/glew.h>

#include "camera.h"
#include "config.h"
#include "cursor.h"
#include "vmath.h"
#include "graphics.h"
#include "update.h"
#include "render.h"

/**
 * The current cursor position.
 */
static vec2 gCursorPosition;

static void update(void) {
    // Get environment info.
    int mouseX, mouseY;
    float objXn, objYn, objZn;
    float objXf, objYf, objZf;

    // Get window mouse coordinates.
    SDL_GetMouseState(&mouseX, &mouseY);
    mouseY = DK_resolution_y - mouseY;

    // Get near plane.
    //gluUnProject(mouseX, mouseY, 0, model.m, proj.m, view, &objXn, &objYn, &objZn);
    DK_UnProject(mouseX, mouseY, 0, &objXn, &objYn, &objZn);

    // Get far plane.
    //gluUnProject(mouseX, mouseY, 1, model.m, proj.m, view, &objXf, &objYf, &objZf);
    DK_UnProject(mouseX, mouseY, 1, &objXf, &objYf, &objZf);

    // Build vector.
    {
        const GLdouble vx = objXf - objXn;
        const GLdouble vy = objYf - objYn;
        const GLdouble vz = objZf - objZn;

        // Intersection with xy plane -- at block height.
        const GLdouble m = (DK_BLOCK_HEIGHT - objZn) / vz;
        gCursorPosition.v[0] = objXn + vx * m;
        gCursorPosition.v[1] = objYn + vy * m;
    }
}

const vec2* DK_GetCursor(void) {
    return &gCursorPosition;
}

void DK_InitCursor(void) {
    DK_OnPreRender(update);
}
