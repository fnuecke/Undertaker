#include <SDL.h>

#include "GLee.h"
#include <GL/glu.h>

#include "camera.h"
#include "config.h"
#include "cursor.h"

float cursor_x, cursor_y;

void DK_cursor(float* x, float* y) {
    *x = cursor_x;
    *y = cursor_y;
}

void DK_update_cursor(void) {
    // Get environment info.
    GLdouble model[16];
    GLdouble proj[16];
    GLint view[4];
    GLdouble ox, oy, oz;
    int mouseX, mouseY;
    GLdouble objXn, objYn, objZn;
    GLdouble objXf, objYf, objZf;

    glGetDoublev(GL_MODELVIEW_MATRIX, model);
    glGetDoublev(GL_PROJECTION_MATRIX, proj);
    glGetIntegerv(GL_VIEWPORT, view);

    gluProject(DK_camera_position()[0], DK_camera_position()[1] + DK_CAMERA_TARGET_DISTANCE, 0, model, proj, view, &ox, &oy, &oz);

    // Get window mouse coordinates.
    SDL_GetMouseState(&mouseX, &mouseY);
    mouseY = DK_resolution_y - mouseY;

    // Get near plane.
    gluUnProject(mouseX, mouseY, 0, model, proj, view, &objXn, &objYn, &objZn);

    // Get far plane.
    gluUnProject(mouseX, mouseY, 1, model, proj, view, &objXf, &objYf, &objZf);

    // Build vector.
    {
        const GLdouble vx = objXf - objXn;
        const GLdouble vy = objYf - objYn;
        const GLdouble vz = objZf - objZn;

        // Intersection with xy plane -- at block height.
        const GLdouble m = (DK_BLOCK_HEIGHT - objZn) / vz;
        cursor_x = objXn + vx * m;
        cursor_y = objYn + vy * m;
    }
}
