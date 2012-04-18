#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <SDL.h>
#include <GL/gl.h>
#include <GL/glext.h>
#include <GL/glu.h>

#include "noise.h"

#include "config.h"
#include "map.h"
#include "astar.h"
#include "selection.h"
#include "camera.h"
#include "units.h"

///////////////////////////////////////////////////////////////////////////////
// Global data
///////////////////////////////////////////////////////////////////////////////

/** Main screen we use */
SDL_Surface* DK_screen;

/** Whether the game is still running; if set to 0 the game will exit */
int DK_running = 1;

///////////////////////////////////////////////////////////////////////////////
// Main logic
///////////////////////////////////////////////////////////////////////////////

/** Initialize SDL and game data */
void DK_init();

/** (Re)initialize openGL and load textures */
void DK_init_gl();

/** Handle user input */
void DK_events();

/** Update logic */
void DK_update();

/** Render to screen */
void DK_render();

///////////////////////////////////////////////////////////////////////////////
// Program entry
///////////////////////////////////////////////////////////////////////////////

int main(int argc, char** argv) {

    // Initialize basics, such as SDL and data.
    DK_init();

    // Start the main loop.
    while (DK_running) {
        DK_events();
        DK_update();
        DK_render();

        SDL_GL_SwapBuffers();
    }

    return EXIT_SUCCESS;
}

///////////////////////////////////////////////////////////////////////////////
// Main logic implementation
///////////////////////////////////////////////////////////////////////////////

void DK_init() {
    // Set up SDL.
    if (SDL_Init(SDL_INIT_AUDIO | SDL_INIT_VIDEO) < 0) {
        fprintf(stderr, "Unable to init SDL: %s\n", SDL_GetError());
        exit(EXIT_FAILURE);
    }
    atexit(SDL_Quit);

    // Set up OpenGL related settings.
    SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 5);
    SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 5);
    SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 5);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    if (DK_USE_ANTIALIASING) {
        SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
        SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 4);
    }

    // Set up video.
    DK_screen = SDL_SetVideoMode(DK_RESOLUTION_X, DK_RESOLUTION_Y, 16, SDL_HWSURFACE | SDL_OPENGL);
    if (DK_screen == NULL) {
        fprintf(stderr, "Unable to set video: %s\n", SDL_GetError());
        exit(EXIT_FAILURE);
    }

    // Set window title.
    SDL_WM_SetCaption("Undertaker", NULL);

    // Load all textures we may need.
    DK_load_textures();

    // Initialize openGL.
    DK_init_gl();

    // Initialize a test map.
    DK_init_map(32);
    DK_init_selection();
    DK_init_a_star();
    DK_init_units();

    int i, j;
    for (i = 0; i < 7; ++i) {
        for (j = 0; j < 7; ++j) {
            if (i <= 1 || j <= 1) {
                DK_block_at(4 + i, 5 + j)->owner = DK_PLAYER_RED;
            }
            if (i > 0 && j > 0 && i < 6 && j < 6) {
                DK_block_at(4 + i, 5 + j)->type = DK_BLOCK_NONE;
            }
        }
    }

    DK_block_at(7, 8)->type = DK_BLOCK_DIRT;
    DK_block_at(8, 8)->type = DK_BLOCK_DIRT;

    DK_block_at(10, 8)->type = DK_BLOCK_WATER;
    DK_block_at(11, 8)->type = DK_BLOCK_WATER;
    DK_block_at(11, 9)->type = DK_BLOCK_WATER;
    //DK_block_at(9, 8)->owner = DK_PLAYER_RED;
    
    DK_add_unit(DK_PLAYER_RED, DK_UNIT_IMP, 5, 10);
    DK_add_unit(DK_PLAYER_RED, DK_UNIT_IMP, 5, 7);
}

void DK_init_gl() {
    // Clear to black.
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClearDepth(1.0f);

    // Define our viewport as the size of our window.
    glViewport(0, 0, DK_RESOLUTION_X, DK_RESOLUTION_Y);

    // Set our projection matrix to match our viewport.
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(80.0, DK_ASPECT_RATIO, 0.1, 1000.0);

    // Anti-alias?
    if (DK_USE_ANTIALIASING) {
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

    // Set up global ambient lighting.
    glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, 1);
    glEnable(GL_LIGHTING);
    glShadeModel(GL_SMOOTH);
    glEnable(GL_COLOR_MATERIAL);

    GLfloat global_ambient[] = {0.0f, 0.0f, 0.0f, 1.0f};
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, global_ambient);

    // Create light components
    GLfloat ambientLight[] = {0.0f, 0.0f, 0.0f, 1.0f};
    GLfloat diffuseLight[] = {1.0f, 0.9f, 0.8f, 1.0f};
    GLfloat specularLight[] = {0.0f, 0.0f, 0.0f, 1.0f};

    // Assign created components to GL_LIGHT0
    glLightfv(GL_LIGHT0, GL_AMBIENT, ambientLight);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuseLight);
    glLightfv(GL_LIGHT0, GL_SPECULAR, specularLight);

    glLightf(GL_LIGHT0, GL_CONSTANT_ATTENUATION, 0.0f);
    glLightf(GL_LIGHT0, GL_QUADRATIC_ATTENUATION, 0.0025f);

    glEnable(GL_LIGHT0);

    //Add directed light
    GLfloat lightColor1[] = {0.3f, 0.25f, 0.25f, 1.0f};
    GLfloat lightPos1[] = {-1.0f, 0.5f, 0.5f, 0.0f};
    glLightfv(GL_LIGHT1, GL_DIFFUSE, lightColor1);
    glLightfv(GL_LIGHT1, GL_POSITION, lightPos1);
    glEnable(GL_LIGHT1);

    GLfloat fog_color[] = {0.0f, 0.0f, 0.0f, 1.0f};
    glFogi(GL_FOG_MODE, GL_LINEAR);
    glFogfv(GL_FOG_COLOR, fog_color);
    glFogf(GL_FOG_DENSITY, 0.1f);
    glHint(GL_FOG_HINT, GL_DONT_CARE);
    glFogf(GL_FOG_START, DK_CAMERA_HEIGHT + DK_BLOCK_SIZE);
    glFogf(GL_FOG_END, DK_CAMERA_HEIGHT + DK_BLOCK_SIZE * 4);
    glEnable(GL_FOG);

    // Load all textures we may need.
    DK_opengl_textures();
}

void DK_events() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_QUIT:
                exit(EXIT_SUCCESS);
                break;
            case SDL_KEYDOWN:
                DK_key_down(&event);
                break;
            case SDL_KEYUP:
                DK_key_up(&event);
                break;
            case SDL_MOUSEBUTTONDOWN:
                DK_mouse_down(&event);
                break;
            case SDL_MOUSEBUTTONUP:
                DK_mouse_up(&event);
                break;
        }
    }
}

void DK_update() {
    DK_update_camera();
    DK_update_units();

    // Wait to get a constant frame rate.
    SDL_Delay(1000/60);
}

void DK_render() {
    // Clear screen.
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Set "camera" position.
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    gluLookAt(DK_camera_position()[0], DK_camera_position()[1], DK_CAMERA_HEIGHT - DK_camera_zoom() * DK_CAMERA_MAX_ZOOM,
            DK_camera_position()[0], DK_camera_position()[1] + DK_CAMERA_TARGET_DISTANCE, 0,
            0, 0, 1);

    GLfloat light_position[] = {160 - 24, 160 - 24, 20, 1};
    glLightfv(GL_LIGHT0, GL_POSITION, light_position);

    DK_render_map();
    DK_render_units();
}
