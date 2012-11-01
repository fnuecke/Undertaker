#include "script.h"

#include "unit.h"
#include "unit_type.h"
#include "unit_ai.h"
#include "job_type.h"
#include "ability_type.h"
#include "ability.h"

///////////////////////////////////////////////////////////////////////////////
// Getters
///////////////////////////////////////////////////////////////////////////////

static int lua_GetAbility(lua_State* L) {
    MP_Unit* unit = MP_Lua_CheckUnit(L, 1);
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
    lua_pushinteger(L, MP_Lua_CheckUnit(L, 1)->type->canPass);
    return 1;
}

static int lua_GetJob(lua_State* L) {
    MP_Lua_PushJob(L, MP_Lua_CheckUnit(L, 1)->ai->state.job);
    return 1;
}

static int lua_GetOwner(lua_State* L) {
    lua_pushinteger(L, MP_Lua_CheckUnit(L, 1)->owner);
    return 1;
}

static int lua_GetPosition(lua_State* L) {
    MP_Unit* unit = MP_Lua_CheckUnit(L, 1);
    lua_pushnumber(L, unit->position.d.x);
    lua_pushnumber(L, unit->position.d.y);
    return 2;
}

static int lua_GetType(lua_State* L) {
    lua_pushinteger(L, MP_Lua_CheckUnit(L, 1)->type->info.id);
    return 1;
}

///////////////////////////////////////////////////////////////////////////////
// Methods
///////////////////////////////////////////////////////////////////////////////

static int lua_Move(lua_State* L) {
    vec2 position;
    position.d.x = luaL_checknumber(L, 2);
    position.d.y = luaL_checknumber(L, 3);
    lua_pushnumber(L, MP_MoveTo(MP_Lua_CheckUnit(L, 1), &position));
    return 1;
}

///////////////////////////////////////////////////////////////////////////////
// Registration / Push / Check
///////////////////////////////////////////////////////////////////////////////

static const luaL_Reg lib[] = {
    {"getAbility", lua_GetAbility},
    {"getCanPass", lua_GetCanPass},
    {"getJob", lua_GetJob},
    {"getOwner", lua_GetOwner},
    {"getPosition", lua_GetPosition},
    {"getType", lua_GetType},

    {"move", lua_Move},
    {NULL, NULL}
};

int MP_Lua_OpenUnit(lua_State* L) {
    luaL_newlib(L, lib);
    return 1;
}
