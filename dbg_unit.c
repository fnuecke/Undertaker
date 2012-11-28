#include "config.h"
#include "graphics.h"
#include "render.h"
#include "unit.h"
#include "unit_ai.h"

///////////////////////////////////////////////////////////////////////////////
// Render
///////////////////////////////////////////////////////////////////////////////

/** Used for debug rendering of units and paths */
static GLUquadric* quadratic = 0;

/** Catmull-rom interpolation */
inline static float cr(float p0, float p1, float p2, float p3, float t) {
    const float m1 = MP_AI_CATMULL_ROM_T * (p2 - p0);
    const float m2 = MP_AI_CATMULL_ROM_T * (p3 - p1);
    return (2 * p1 - 2 * p2 + m1 + m2) * t * t * t +
            (-3 * p1 + 3 * p2 - 2 * m1 - m2) * t * t +
            m1 * t +
            p1;
}

static void renderPathing(const MP_Unit* unit) {
    MP_Material material;

    // If we display pathing render the units current path.
    if (MP_DBG_drawPaths && MP_IsUnitMoving(unit)) {
        const vec2* path = unit->ai->pathing.nodes;
        const unsigned int depth = unit->ai->pathing.depth;

        if (!quadratic) {
            quadratic = gluNewQuadric();
        }

        // Draw a line along the path.
        MP_InitMaterial(&material);
        material.emissivity = 1.0f;
        material.diffuseColor.c.r = 0.8f;
        material.diffuseColor.c.g = 0.8f;
        material.diffuseColor.c.b = 0.9f;
        MP_SetMaterial(&material);
        glLineWidth(1.75f);

        glBegin(GL_LINES);
        {
            unsigned int j = 0;
            glVertex3f(path[1].d.x * MP_BLOCK_SIZE, path[1].d.y * MP_BLOCK_SIZE, MP_D_DRAW_PATH_HEIGHT);
            for (j = 2; j <= depth; ++j) {
                // Somewhere in the middle, smooth the path.
                for (unsigned int k = 1; k < 20; ++k) {
                    const float t = k / 20.0f;
                    const float x = cr(path[j - 2].d.x, path[j - 1].d.x, path[j].d.x, path[j + 1].d.x, t);
                    const float y = cr(path[j - 2].d.y, path[j - 1].d.y, path[j].d.y, path[j + 1].d.y, t);
                    glVertex3f(x * MP_BLOCK_SIZE, y * MP_BLOCK_SIZE, MP_D_DRAW_PATH_HEIGHT);
                    glVertex3f(x * MP_BLOCK_SIZE, y * MP_BLOCK_SIZE, MP_D_DRAW_PATH_HEIGHT);
                }
            }
            glVertex3f(path[j - 1].d.x * MP_BLOCK_SIZE, path[j - 1].d.y * MP_BLOCK_SIZE, MP_D_DRAW_PATH_HEIGHT);
        }
        glEnd();

        // Draw points marking the waypoints.
        material.diffuseColor.c.r = 0.8f;
        material.diffuseColor.c.g = 0.4f;
        material.diffuseColor.c.b = 0.4f;
        MP_SetMaterial(&material);

        for (unsigned int j = 1; j <= depth; ++j) {
            MP_PushModelMatrix();
            MP_TranslateModelMatrix(path[j].d.x * MP_BLOCK_SIZE, path[j].d.y * MP_BLOCK_SIZE, MP_D_DRAW_PATH_HEIGHT);
            gluSphere(quadratic, 0.5f, 8, 8);
            MP_PopModelMatrix();
        }
    }
}

void MP_RenderPathing(const MP_Unit* unit) {
    // Render pathing debug information for the unit.
    renderPathing(unit);
}
