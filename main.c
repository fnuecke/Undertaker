#include <stdlib.h>

#include <SDL/SDL.h>

#include "config.h"
#include "init.h"
#include "events.h"
#include "render.h"
#include "timer.h"

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
    MP_Init();
    T_Init();

    // Start the main loop.
    while (running) {
        T_Start();

        MP_Input();
        MP_DispatchUpdateEvent();
        MP_Render();

        SDL_GL_SwapBuffers();

        T_Stop();

        load_accumulator += T_GetElapsedTimeInMicroSec();
        if (load_counter++ > MP_FRAMERATE) {
            char title[128] = {0};
            snprintf(title, sizeof (title), "Undertaker - Load: %.2f - Lights: %d", load_accumulator / 1000000.0f, MP_DEBUG_VisibleLightCount());
            SDL_WM_SetCaption(title, NULL);
            load_counter = 0;
            load_accumulator = 0;
        }

        // Wait to get a constant frame rate.
        delay = 1000.0f / MP_FRAMERATE - T_GetElapsedTimeInMilliSec();
        if (delay > 0) {
            SDL_Delay(delay);
        }
    }

    return EXIT_SUCCESS;
}
