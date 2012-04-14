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

///////////////////////////////////////////////////////////////////////////////
// Global data
///////////////////////////////////////////////////////////////////////////////

/** Main screen we use */
SDL_Surface* DK_screen;

typedef enum {
    DK_CAMD_NONE = 0,
    DK_CAMD_NORTH = 1,
    DK_CAMD_EAST = 2,
    DK_CAMD_SOUTH = 4,
    DK_CAMD_WEST = 8
} DK_CameraDirection;
DK_CameraDirection DK_camera_direction;
float DK_camera_velocity[] = {0, 0};
float DK_camera_friction = 0;
float DK_camera_position[] = {0, 0};
float DK_camera_zoom = 0;
float DK_camera_zoom_target = 0;

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
    DK_map_size = 16;
    DK_map = calloc(DK_map_size * DK_map_size, sizeof (DK_Block));

    int i, j;
    for (i = 0; i < DK_map_size; ++i) {
        for (j = 0; j < DK_map_size; ++j) {
            if (i == 0 || j == 0 || i == DK_map_size - 1 || j == DK_map_size - 1) {
                DK_map[i + j * DK_map_size].type = DK_BLOCK_ROCK;
            } else {
                DK_map[i + j * DK_map_size].type = DK_BLOCK_DIRT;
            }
        }
    }

    for (i = 0; i < 5; ++i) {
        for (j = 0; j < 5; ++j) {
            DK_map[(4 + i) + (5 + j) * DK_map_size].type = DK_BLOCK_NONE;
        }
    }

    DK_map[7 + 16 * 8].type = DK_BLOCK_WATER;
    DK_map[8 + 16 * 8].type = DK_BLOCK_WATER;
}

void DK_init_gl() {
    // Clear to black.
    glClearColor(0, 0.3f, 0.5f, 0);
    glClearDepth(1.0f);

    // Define our viewport as the size of our window.
    glViewport(0, 0, DK_RESOLUTION_X, DK_RESOLUTION_Y);

    // Set our projection matrix to match our viewport.
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(80.0, DK_ASPECT_RATIO, 1.0, 1000.0);

    // Initialize the model/view matrix.
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glEnable(GL_TEXTURE_2D);

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glDepthMask(GL_TRUE);

    glEnable(GL_CULL_FACE);

    glEnable(GL_MULTISAMPLE);

    // Set up global ambient lighting.
    //glEnable(GL_LIGHTING);
    glShadeModel(GL_SMOOTH);

    // Create light components
    GLfloat ambientLight[] = {0.2f, 0.2f, 0.2f, 1.0f};
    GLfloat diffuseLight[] = {0.8f, 0.8f, 0.8f, 1.0f};
    GLfloat specularLight[] = {0.0f, 0.0f, 0.0f, 1.0f};

    // Assign created components to GL_LIGHT0
    glLightfv(GL_LIGHT0, GL_AMBIENT, ambientLight);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuseLight);
    glLightfv(GL_LIGHT0, GL_SPECULAR, specularLight);

    glLightf(GL_LIGHT0, GL_CONSTANT_ATTENUATION, 0.0f);
    glLightf(GL_LIGHT0, GL_QUADRATIC_ATTENUATION, 0.005f / (DK_UNIT_SCALING * DK_UNIT_SCALING));

    glEnable(GL_LIGHT0);

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
                switch (event.key.keysym.sym) {
                    case SDLK_UP:
                        DK_camera_direction |= DK_CAMD_NORTH;
                        break;
                    case SDLK_DOWN:
                        DK_camera_direction |= DK_CAMD_SOUTH;
                        break;
                    case SDLK_LEFT:
                        DK_camera_direction |= DK_CAMD_WEST;
                        break;
                    case SDLK_RIGHT:
                        DK_camera_direction |= DK_CAMD_EAST;
                        break;
                }
                break;
            case SDL_KEYUP:
                switch (event.key.keysym.sym) {
                    case SDLK_UP:
                        DK_camera_direction &= ~DK_CAMD_NORTH;
                        break;
                    case SDLK_DOWN:
                        DK_camera_direction &= ~DK_CAMD_SOUTH;
                        break;
                    case SDLK_LEFT:
                        DK_camera_direction &= ~DK_CAMD_WEST;
                        break;
                    case SDLK_RIGHT:
                        DK_camera_direction &= ~DK_CAMD_EAST;
                        break;
                }
                break;
            case SDL_MOUSEBUTTONDOWN:
                switch (event.button.button) {
                    case SDL_BUTTON_WHEELUP:
                        DK_camera_zoom_target += DK_CAMERA_ZOOM_STEP;
                        if (DK_camera_zoom_target > DK_CAMERA_MAX_ZOOM) {
                            DK_camera_zoom_target = DK_CAMERA_MAX_ZOOM;
                        }
                        break;
                    case SDL_BUTTON_WHEELDOWN:
                        DK_camera_zoom_target -= DK_CAMERA_ZOOM_STEP;
                        if (DK_camera_zoom_target < 0) {
                            DK_camera_zoom_target = 0;
                        }
                        break;
                }
        }
    }
}

