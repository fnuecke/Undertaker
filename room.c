#include "room.h"

#define LUA_MT_ROOM "Room"

void MP_Lua_RegisterRoom(lua_State* L) {
    luaL_newmetatable(L, LUA_MT_ROOM);

    // TODO register methods

    lua_pop(L, 1);
}

void MP_Lua_pushroom(lua_State* L, MP_Room* room) {
    MP_Room** ud = (MP_Room**) lua_newuserdata(L, sizeof (MP_Room*));
    *ud = room;
    luaL_setmetatable(L, LUA_MT_ROOM);
}

MP_Room* MP_Lua_checkroom(lua_State* L, int narg) {
    void* ud = luaL_checkudata(L, narg, LUA_MT_ROOM);
    luaL_argcheck(L, ud, 1, "'Room' expected");
    return *(MP_Room**) ud;
}

#undef LUA_MT_ROOM
