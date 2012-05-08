#include "picking.h"

#include <inttypes.h>

#include <GL/glew.h>

#include "config.h"
#include "graphics.h"
#include "vmath.h"

GLuint MP_Pick(int x, int y, void(*render)(void)) {
    GLuint buffer[64] = {0};
    GLuint closest, depth;
    GLint hits, hit;

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

    // Find the element that's closest to the eye.
    closest = 0;
    depth = UINT32_MAX;
    for (hit = 0; hit < hits; ++hit) {
        if (buffer[hit * 4 + 3] && buffer[hit * 4 + 2] < depth) {
            closest = buffer[hit * 4 + 3];
            depth = buffer[hit * 4 + 1];
        }
    }

    return closest;
}
