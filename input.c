#include <SDL.h>

#include "config.h"
#include "input.h"
#include "selection.h"
#include "map.h"
#include "camera.h"

typedef enum {
    MODE_NONE,
    MODE_SELECT,
    MODE_DESELECT
} SelectMode;

static SelectMode mode;
static int start_x, start_y;

void DK_key_down(const SDL_Event* e) {
    switch (e->key.keysym.sym) {
        case SDLK_UP:
            DK_camera_set_direction(DK_CAMD_NORTH);
            break;
        case SDLK_DOWN:
            DK_camera_set_direction(DK_CAMD_SOUTH);
            break;
        case SDLK_LEFT:
            DK_camera_set_direction(DK_CAMD_WEST);
            break;
        case SDLK_RIGHT:
            DK_camera_set_direction(DK_CAMD_EAST);
            break;
    }
}

void DK_key_up(const SDL_Event* e) {
    switch (e->key.keysym.sym) {
        case SDLK_UP:
            DK_camera_unset_direction(DK_CAMD_NORTH);
            break;
        case SDLK_DOWN:
            DK_camera_unset_direction(DK_CAMD_SOUTH);
            break;
        case SDLK_LEFT:
            DK_camera_unset_direction(DK_CAMD_WEST);
            break;
        case SDLK_RIGHT:
            DK_camera_unset_direction(DK_CAMD_EAST);
            break;
    }
}

void DK_mouse_down(const SDL_Event* e) {
    switch (e->button.button) {
        case SDL_BUTTON_WHEELUP:
            DK_camera_zoom_in();
            break;
        case SDL_BUTTON_WHEELDOWN:
            DK_camera_zoom_out();
            break;
        case SDL_BUTTON_LEFT:
        {
            // Find the block we clicked on.
            DK_block_under_cursor(&start_x, &start_y);
            if (DK_block_is_selectable(DK_PLAYER_RED, start_x, start_y)) {
                // OK, if it's selectable, start selection.
                if (DK_block_is_selected(DK_PLAYER_RED, start_x, start_y)) {
                    mode = MODE_DESELECT;
                } else {
                    mode = MODE_SELECT;
                }
                // Don't test for other objects.
                return;
            } else {
                mode = MODE_NONE;
            }

        }
            break;
    }
}

void DK_mouse_up(const SDL_Event* e) {
    switch (e->button.button) {
        case SDL_BUTTON_LEFT:
        {
            if (mode != MODE_NONE) {
                // Find the block we released over.
                int end_x = start_x, end_y = start_y;
                DK_block_under_cursor(&end_x, &end_y);

                if (end_x < start_x) {
                    const int tmp = end_x;
                    end_x = start_x;
                    start_x = tmp;
                }
                if (end_y < start_y) {
                    const int tmp = end_y;
                    end_y = start_y;
                    start_y = tmp;
                }

                int x, y;
                for (x = start_x; x <= end_x; ++x) {
                    for (y = start_y; y <= end_y; ++y) {
                        if (mode == MODE_SELECT) {
                            DK_block_select(DK_PLAYER_RED, x, y);
                        } else if (mode == MODE_DESELECT) {
                            DK_block_deselect(DK_PLAYER_RED, x, y);
                        }
                    }
                }
            }
        }
            break;
    }
}
