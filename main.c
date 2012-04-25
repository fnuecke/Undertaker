#include <stdlib.h>

#include <SDL/SDL.h>

#include "config.h"
#include "init.h"
#include "events.h"
#include "render.h"
#include "update.h"

///////////////////////////////////////////////////////////////////////////////
// Global data
///////////////////////////////////////////////////////////////////////////////

/** Accumulated load since last output */
static float load_accumulator = 0;

/** Number of accumulated frame loads */
static int load_counter = 0;

/** Whether the game is still running; if set to 0 the game will exit */
static int running = 1;

///////////////////////////////////////////////////////////////////////////////
// Program entry
///////////////////////////////////////////////////////////////////////////////

int main(int argc, char** argv) {
    Uint32 start, end;
    int delay;

    // Initialize basics, such as SDL and data.
    DK_Init();

    // Start the main loop.
    while (running) {
        start = SDL_GetTicks();

        DK_Events();
        DK_Update();
        DK_render();

        SDL_GL_SwapBuffers();

        end = SDL_GetTicks();

        // Wait to get a constant frame rate.
        delay = 1000.0f / DK_FRAMERATE - (end - start);
        if (delay > 0) {
            load_accumulator += (end - start) * DK_FRAMERATE / 1000.0f;
            if (load_counter++ > DK_FRAMERATE) {
                char title[32] = {0};
                sprintf(title, "Undertaker - Load: %.2f", load_accumulator / DK_FRAMERATE);
                SDL_WM_SetCaption(title, NULL);
                load_counter = 0;
                load_accumulator = 0;
            }
            SDL_Delay(delay);
        }
    }

    return EXIT_SUCCESS;
}
