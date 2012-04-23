#include <stdio.h>

#include <SDL/SDL.h>
#include <GL/glew.h>

#include "config.h"
#include "init.h"
#include "map.h"
#include "render.h"
#include "selection.h"
#include "textures.h"
#include "units.h"

static void shutdown(void) {
    fprintf(DK_log_target, "INFO: Game shutting down...\n");
    DK_save_config();
}

void DK_init(void) {
    SDL_Surface* screen;

    DK_load_config();

    fprintf(DK_log_target, "------------------------------------------------------------------------\n");
    fprintf(DK_log_target, "INFO: Game starting up...\n");

    atexit(shutdown);

    // Set up SDL.
    if (SDL_Init(SDL_INIT_AUDIO | SDL_INIT_VIDEO) < 0) {
        fprintf(DK_log_target, "ERROR: Unable to initialize SDL: %s\n", SDL_GetError());
        exit(EXIT_FAILURE);
    }
    atexit(SDL_Quit);

    fprintf(DK_log_target, "INFO: SDL initialized successfully.\n");

    // Set up OpenGL related settings.
    SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 5);
    SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 5);
    SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 5);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_SWAP_CONTROL, 0);
    if (DK_use_antialiasing) {
        SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
        SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 4);
    }

    fprintf(DK_log_target, "INFO: Done configuring SDL.\n");

    // Set up video.
    screen = SDL_SetVideoMode(DK_resolution_x, DK_resolution_y, 0, SDL_HWSURFACE | SDL_OPENGL);
    if (screen == NULL) {
        fprintf(DK_log_target, "ERROR: Unable to set video: %s.\n", SDL_GetError());
        exit(EXIT_FAILURE);
    }

    // Set window title.
    SDL_WM_SetCaption("Undertaker", NULL);

    fprintf(DK_log_target, "INFO: Video set up successfully.\n");

    if (glewInit()) {
        fprintf(DK_log_target, "ERROR: Unable to initialize GLEW (%d).\n", glGetError());
        exit(EXIT_FAILURE);
    }

    fprintf(DK_log_target, "INFO: GLEW initialized successfully\n");

    if (!GLEW_VERSION_2_0) {
        fprintf(DK_log_target, "ERROR: OpenGL 2.0 not supported.\n");
        exit(EXIT_FAILURE);
    }
    fprintf(DK_log_target, "INFO: OpenGL 2.0 supported.\n");

    // Load all textures we may need.
    DK_load_textures();

    fprintf(DK_log_target, "INFO: Done loading textures.\n");

    // Initialize openGL.
    DK_init_gl();

    fprintf(DK_log_target, "INFO: Done initializing OpenGL.\n");

    // Initialize a test map.
    DK_map_load("test");
}

