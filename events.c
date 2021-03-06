#include "events.h"

#include <SDL/SDL.h>

#include "block.h"
#include "camera.h"
#include "config.h"
#include "cursor.h"
#include "render.h"
#include "selection.h"
#include "unit.h"
#include "map.h"
#include "block_type.h"
#include "hand.h"

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
            MP_DBG_drawTestTexture = 1 - MP_DBG_drawTestTexture;
            break;
        case SDLK_F2:
            MP_DBG_drawPaths = 1 - MP_DBG_drawPaths;
            break;
        case SDLK_F3:
            MP_DBG_drawJobs = 1 - MP_DBG_drawJobs;
            break;
        case SDLK_F4:
        {
            vec2 p = *MP_GetCursor(MP_CURSOR_LEVEL_FLOOR);
            v2idivs(&p, MP_BLOCK_SIZE);
            MP_AddUnit(MP_PLAYER_ONE, MP_GetUnitTypeById(1), &p);
            break;
        }
        case SDLK_F5:
            MP_DBG_deferredBuffer = (MP_DBG_deferredBuffer + 1) % MP_DBG_BUFFER_COUNT;
            break;
        case SDLK_F6:
            MP_DBG_drawPickingMode = 1 - MP_DBG_drawPickingMode;
            break;
        case SDLK_F7:
            MP_DBG_useDeferredShader = 1 - MP_DBG_useDeferredShader;
            break;
        case SDLK_F8:
            MP_DBG_isAIEnabled = 1 - MP_DBG_isAIEnabled;
            break;
        case SDLK_F9:
        {
            MP_Light* light = calloc(1, sizeof (MP_Light));
            light->diffuseColor.c.r = 0.5f;
            light->diffuseColor.c.g = 0.5f;
            light->diffuseColor.c.b = 0.5f;
            light->diffuseRange = 3 * MP_BLOCK_SIZE;
            light->specularColor.c.r = 0.5f;
            light->specularColor.c.g = 0.5f;
            light->specularColor.c.b = 0.5f;
            light->specularRange = 5 * MP_BLOCK_SIZE;
            light->position.d.x = MP_GetCursor(MP_CURSOR_LEVEL_FLOOR)->v[0];
            light->position.d.y = MP_GetCursor(MP_CURSOR_LEVEL_FLOOR)->v[1];
            light->position.d.z = MP_BLOCK_HEIGHT / 2;
            MP_AddLight(light);
            break;
        }
        case SDLK_F10:
            MP_DBG_drawLightVolumes = 1 - MP_DBG_drawLightVolumes;
            break;

        case SDLK_BACKQUOTE: {
            MP_Block* block = MP_GetBlockUnderCursor();
            if (block) {
                MP_SetBlockOwner(block, MP_PLAYER_ONE);
            }
            break;
        }
        case SDLK_1: {
            MP_Block* block = MP_GetBlockUnderCursor();
            if (block) {
                MP_SetBlockType(block, MP_GetBlockTypeById(1));
            }
            break;
        }
        case SDLK_2: {
            MP_Block* block = MP_GetBlockUnderCursor();
            if (block) {
                MP_SetBlockType(block, MP_GetBlockTypeById(2));
            }
            break;
        }
        case SDLK_3: {
            MP_Block* block = MP_GetBlockUnderCursor();
            if (block) {
                MP_SetBlockType(block, MP_GetBlockTypeById(3));
            }
            break;
        }
        case SDLK_4: {
            MP_Block* block = MP_GetBlockUnderCursor();
            if (block) {
                MP_SetBlockType(block, MP_GetBlockTypeById(4));
            }
            break;
        }
        case SDLK_5: {
            MP_Block* block = MP_GetBlockUnderCursor();
            if (block) {
                MP_SetBlockType(block, MP_GetBlockTypeById(5));
            }
            break;
        }
        case SDLK_6: {
            MP_Block* block = MP_GetBlockUnderCursor();
            if (block) {
                MP_SetBlockType(block, MP_GetBlockTypeById(6));
            }
            break;
        }
        case SDLK_7: {
            MP_Block* block = MP_GetBlockUnderCursor();
            if (block) {
                MP_SetBlockType(block, MP_GetBlockTypeById(7));
            }
            break;
        }
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
            if (MP_GetBlockDepthUnderCursor() < MP_GetUnitDepthUnderCursor()) {
                // Block under cursor is closer to camera.
                if (MP_GetBlockUnderCursor()) {
                    // And there actually is a block, begin selecting.
                    MP_BeginSelection();
                }
            } else if (MP_GetUnitUnderCursor()) {
                // Pick up unit.
                MP_Unit* unit = MP_GetUnitUnderCursor();
                if (unit->owner == MP_PLAYER_ONE) {
                    MP_PickUpUnit(unit->owner, unit);
                }
            }
            break;
        case SDL_BUTTON_RIGHT:
            if (MP_DiscardSelection()) {
                // Was selecting, that's it.
                break;
            } else {
                // Not selecting, try to drop something from the hand.
                vec2 position = *MP_GetCursor(MP_CURSOR_LEVEL_FLOOR);
                v2idivs(&position, MP_BLOCK_SIZE);
                MP_DropTopHandEntry(MP_PLAYER_ONE, &position);
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

void MP_Input(void) {
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

#define MP_EVENT_IMPL(NAME, CALL, ...) \
static struct { \
    MP_##NAME##EventCallback* list; \
    unsigned int length; \
    unsigned int capacity; \
} g##NAME##Callbacks = {NULL, 0, 0}; \
void MP_Add##NAME##EventListener(MP_##NAME##EventCallback callback) { \
    if (g##NAME##Callbacks.length >= g##NAME##Callbacks.capacity) { \
        g##NAME##Callbacks.capacity = g##NAME##Callbacks.capacity * 2 + 1; \
        g##NAME##Callbacks.list = (MP_##NAME##EventCallback*)realloc(g##NAME##Callbacks.list, g##NAME##Callbacks.capacity * sizeof(MP_##NAME##EventCallback)); \
    } \
    g##NAME##Callbacks.list[g##NAME##Callbacks.length] = callback; \
    ++g##NAME##Callbacks.length; \
} \
void MP_Dispatch##NAME##Event(__VA_ARGS__) { \
    for (unsigned int i = 0; i < g##NAME##Callbacks.length; ++i) { \
        g##NAME##Callbacks.list[i]CALL; \
    } \
}

MP_EVENT_IMPL(Update, (), void)

MP_EVENT_IMPL(MapChange, (), void)

MP_EVENT_IMPL(PreRender, (), void)

MP_EVENT_IMPL(Render, (), void)

MP_EVENT_IMPL(PostRender, (), void)

MP_EVENT_IMPL(ModelMatrixChanged, (), void)

MP_EVENT_IMPL(UnitAdded, (unit), MP_Unit* unit)

MP_EVENT_IMPL(BlockTypeChanged, (block), MP_Block* block)

MP_EVENT_IMPL(BlockOwnerChanged, (block), MP_Block* block)

MP_EVENT_IMPL(BlockSelectionChanged, (block, player), MP_Block* block, MP_Player player)

#undef MP_EVENT_IMPL
