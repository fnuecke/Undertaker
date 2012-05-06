#include <stdlib.h>

#include <SDL/SDL.h>

#include "config.h"
#include "init.h"
#include "events.h"
#include "render.h"
#include "timer.h"
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
    int delay;

    // Initialize basics, such as SDL and data.
    DK_Init();
    T_Init();

    // Start the main loop.
    while (running) {
        T_Start();

        DK_Events();
        DK_Update();
        DK_Render();

        SDL_GL_SwapBuffers();

        T_Stop();

        // Wait to get a constant frame rate.
        delay = 1000.0f / DK_FRAMERATE - T_GetElapsedTimeInMilliSec();
        if (delay > 0) {
            load_accumulator += T_GetElapsedTimeInMicroSec();
            if (load_counter++ > DK_FRAMERATE) {
                char title[32] = {0};
                sprintf(title, "Undertaker - Load: %.2f", load_accumulator / 1000000.0f);
                SDL_WM_SetCaption(title, NULL);
                load_counter = 0;
                load_accumulator = 0;
            }
            SDL_Delay(delay);
        }
    }

    return EXIT_SUCCESS;
}
