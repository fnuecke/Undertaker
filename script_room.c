#include "script.h"

///////////////////////////////////////////////////////////////////////////////
// Getters
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// Methods
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// Registration / Push / Check
///////////////////////////////////////////////////////////////////////////////

static const luaL_Reg lib[] = {
    {NULL, NULL}
};

int luaopen_room(lua_State* L) {
    luaL_newlib(L, lib);
    return 1;
}

void luaMP_pushroom(lua_State* L, MP_Room* room) {
    MP_Room** ud = (MP_Room**) lua_newuserdata(L, sizeof (MP_Room*));
    *ud = room;
    luaL_setmetatable(L, LUA_ROOMLIBNAME);
}

bool luaMP_isroom(lua_State* L, int narg) {
    return luaL_checkudata(L, narg, LUA_ROOMLIBNAME) != NULL;
}

MP_Room* luaMP_toroom(lua_State* L, int narg) {
    return *(MP_Room**) lua_touserdata(L, narg);
}

MP_Room* luaMP_checkroom(lua_State* L, int narg) {
    void* ud = luaL_checkudata(L, narg, LUA_ROOMLIBNAME);
    luaL_argcheck(L, ud != NULL, narg, "'" LUA_ROOMLIBNAME "' expected");
    return *(MP_Room**) ud;
}
