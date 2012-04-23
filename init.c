#include <stdio.h>

#include <SDL/SDL.h>
#include <GL/glew.h>

#include "astar.h"
#include "config.h"
#include "init.h"
#include "map.h"
#include "render.h"
#include "selection.h"
#include "textures.h"
#include "units.h"

void DK_init(void) {
    int i, j;
    SDL_Surface* screen;

    fprintf(stdout, "------------------------------------------------------------\n");

    fprintf(stdout, "Initializing SDL...\n");

    // Set up SDL.
    if (SDL_Init(SDL_INIT_AUDIO | SDL_INIT_VIDEO) < 0) {
        fprintf(stderr, "Unable to init SDL: %s\n", SDL_GetError());
        exit(EXIT_FAILURE);
    }
    atexit(SDL_Quit);

    fprintf(stdout, "SDL initialized successfully, setting up...\n");

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


    fprintf(stdout, "Starting up video...\n");

    // Set up video.
    screen = SDL_SetVideoMode(DK_resolution_x, DK_resolution_y, 16, SDL_HWSURFACE | SDL_OPENGL);
    if (screen == NULL) {
        fprintf(stderr, "Unable to set video: %s\n", SDL_GetError());
        exit(EXIT_FAILURE);
    }

    //fprintf(stdout, "Successfully started up video, querying capabilities...\n");

    // Set window title.
    SDL_WM_SetCaption("Undertaker", NULL);

    fprintf(stdout, "Loading textures...\n");

    // Load all textures we may need.
    DK_load_textures();

    fprintf(stdout, "Initializing OpenGL...\n");

    // Initialize openGL.
    DK_init_gl();

    // Initialize a test map.
    DK_init_map(128);
    DK_init_selection();
    DK_init_a_star();
    DK_init_units();

    for (i = 0; i < 7; ++i) {
        for (j = 0; j < 7; ++j) {
            if (i <= 1 || j <= 1) {
                DK_block_set_owner(DK_block_at(4 + i, 5 + j), DK_PLAYER_RED);
            }
            if (i > 0 && j > 0 && i < 6 && j < 6) {
                DK_block_set_type(DK_block_at(4 + i, 5 + j), DK_BLOCK_NONE);
            }
        }
    }

    DK_block_set_type(DK_block_at(7, 8), DK_BLOCK_DIRT);
    DK_block_set_type(DK_block_at(8, 8), DK_BLOCK_DIRT);

    DK_block_set_type(DK_block_at(10, 8), DK_BLOCK_WATER);
    DK_block_set_type(DK_block_at(11, 8), DK_BLOCK_WATER);
    DK_block_set_type(DK_block_at(11, 9), DK_BLOCK_WATER);
    //DK_block_at(9, 8)->owner = DK_PLAYER_RED;

    for (i = 0; i < 2; ++i) {
        DK_add_unit(DK_PLAYER_RED, DK_UNIT_IMP, 5, 10);
    }
}

