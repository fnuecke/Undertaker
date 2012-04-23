#include <SDL.h>

#include "camera.h"
#include "config.h"
#include "input.h"
#include "selection.h"
#include "units.h"

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
        default:
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
        case SDLK_F1:
            DK_d_draw_test_texture = 1 - DK_d_draw_test_texture;
            break;
        case SDLK_F2:
            DK_d_draw_paths = 1 - DK_d_draw_paths;
            break;
        case SDLK_F3:
            DK_d_draw_jobs = 1 - DK_d_draw_jobs;
            break;
        case SDLK_F4:
            DK_add_unit(DK_PLAYER_RED, DK_UNIT_IMP, 5, 10);
            break;
        default:
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
            DK_selection_begin();
            break;
        case SDL_BUTTON_RIGHT:
            if (DK_selection_cancel()) {
                break;
            }
            break;
        default:
            break;
    }
}

void DK_mouse_up(const SDL_Event* e) {
    switch (e->button.button) {
        case SDL_BUTTON_LEFT:
            DK_selection_end();
            break;
        default:
            break;
    }
}
