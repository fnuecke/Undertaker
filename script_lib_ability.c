#include "ability.h"
#include "config.h"
#include "script.h"

///////////////////////////////////////////////////////////////////////////////
// Getters
///////////////////////////////////////////////////////////////////////////////

static int lua_GetUnit(lua_State* L) {
    MP_Ability* ability = MP_Lua_CheckAbility(L, 1);
    MP_Lua_PushUnit(L, ability->unit);
    return 1;
}

static int lua_GetCooldown(lua_State* L) {
    MP_Ability* ability = MP_Lua_CheckAbility(L, 1);
    lua_pushnumber(L, ability->cooldown / (float)MP_FRAMERATE);
    return 1;
}

static int lua_GetProperties(lua_State* L) {
    MP_Ability* ability = MP_Lua_CheckAbility(L, 1);
    lua_rawgeti(L, LUA_REGISTRYINDEX, ability->properties);
    return 1;
}

///////////////////////////////////////////////////////////////////////////////
// Methods
///////////////////////////////////////////////////////////////////////////////

static int lua_Use(lua_State* L) {
    MP_Ability* ability = MP_Lua_CheckAbility(L, 1);
    lua_pushnumber(L, MP_UseAbility(ability));
    return 1;
}

///////////////////////////////////////////////////////////////////////////////
// Registration / Push / Check
///////////////////////////////////////////////////////////////////////////////

static const luaL_Reg lib[] = {
    {"getCooldown", lua_GetCooldown},
    {"getProperties", lua_GetProperties},
    {"getUnit", lua_GetUnit},
    
    {"use", lua_Use},
    {NULL, NULL}
};

int MP_Lua_OpenAbility(lua_State* L) {
    luaL_newlib(L, lib);
    return 1;
}
