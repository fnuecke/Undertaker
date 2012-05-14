#include "picking.h"

#include <GL/glew.h>

#include "config.h"
#include "graphics.h"
#include "vmath.h"

bool MP_Pick(int x, int y, void(*render)(void), GLuint* name, float* depth) {
    GLuint buffer[64] = {0};
    GLint hits;

    glSelectBuffer(64, buffer);
    glRenderMode(GL_SELECT);
    glInitNames();
    glPushName(0);

    // Now modify the viewing volume, restricting selection area around the
    // cursor. This way only render what's close to it.
    MP_BeginPerspectiveForPicking(x, y);

    render();

    // Done with rendering, pop the matrix.
    MP_EndPerspective();

    // See if we hit anything.
    hits = glRenderMode(GL_RENDER);

    if (!hits) {
        return false;
    } else {
        // Find the element that's closest to the eye.
        GLuint closest = 0, zbuffer = UINT32_MAX;
        for (GLint hit = 0; hit < hits; ++hit) {
            if (buffer[hit * 4 + 3] && buffer[hit * 4 + 2] < zbuffer) {
                closest = buffer[hit * 4 + 3];
                zbuffer = buffer[hit * 4 + 1];
            }
        }

        if (name) {
            *name = closest;
        }
        if (depth) {
            *depth = zbuffer / (float) UINT32_MAX;
        }
    }
    return true;
}
