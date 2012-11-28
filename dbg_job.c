#include "config.h"
#include "events.h"
#include "job.h"
#include "job_type.h"
#include "render.h"
#include "vmath.h"

///////////////////////////////////////////////////////////////////////////////
// Rendering
///////////////////////////////////////////////////////////////////////////////

static const vec4 colors[] = {
    {
        {1, 0, 0, 1}
    },
    {
        {0, 1, 0, 1}
    },
    {
        {0, 0, 1, 1}
    },
    {
        {0.9f, 0.3f, 0.3f, 1}
    },
    {
        {0.3f, 0.9f, 0.3f, 1}
    },
    {
        {0.3f, 0.3f, 0.9f, 1}
    },
    {
        {0.8f, 0.5f, 0.5f, 1}
    },
    {
        {0.5f, 0.8f, 0.5f, 1}
    },
    {
        {0.5f, 0.5f, 0.8f, 1}
    },
};
static const unsigned int colorCount = sizeof (colors) / sizeof (vec4);

/** Debug rendering of where jobs are */
static void onRender(void) {
    if (MP_DBG_drawJobs) {
        MP_Material material;
        MP_InitMaterial(&material);
        material.emissivity = 1.0f;

        for (unsigned int typeId = 0; typeId < MP_GetJobTypeCount(); ++typeId) {
            unsigned int count;
            MP_Job * const* jobs = MP_GetJobs(MP_GetJobTypeById(typeId + 1), MP_PLAYER_ONE, &count);
            for (unsigned int number = 0; number < count; ++number) {
                const MP_Job* job = jobs[number];

                // Pick a color for the job.
                material.diffuseColor = colors[typeId % colorCount];
                /*
                                switch (type) {
                                    case MP_JOB_DIG:
                                        material.diffuseColor.c.r = 0.4f;
                                        material.diffuseColor.c.g = 0.8f;
                                        material.diffuseColor.c.b = 0.4f;
                                        break;
                                    case MP_JOB_CONVERT_TILE:
                                    case MP_JOB_CONVERT_WALL:
                                        material.diffuseColor.c.r = 0.4f;
                                        material.diffuseColor.c.g = 0.4f;
                                        material.diffuseColor.c.b = 0.8f;
                                        break;

                                    default:
                                        material.diffuseColor.c.r = 0.4f;
                                        material.diffuseColor.c.g = 0.4f;
                                        material.diffuseColor.c.b = 0.4f;
                                        break;
                                }
                 */

                // Highlight if it's being worked on.
                if (job->worker) {
                    material.diffuseColor.c.r += 0.2f;
                    material.diffuseColor.c.g += 0.2f;
                    material.diffuseColor.c.b += 0.2f;
                }

                // Apply material and draw quad.
                MP_SetMaterial(&material);
                glBegin(GL_QUADS);
                {
                    vec2 position = MP_GetJobPosition(job);
                    glVertex3f((position.d.x - 0.1f) * MP_BLOCK_SIZE,
                               (position.d.y - 0.1f) * MP_BLOCK_SIZE,
                               MP_D_DRAW_PATH_HEIGHT + 0.1f);
                    glVertex3f((position.d.x + 0.1f) * MP_BLOCK_SIZE,
                               (position.d.y - 0.1f) * MP_BLOCK_SIZE,
                               MP_D_DRAW_PATH_HEIGHT + 0.1f);
                    glVertex3f((position.d.x + 0.1f) * MP_BLOCK_SIZE,
                               (position.d.y + 0.1f) * MP_BLOCK_SIZE,
                               MP_D_DRAW_PATH_HEIGHT + 0.1f);
                    glVertex3f((position.d.x - 0.1f) * MP_BLOCK_SIZE,
                               (position.d.y + 0.1f) * MP_BLOCK_SIZE,
                               MP_D_DRAW_PATH_HEIGHT + 0.1f);
                }
                glEnd();
            }
        }
    }
}

void MP_Debug_InitJobs(void) {
    MP_AddRenderEventListener(onRender);
}
