#include "unit.h"

#include <float.h>
#include <math.h>
#include <memory.h>
#include <stdlib.h>

#include <GL/glew.h>
#include <SDL/SDL.h>

#include "astar.h"
#include "bitset.h"
#include "block.h"
#include "config.h"
#include "events.h"
#include "graphics.h"
#include "job.h"
#include "log.h"
#include "script.h"
#include "map.h"
#include "picking.h"
#include "render.h"
#include "unit_ai.h"
#include "vmath.h"
#include "ability.h"

///////////////////////////////////////////////////////////////////////////////
// Global variables
///////////////////////////////////////////////////////////////////////////////

/** Units in the game, must be ensured to be non-sparse */
static MP_Unit** gUnits[MP_PLAYER_COUNT];

/** Number of units and list capacity per player */
static unsigned short gUnitCount[MP_PLAYER_COUNT] = {0};
static unsigned short gUnitCapacity[MP_PLAYER_COUNT] = {0};

/** The unit currently under the cursor */
static MP_Unit* gCursorUnit = NULL;

/** Distance of the hovered unit to the camera */
static float gCursorZ = 0;

/** Currently doing a picking (select) render pass? */
static bool gIsPicking = false;

///////////////////////////////////////////////////////////////////////////////
// Allocation
///////////////////////////////////////////////////////////////////////////////

