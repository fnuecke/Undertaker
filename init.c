#include "init.h"

#include <stdio.h>

#include <SDL/SDL.h>
#include <GL/glew.h>

#include "astar.h"
#include "block.h"
#include "camera.h"
#include "config.h"
#include "graphics.h"
#include "map.h"
#include "map_loader.h"
#include "render.h"
#include "selection.h"
#include "textures.h"
#include "units.h"
#include "camera.h"
#include "cursor.h"
#include "jobs.h"
#include "room_meta.h"

static void shutdown(void) {
    fprintf(MP_log_target, "INFO: Game shutting down...\n");
    MP_save_config();
}

void MP_Init(void) {
    SDL_Surface* screen;

    MP_load_config();

    fprintf(MP_log_target, "------------------------------------------------------------------------\n");
    fprintf(MP_log_target, "INFO: Game starting up...\n");

    atexit(shutdown);

    // Set up SDL.
    if (SDL_Init(SDL_INIT_AUDIO | SDL_INIT_VIDEO) < 0) {
        fprintf(MP_log_target, "ERROR: Unable to initialize SDL: %s\n", SDL_GetError());
        exit(EXIT_FAILURE);
    }
    atexit(SDL_Quit);

    fprintf(MP_log_target, "INFO: SDL initialized successfully.\n");

    // Set up OpenGL related settings.
    SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 5);
    SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 5);
    SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 5);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 1);
    SDL_GL_SetAttribute(SDL_GL_SWAP_CONTROL, 0);
    if (MP_use_antialiasing) {
        SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
        SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 4);
    }

    fprintf(MP_log_target, "INFO: Done configuring SDL.\n");

    // Set up video.
    screen = SDL_SetVideoMode(MP_resolution_x, MP_resolution_y, 0, SDL_HWSURFACE | SDL_OPENGL);
    if (screen == NULL) {
        fprintf(MP_log_target, "ERROR: Unable to set video: %s.\n", SDL_GetError());
        exit(EXIT_FAILURE);
    }

    // Set window title.
    SDL_WM_SetCaption("Undertaker", NULL);

    fprintf(MP_log_target, "INFO: Video set up successfully.\n");

    if (glewInit()) {
        fprintf(MP_log_target, "ERROR: Unable to initialize GLEW (%d).\n", glGetError());
        exit(EXIT_FAILURE);
    }

    fprintf(MP_log_target, "INFO: GLEW initialized successfully\n");

    if (!GLEW_VERSION_3_3) {
        fprintf(MP_log_target, "ERROR: OpenGL 3.3 not supported.\n");
        exit(EXIT_FAILURE);
    }
    fprintf(MP_log_target, "INFO: OpenGL 3.3 supported.\n");

    // Initialize openGL.
    MP_InitRender();

    fprintf(MP_log_target, "INFO: Done initializing OpenGL.\n");

    // Set up event bindings.
    MP_InitGraphics();

    MP_InitAStar();
    MP_InitCamera();
    MP_InitCursor();
    MP_InitSelection();
    MP_InitUnits();
    MP_InitMap();
    MP_InitJobs();

    fprintf(MP_log_target, "INFO: Done initializing internal hooks.\n");

    // Initialize a test map.
    MP_LoadMap("defaults");
}
