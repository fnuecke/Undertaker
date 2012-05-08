#include "events.h"

#include <SDL/SDL.h>

#include "camera.h"
#include "config.h"
#include "cursor.h"
#include "render.h"
#include "selection.h"
#include "units.h"
#include "map.h"
#include "block_meta.h"

///////////////////////////////////////////////////////////////////////////////
// Event handlers
///////////////////////////////////////////////////////////////////////////////

static void key_down(const SDL_Event* e) {
    switch (e->key.keysym.sym) {
        case SDLK_UP:
            MP_CameraStartScrolling(MP_CAMERA_DIRECTION_NORTH);
            break;
        case SDLK_DOWN:
            MP_CameraStartScrolling(MP_CAMERA_DIRECTION_SOUTH);
            break;
        case SDLK_LEFT:
            MP_CameraStartScrolling(MP_CAMERA_DIRECTION_WEST);
            break;
        case SDLK_RIGHT:
            MP_CameraStartScrolling(MP_CAMERA_DIRECTION_EAST);
            break;

        case SDLK_RETURN:
            if (SDL_GetModState() & (KMOD_LALT | KMOD_RALT)) {

            }
            break;

            // Debugging commands.

        case SDLK_F1:
            MP_d_draw_test_texture = 1 - MP_d_draw_test_texture;
            break;
        case SDLK_F2:
            MP_d_draw_paths = 1 - MP_d_draw_paths;
            break;
        case SDLK_F3:
            MP_d_draw_jobs = 1 - MP_d_draw_jobs;
            break;
        case SDLK_F4:
        {
            vec2 p = {
                {5, 10}
            };
            MP_AddUnit(MP_PLAYER_ONE, MP_GetUnitMeta(1), &p);
            break;
        }
        case SDLK_F5:
            MP_d_draw_deferred = (MP_d_draw_deferred + 1) % MP_D_DISPLAY_MODE_COUNT;
            break;
        case SDLK_F6:
            MP_d_draw_picking_mode = 1 - MP_d_draw_picking_mode;
            break;
        case SDLK_F7:
            MP_d_draw_deferred_shader = 1 - MP_d_draw_deferred_shader;
            break;
        case SDLK_F8:
            MP_d_ai_enabled = 1 - MP_d_ai_enabled;
            break;
        case SDLK_F9:
        {
            MP_Light* light = calloc(1, sizeof (MP_Light));
            light->diffuseColor.c.r = 1;
            light->diffuseColor.c.g = 1;
            light->diffuseColor.c.b = 1;
            light->diffusePower = 40;
            light->specularColor.c.r = 1;
            light->specularColor.c.g = 1;
            light->specularColor.c.b = 1;
            light->specularPower = 40;
            light->position.d.x = MP_GetCursor(MP_CURSOR_LEVEL_FLOOR)->v[0];
            light->position.d.y = MP_GetCursor(MP_CURSOR_LEVEL_FLOOR)->v[1];
            light->position.d.z = MP_BLOCK_HEIGHT / 2;
            MP_AddLight(light);
            break;
        }
        case SDLK_F10:
            MP_d_draw_light_volumes = 1 - MP_d_draw_light_volumes;
            break;

        case SDLK_1:
            MP_SetBlockMeta(MP_GetBlockUnderCursor(NULL, NULL), MP_GetBlockMeta(1));
            break;
        case SDLK_2:
            MP_SetBlockMeta(MP_GetBlockUnderCursor(NULL, NULL), MP_GetBlockMeta(2));
            break;
        case SDLK_3:
            MP_SetBlockMeta(MP_GetBlockUnderCursor(NULL, NULL), MP_GetBlockMeta(3));
            break;
        case SDLK_4:
            MP_SetBlockMeta(MP_GetBlockUnderCursor(NULL, NULL), MP_GetBlockMeta(4));
            break;
        case SDLK_5:
            MP_SetBlockMeta(MP_GetBlockUnderCursor(NULL, NULL), MP_GetBlockMeta(5));
            break;
        case SDLK_6:
            MP_SetBlockMeta(MP_GetBlockUnderCursor(NULL, NULL), MP_GetBlockMeta(6));
            break;
        case SDLK_7:
            MP_SetBlockMeta(MP_GetBlockUnderCursor(NULL, NULL), MP_GetBlockMeta(7));
            break;
        default:
            break;
    }
}

static void key_up(const SDL_Event* e) {
    switch (e->key.keysym.sym) {
        case SDLK_UP:
            MP_CameraStopScrolling(MP_CAMERA_DIRECTION_NORTH);
            break;
        case SDLK_DOWN:
            MP_CameraStopScrolling(MP_CAMERA_DIRECTION_SOUTH);
            break;
        case SDLK_LEFT:
            MP_CameraStopScrolling(MP_CAMERA_DIRECTION_WEST);
            break;
        case SDLK_RIGHT:
            MP_CameraStopScrolling(MP_CAMERA_DIRECTION_EAST);
            break;
        default:
            break;
    }
}

static void mouse_down(const SDL_Event* e) {
    switch (e->button.button) {
        case SDL_BUTTON_WHEELUP:
            MP_CameraZoomIn();
            break;
        case SDL_BUTTON_WHEELDOWN:
            MP_CameraZoomOut();
            break;
        case SDL_BUTTON_LEFT:
            MP_BeginSelection();
            break;
        case SDL_BUTTON_RIGHT:
            if (MP_DiscardSelection()) {
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
            MP_ConfirmSelection();
            break;
        default:
            break;
    }
}

///////////////////////////////////////////////////////////////////////////////
// Header implementation
///////////////////////////////////////////////////////////////////////////////

void MP_Events(void) {
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