static void ensureUnitListSize(MP_Player player) {
    if (gUnitCount[player] >= gUnitCapacity[player]) {
        gUnitCapacity[player] = gUnitCapacity[player] * 2 + 1;
        if (!(gUnits[player] =
            realloc(gUnits[player], gUnitCapacity[player] * sizeof (MP_Unit**)))) {
            MP_log_fatal("Out of memory while resizing unit list.\n");
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
// Update
///////////////////////////////////////////////////////////////////////////////

static void onUpdate(void) {
    if (MP_DBG_isAIEnabled) {
        for (unsigned int player = 0; player < MP_PLAYER_COUNT; ++player) {
            for (unsigned short unitId = 0; unitId < gUnitCount[player]; ++unitId) {
                // Update the unit's AI state.
                MP_UpdateAI(gUnits[player][unitId]);
                // Update ability cooldowns.
                for (unsigned int i = 0; i < gUnits[player][unitId]->type->abilityCount; ++i) {
                    if (gUnits[player][unitId]->abilities[i].cooldown > 0) {
                        --gUnits[player][unitId]->abilities[i].cooldown;
                    }
                }
            }
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
// Render
///////////////////////////////////////////////////////////////////////////////

/** Used for debug rendering of units and paths */
static GLUquadric* quadratic = 0;

static void onRender(void) {
    MP_Material material;

    if (!quadratic) {
        quadratic = gluNewQuadric();
    }

#define LN(x, y) glLoadName(((unsigned short) (y) << 16) | (unsigned short) (x))
    for (unsigned int player = 0; player < MP_PLAYER_COUNT; ++player) {
        for (unsigned short unitId = 0; unitId < gUnitCount[player]; ++unitId) {
            const MP_Unit* unit = gUnits[player][unitId];

            // Skip while in hand.
            if (unit->ai->isInHand) {
                // TODO render as being held, skip when picking.
                continue;
            }

            // Push name of the unit for picking.
            LN(player, unitId);

            MP_InitMaterial(&material);
            material.specularIntensity = 0.9f;
            material.specularExponent = 25.0f;

            // Set color based on current job.
            material.diffuseColor.c.r = 0.6f;
            material.diffuseColor.c.g = 0.6f;
            material.diffuseColor.c.b = 0.6f;
            MP_SetMaterial(&material);

            // Render the unit.
            MP_PushModelMatrix();
            MP_TranslateModelMatrix(unit->position.d.x * MP_BLOCK_SIZE,
                                    unit->position.d.y * MP_BLOCK_SIZE, 4);
            gluSphere(quadratic, MP_BLOCK_SIZE / 6.0f, 16, 16);
            MP_PopModelMatrix();

            // Render debug AI information.
            if (!gIsPicking) {
                MP_RenderPathing(unit);
            }
        }
    }
#undef LN
}

static void onPreRender(void) {
    int mouseX, mouseY;
    GLuint name;

    SDL_GetMouseState(&mouseX, &mouseY);

    gIsPicking = true;
    if (MP_Pick(mouseX, MP_resolutionY - mouseY, &onRender, &name, &gCursorZ)) {
        const MP_Player player = (short) (name & 0xFFFF);
        const unsigned int unitId = (short) (name >> 16);
        gCursorUnit = gUnits[player][unitId];
    } else {
        gCursorUnit = NULL;
        gCursorZ = FLT_MAX;
    }
    gIsPicking = false;
}

///////////////////////////////////////////////////////////////////////////////
// Header implementation
///////////////////////////////////////////////////////////////////////////////

bool MP_IsUnitMoving(const MP_Unit* unit) {
    return unit->ai->pathing.index <= unit->ai->pathing.depth;
}

MP_Unit* MP_GetUnitUnderCursor(void) {
    return gCursorUnit;
}

float MP_GetUnitDepthUnderCursor(void) {
    return gCursorZ;
}

MP_Unit* MP_AddUnit(MP_Player player, const MP_UnitType* meta, const vec2* position) {
    lua_State* L = MP_Lua();
    MP_Unit* unit;

    // Can that kind of unit be spawned at the specified position? Also checks
    // for meta validity, implicitly.
    if (!MP_IsBlockPassableBy(MP_GetBlockAt((int) floorf(position->d.x), (int) floorf(position->d.y)), meta)) {
        return NULL;
    }

    // Ensure we have the capacity to add the unit.
    ensureUnitListSize(player);

    // Allocate the actual unit.
    if (!(unit = calloc(1, sizeof (MP_Unit)))) {
        MP_log_fatal("Out of memory while allocating a job.\n");
    }
    unit->type = meta;
    unit->owner = player;
    unit->position = *position;

    // Allocate ability list.
    if (!(unit->abilities = calloc(unit->type->abilityCount, sizeof (MP_Ability)))) {
        MP_log_fatal("Out of memory while allocating unit abilities.\n");
    }
    
    // Create shallow copy of ability property list.
    for (unsigned int number = 0; number < unit->type->abilityCount; ++number) {
        unit->abilities[number].type = unit->type->abilities[number].type;
        unit->abilities[number].unit = unit;
        lua_rawgeti(L, LUA_REGISTRYINDEX, unit->type->abilities[number].properties);
        lua_newtable(L);
        lua_pushnil(L);
        while (lua_next(L, -3)) {
            lua_pushvalue(L, -2); // duplicate key
            lua_insert(L, -2); // and move it in front of our value
            lua_settable(L, -4); // then add it to the new table
        }
        unit->abilities[number].properties = luaL_ref(L, LUA_REGISTRYINDEX);
    }
    
    // Allocate job saturation memory.
    if (!(unit->jobSaturation = calloc(unit->type->jobCount, sizeof (float)))) {
        MP_log_fatal("Out of memory while allocating job saturation.\n");
    }

    // Set initial job saturations.
    for (unsigned int number = 0; number < unit->type->jobCount; ++number) {
        unit->jobSaturation[number] = unit->type->jobs[number].initialSaturation;
    }

    // OK, allocate AI data, if necessary.
    if (!(unit->ai = calloc(1, sizeof (MP_AI_Info)))) {
        MP_log_fatal("Out of memory while allocating AI info.\n");
    }

    // Disable movement initially.
    unit->ai->pathing.index = 1;

    // Store it in our list.
    gUnits[player][gUnitCount[player]++] = unit;

    // Send event to AI scripts.
    MP_DispatchUnitAddedEvent(unit);

    return unit;
}

void MP_StopJob(MP_Job* job) {
    if (job && job->worker) {
        AI_State* state = &job->worker->ai->state;
        state->job = NULL;
        state->jobRunDelay = 0;
        state->jobSearchDelay = 0;
        state->active = false;
        job->worker = NULL;
    }
}

///////////////////////////////////////////////////////////////////////////////
// Initialization / Cleanup
///////////////////////////////////////////////////////////////////////////////

void MP_ClearUnits(void) {
    for (unsigned int player = 0; player < MP_PLAYER_COUNT; ++player) {
        for (unsigned short unitId = 0; unitId < gUnitCount[player]; ++unitId) {
            free(gUnits[player][unitId]);
            gUnits[player][unitId] = NULL;
        }
        gUnitCount[player] = 0;

        free(gUnits[player]);
        gUnits[player] = NULL;
        gUnitCapacity[player] = 0;
    }

    gCursorUnit = NULL;
    gCursorZ = 0;
}

void MP_InitUnits(void) {
    MP_AddUpdateEventListener(onUpdate);
    MP_AddPreRenderEventListener(onPreRender);
    MP_AddRenderEventListener(onRender);

    memset(gUnits, 0, MP_PLAYER_COUNT * sizeof (MP_Unit**));
    memset(gUnitCount, 0, MP_PLAYER_COUNT * sizeof (unsigned short));
    memset(gUnitCapacity, 0, MP_PLAYER_COUNT * sizeof (unsigned short));
}
