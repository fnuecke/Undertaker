#include "ability.h"
#include "ability_type.h"
#include "job_type.h"
#include "script.h"
#include "unit.h"
#include "unit_ai.h"
#include "unit_type.h"

///////////////////////////////////////////////////////////////////////////////
// Getters
///////////////////////////////////////////////////////////////////////////////

static int lua_GetAbility(lua_State* L) {
    const MP_Unit* unit = MP_Lua_CheckUnit(L, 1);
    const MP_AbilityType* ability = MP_Lua_CheckAbilityType(L, 2);

    // Find an ability of that type.
    for (unsigned int i = 0; i < unit->type->abilityCount; ++i) {
        if (unit->abilities[i].type == ability) {
            MP_Lua_PushAbility(L, &unit->abilities[i]);
            return 1;
        }
    }

    // No such ability, push null.
    lua_pushnil(L);

    return 1;
}

static int lua_GetCanPass(lua_State* L) {
    const MP_Unit* unit = MP_Lua_CheckUnit(L, 1);

    lua_pushinteger(L, unit->type->canPass);

    return 1;
}

static int lua_GetJob(lua_State* L) {
    const MP_Unit* unit = MP_Lua_CheckUnit(L, 1);

    MP_Lua_PushJob(L, unit->ai->state.job);

    return 1;
}

static int lua_GetPlayer(lua_State* L) {
    const MP_Unit* unit = MP_Lua_CheckUnit(L, 1);

    lua_pushinteger(L, unit->owner);

    return 1;
}

static int lua_GetPosition(lua_State* L) {
    const MP_Unit* unit = MP_Lua_CheckUnit(L, 1);

    MP_Lua_PushVec2(L, unit->position);

    return 2;
}

static int lua_GetType(lua_State* L) {
    const MP_Unit* unit = MP_Lua_CheckUnit(L, 1);

    lua_pushinteger(L, unit->type->info.id);

    return 1;
}

///////////////////////////////////////////////////////////////////////////////
// Methods
///////////////////////////////////////////////////////////////////////////////

static int lua_Move(lua_State* L) {
    const MP_Unit* unit = MP_Lua_CheckUnit(L, 1);

    vec2 position;
    position.d.x = luaL_checknumber(L, 2);
    position.d.y = luaL_checknumber(L, 3);
    lua_pushnumber(L, MP_MoveTo(unit, &position));

    return 1;
}

///////////////////////////////////////////////////////////////////////////////
// Registration / Push / Check
///////////////////////////////////////////////////////////////////////////////

static const luaL_Reg lib[] = {
    {"getAbility", lua_GetAbility},
    {"getCanPass", lua_GetCanPass},
    {"getJob", lua_GetJob},
    {"getOwner", lua_GetPlayer},
    {"getPosition", lua_GetPosition},
    {"getType", lua_GetType},

    {"move", lua_Move},
    {NULL, NULL}
};

int MP_Lua_OpenUnit(lua_State* L) {
    luaL_newlib(L, lib);
    return 1;
}
