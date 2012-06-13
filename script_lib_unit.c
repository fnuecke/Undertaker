#include "script.h"

#include "unit.h"
#include "meta_unit.h"
#include "unit_ai.h"
#include "meta_job.h"

///////////////////////////////////////////////////////////////////////////////
// Getters
///////////////////////////////////////////////////////////////////////////////

static int lua_GetCanPass(lua_State* L) {
    lua_pushinteger(L, luaMP_checkunit(L, 1)->meta->canPass);
    return 1;
}

static int lua_GetOwner(lua_State* L) {
    lua_pushinteger(L, luaMP_checkunit(L, 1)->owner);
    return 1;
}

static int lua_GetPosition(lua_State* L) {
    MP_Unit* unit = luaMP_checkunit(L, 1);
    lua_pushnumber(L, unit->position.d.x);
    lua_pushnumber(L, unit->position.d.y);
    return 2;
}

static int lua_GetType(lua_State* L) {
    lua_pushinteger(L, luaMP_checkunit(L, 1)->meta->id);
    return 1;
}

///////////////////////////////////////////////////////////////////////////////
// Methods
///////////////////////////////////////////////////////////////////////////////

static int lua_Move(lua_State* L) {
    vec2 position;
    position.d.x = luaL_checknumber(L, 2);
    position.d.y = luaL_checknumber(L, 3);
    lua_pushnumber(L, MP_MoveTo(luaMP_checkunit(L, 1), &position));
    return 1;
}

///////////////////////////////////////////////////////////////////////////////
// Registration / Push / Check
///////////////////////////////////////////////////////////////////////////////

static const luaL_Reg lib[] = {
    {"getCanPass", lua_GetCanPass},
    {"getOwner", lua_GetOwner},
    {"getPosition", lua_GetPosition},
    {"getType", lua_GetType},

    {"move", lua_Move},
    {NULL, NULL}
};

int luaopen_unit(lua_State* L) {
    luaL_newlib(L, lib);
    return 1;
}

void luaMP_pushunit(lua_State* L, MP_Unit* unit) {
    MP_Unit** ud = (MP_Unit**) lua_newuserdata(L, sizeof (MP_Unit*));
    *ud = unit;
    luaL_setmetatable(L, LUA_UNITLIBNAME);
}

bool luaMP_isunit(lua_State* L, int narg) {
    return luaL_checkudata(L, narg, LUA_UNITLIBNAME) != NULL;
}

MP_Unit* luaMP_tounit(lua_State* L, int narg) {
    return *(MP_Unit**) lua_touserdata(L, narg);
}

MP_Unit* luaMP_checkunit(lua_State* L, int narg) {
    void* ud = luaL_checkudata(L, narg, LUA_UNITLIBNAME);
    luaL_argcheck(L, ud != NULL, narg, "'" LUA_UNITLIBNAME "' expected");
    return *(MP_Unit**) ud;
}
