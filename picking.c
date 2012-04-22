#include <inttypes.h>

#include "GLee.h"
#include <GL/glu.h>

#include "config.h"
#include "picking.h"

GLuint DK_pick(int x, int y, void(*render)(void)) {
    GLuint buffer[64] = {0}, closest;
    GLint hits, hit;
    unsigned int depth;

    // Store state.
    glPushAttrib(GL_TRANSFORM_BIT | GL_ENABLE_BIT | GL_CURRENT_BIT);

    glSelectBuffer(sizeof (buffer), buffer);
    glRenderMode(GL_SELECT);
    glInitNames();
    glPushName(0);

    // Now modify the viewing volume, restricting selection area around the cursor
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();

    // Restrict the draw to an area around the cursor.
    {
        GLint view[4];
        glGetIntegerv(GL_VIEWPORT, view);
        gluPickMatrix(x, y, 1.0, 1.0, view);
        gluPerspective(80.0, DK_ASPECT_RATIO, 0.1, 1000.0);

        render();
    }
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();

    hits = glRenderMode(GL_RENDER);

    // Restore state.
    glPopAttrib();

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