void DK_update_camera() {
    if (DK_camera_direction & DK_CAMD_NORTH) {
        DK_camera_velocity[1] = DK_CAMERA_SPEED;
    }
    if (DK_camera_direction & DK_CAMD_SOUTH) {
        DK_camera_velocity[1] = -DK_CAMERA_SPEED;
    }
    if (DK_camera_direction & DK_CAMD_EAST) {
        DK_camera_velocity[0] = DK_CAMERA_SPEED;
    }
    if (DK_camera_direction & DK_CAMD_WEST) {
        DK_camera_velocity[0] = -DK_CAMERA_SPEED;
    }

    DK_camera_position[0] += DK_camera_velocity[0];
    DK_camera_position[1] += DK_camera_velocity[1];

    if (DK_camera_position[0] < 0) {
        DK_camera_position[0] = 0;
    }
    if (DK_camera_position[0] > DK_map_size * DK_BLOCK_SIZE) {
        DK_camera_position[0] = DK_map_size * DK_BLOCK_SIZE;
    }

    if (DK_camera_position[1] < 0) {
        DK_camera_position[1] = 0;
    }
    if (DK_camera_position[1] > DK_map_size * DK_BLOCK_SIZE) {
        DK_camera_position[1] = DK_map_size * DK_BLOCK_SIZE;
    }

    DK_camera_velocity[0] *= DK_CAMERA_FRICTION;
    DK_camera_velocity[1] *= DK_CAMERA_FRICTION;

    if (fabs(DK_camera_velocity[0]) < 0.0001f) {
        DK_camera_velocity[0] = 0;
    }
    if (fabs(DK_camera_velocity[1]) < 0.0001f) {
        DK_camera_velocity[1] = 0;
    }

    DK_camera_zoom += (DK_camera_zoom_target - DK_camera_zoom) / 2.0f;
}

void DK_update() {
    DK_update_camera();

    // Wait to get a constant frame rate.
    SDL_Delay(16);
}

void DK_render() {
    // Clear screen.
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Set "camera" position.
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    gluLookAt(DK_camera_position[0], DK_camera_position[1], DK_CAMERA_HEIGHT - DK_camera_zoom,
            DK_camera_position[0], DK_camera_position[1] + DK_BLOCK_SIZE * 2, 0,
            0, 0, 1);

    GLfloat light_position[] = {DK_UNIT_SCALING * (160 - 24), DK_UNIT_SCALING * (160 - 24), DK_UNIT_SCALING * 20, 1};
    glLightfv(GL_LIGHT0, GL_POSITION, light_position);

    DK_render_map();

    // Display what we painted.
    SDL_GL_SwapBuffers();
}
