#include <SDL/SDL.h>
#include <GL/glew.h>

#include "camera.h"
#include "config.h"
#include "cursor.h"

static vec2 gCursorPosition;

const vec2* DK_GetCursor(void) {
    return &gCursorPosition;
}

void DK_UpdateCursor(void) {
    // Get environment info.
    const vec2* camera_position = DK_GetCameraPosition();
    mat4 model;
    mat4 proj;
    GLint view;
    GLdouble ox, oy, oz;
    int mouseX, mouseY;
    GLdouble objXn, objYn, objZn;
    GLdouble objXf, objYf, objZf;

    
    glGetDoublev(GL_MODELVIEW_MATRIX, model);
    glGetDoublev(GL_PROJECTION_MATRIX, proj);
    glGetIntegerv(GL_VIEWPORT, view);

    gluProject(camera_position->x, camera_position->y + DK_CAMERA_TARGET_DISTANCE, 0, model->m, proj->m, view, &ox, &oy, &oz);

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
        gCursorPosition.x = objXn + vx * m;
        gCursorPosition.y = objYn + vy * m;
    }
}
