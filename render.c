#include <SDL/SDL.h>
#include <GL/glew.h>

#include "camera.h"
#include "config.h"
#include "cursor.h"
#include "jobs.h"
#include "map.h"
#include "render.h"
#include "textures.h"
#include "units.h"

///////////////////////////////////////////////////////////////////////////////
// Helper methods
///////////////////////////////////////////////////////////////////////////////

static void init_lighting(void) {
    // Light colors.
    GLfloat darkness[] = {0.0f, 0.0f, 0.0f, 1.0f};
    GLfloat ambient_global[] = {0.1f, 0.1f, 0.1f, 1.0f};
    GLfloat ambient_directed[] = {0.1f, 0.06f, 0.06f, 1.0f};
    GLfloat hand_diffuse[] = {0.5f, 0.4f, 0.3f, 1.0f};

    // And directions.
    GLfloat ambient_direction[] = {-1.0f, 1.0f, 1.0f, 0.0f};

    // Set up global ambient lighting.
    glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, 1);
    glEnable(GL_LIGHTING);
    glShadeModel(GL_SMOOTH);
    glEnable(GL_COLOR_MATERIAL);

    // Ambient overall light.
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, ambient_global);

    //Add ambient directed light.
    glLightfv(GL_LIGHT0, GL_AMBIENT, darkness);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, ambient_directed);
    glLightfv(GL_LIGHT0, GL_AMBIENT, darkness);
    glLightfv(GL_LIGHT0, GL_POSITION, ambient_direction);
    glEnable(GL_LIGHT0);

    // Assign created components to GL_LIGHT0
    glLightfv(GL_LIGHT1, GL_AMBIENT, darkness);
    glLightfv(GL_LIGHT1, GL_DIFFUSE, hand_diffuse);
    glLightfv(GL_LIGHT1, GL_SPECULAR, darkness);
    glLightf(GL_LIGHT1, GL_CONSTANT_ATTENUATION, 0.0f);
    glLightf(GL_LIGHT1, GL_QUADRATIC_ATTENUATION, 0.00125f);
    glEnable(GL_LIGHT1);
}

///////////////////////////////////////////////////////////////////////////////
// Header implementation
///////////////////////////////////////////////////////////////////////////////

void DK_init_gl(void) {
    // Clear to black.
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClearDepth(1.0f);

    // Define our viewport as the size of our window.
    glViewport(0, 0, DK_resolution_x, DK_resolution_y);

    // Set our projection matrix to match our viewport.
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(DK_field_of_view, DK_ASPECT_RATIO, 0.1, 1000.0);

    // Anti-alias?
    if (DK_use_antialiasing) {
        glEnable(GL_MULTISAMPLE);
    }

    // Initialize the model/view matrix.
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    // We do use textures, so enable that.
    glEnable(GL_TEXTURE_2D);

    // Also enable depth testing to get stuff in the right order.
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glDepthMask(GL_TRUE);

    // We'll make sure stuff is rotated correctly, so we can cull back-faces.
    glEnable(GL_CULL_FACE);
    glFrontFace(GL_CCW);

    init_lighting();

    if (DK_use_fog) {
        GLfloat fog_color[] = {0.0f, 0.0f, 0.0f, 1.0f};
        glFogi(GL_FOG_MODE, GL_LINEAR);
        glFogfv(GL_FOG_COLOR, fog_color);
        glFogf(GL_FOG_DENSITY, 0.1f);
        glHint(GL_FOG_HINT, GL_DONT_CARE);
        glFogf(GL_FOG_START, DK_CAMERA_HEIGHT + DK_BLOCK_SIZE);
        glFogf(GL_FOG_END, DK_CAMERA_HEIGHT + DK_BLOCK_SIZE * 4);
        glEnable(GL_FOG);
    }

    // Load all textures we may need.
    DK_opengl_textures();
}

void DK_render(void) {
    float x, y;

    // Clear screen.
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Set "camera" position.
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    gluLookAt(DK_camera_position()[0], DK_camera_position()[1], DK_CAMERA_HEIGHT - DK_camera_zoom() * DK_CAMERA_MAX_ZOOM,
            DK_camera_position()[0], DK_camera_position()[1] + DK_CAMERA_TARGET_DISTANCE, 0,
            0, 0, 1);

    // Update cursor position (depends on view being set up).
    DK_update_cursor();

    // Get cursor position and update hand light accordingly.
    DK_cursor(&x, &y);
    {
        GLfloat light_position[] = {x, y, DK_BLOCK_HEIGHT * 2, 1};
        glLightfv(GL_LIGHT1, GL_POSITION, light_position);
    }

    // Render game components.
    DK_render_map();
    DK_render_units();
    DK_render_jobs();

    SDL_GL_SwapBuffers();
}
