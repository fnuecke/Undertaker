#include <SDL/SDL.h>

#include "camera.h"
#include "config.h"
#include "events.h"
#include "selection.h"
#include "units.h"

///////////////////////////////////////////////////////////////////////////////
// Event handlers
///////////////////////////////////////////////////////////////////////////////

static void key_down(const SDL_Event* e) {
    switch (e->key.keysym.sym) {
        case SDLK_UP:
            DK_CameraStartScrolling(DK_CAMERA_DIRECTION_NORTH);
            break;
        case SDLK_DOWN:
            DK_CameraStartScrolling(DK_CAMERA_DIRECTION_SOUTH);
            break;
        case SDLK_LEFT:
            DK_CameraStartScrolling(DK_CAMERA_DIRECTION_WEST);
            break;
        case SDLK_RIGHT:
            DK_CameraStartScrolling(DK_CAMERA_DIRECTION_EAST);
            break;
        default:
            break;
    }
}

static void key_up(const SDL_Event* e) {
    switch (e->key.keysym.sym) {
        case SDLK_UP:
            DK_CameraStopScrolling(DK_CAMERA_DIRECTION_NORTH);
            break;
        case SDLK_DOWN:
            DK_CameraStopScrolling(DK_CAMERA_DIRECTION_SOUTH);
            break;
        case SDLK_LEFT:
            DK_CameraStopScrolling(DK_CAMERA_DIRECTION_WEST);
            break;
        case SDLK_RIGHT:
            DK_CameraStopScrolling(DK_CAMERA_DIRECTION_EAST);
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
            DK_AddUnit(DK_PLAYER_ONE, DK_UNIT_IMP, 5, 10);
            break;
        case SDLK_F5:
            DK_d_draw_deferred = (DK_d_draw_deferred + 1) % DK_D_DEFERRED_COUNT;
            break;
        case SDLK_F6:
            DK_d_draw_picking_mode = 1 - DK_d_draw_picking_mode;
            break;
        case SDLK_F7:
            DK_d_draw_deferred_shader = 1 - DK_d_draw_deferred_shader;
            break;
        default:
            break;
    }
}

static void mouse_down(const SDL_Event* e) {
    switch (e->button.button) {
        case SDL_BUTTON_WHEELUP:
            DK_CameraZoomIn();
            break;
        case SDL_BUTTON_WHEELDOWN:
            DK_CameraZoomOut();
            break;
        case SDL_BUTTON_LEFT:
            DK_BeginSelection();
            break;
        case SDL_BUTTON_RIGHT:
            if (DK_DiscardSelection()) {
                break;
            }
            break;
        default:
            break;
    }
}

static void mouse_up(const SDL_Event* e) {
    switch (e->button.button) {
        case SDL_BUTTON_LEFT:
            DK_ConfirmSelection();
            break;
        default:
            break;
    }
}

///////////////////////////////////////////////////////////////////////////////
// Header implementation
///////////////////////////////////////////////////////////////////////////////

void DK_Events(void) {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_QUIT:
                exit(EXIT_SUCCESS);
                break;
            case SDL_KEYDOWN:
                key_down(&event);
                break;
            case SDL_KEYUP:
                key_up(&event);
                break;
            case SDL_MOUSEBUTTONDOWN:
                mouse_down(&event);
                break;
            case SDL_MOUSEBUTTONUP:
                mouse_up(&event);
                break;
            default:
                break;
        }
    }
}
