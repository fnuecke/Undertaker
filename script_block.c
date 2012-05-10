#include "script.h"

#include "block.h"
#include "map.h"

///////////////////////////////////////////////////////////////////////////////
// Getters
///////////////////////////////////////////////////////////////////////////////

static int lua_GetDurability(lua_State* L) {
    lua_pushnumber(L, luaMP_checkblock(L, 1)->durability);
    return 1;
}

static int lua_GetGold(lua_State* L) {
    lua_pushinteger(L, luaMP_checkblock(L, 1)->gold);
    return 1;
}

static int lua_GetOwner(lua_State* L) {
    lua_pushinteger(L, luaMP_checkblock(L, 1)->owner);
    return 1;
}

static int lua_GetPassability(lua_State* L) {
    lua_pushinteger(L, luaMP_checkblock(L, 1)->meta->passability);
    return 1;
}

static int lua_GetPosition(lua_State* L) {
    unsigned short x, y;
    MP_GetBlockCoordinates(&x, &y, luaMP_checkblock(L, 1));
    lua_pushinteger(L, x);
    lua_pushinteger(L, y);
    return 2;
}

static int lua_GetRoom(lua_State* L) {
    luaMP_pushroom(L, luaMP_checkblock(L, 1)->room);
    return 1;
}

static int lua_GetStrength(lua_State* L) {
    lua_pushnumber(L, luaMP_checkblock(L, 1)->strength);
    return 1;
}

static int lua_GetType(lua_State* L) {
    lua_pushinteger(L, luaMP_checkblock(L, 1)->meta->id);
    return 1;
}

static int lua_IsPassable(lua_State* L) {
    lua_pushboolean(L, MP_IsBlockPassable(luaMP_checkblock(L, 1)));
    return 1;
}

static int lua_IsPassableBy(lua_State* L) {
    lua_pushboolean(L, MP_IsBlockPassableBy(luaMP_checkblock(L, 1), luaMP_checkunit(L, 2)));
    return 1;
}

static int lua_IsDestructible(lua_State* L) {
    lua_pushboolean(L, MP_IsBlockDestructible(luaMP_checkblock(L, 1)));
    return 1;
}

static int lua_IsConvertible(lua_State* L) {
    lua_pushboolean(L, MP_IsBlockConvertible(luaMP_checkblock(L, 1)));
    return 1;
}

///////////////////////////////////////////////////////////////////////////////
// Methods
///////////////////////////////////////////////////////////////////////////////

static int lua_BlockAt(lua_State* L) {
    luaMP_pushblock(L, MP_GetBlockAt(luaL_checkunsigned(L, 1), luaL_checkunsigned(L, 2)));
    return 1;
}

static int lua_ConvertBlock(lua_State* L) {
    lua_pushboolean(L, MP_ConvertBlock(
            luaMP_checkblock(L, 1), luaL_checknumber(L, 2),
            luaL_checkunsigned(L, 3)));
    return 1;
}

static int lua_DamageBlock(lua_State* L) {
    lua_pushboolean(L, MP_DamageBlock(
            luaMP_checkblock(L, 1), luaL_checknumber(L, 2)));
    return 1;
}

///////////////////////////////////////////////////////////////////////////////
// Registration / Push / Check
///////////////////////////////////////////////////////////////////////////////

static const luaL_Reg lib[] = {
    {"getDurability", lua_GetDurability},
    {"getGold", lua_GetGold},
    {"getOwner", lua_GetOwner},
    {"getPassability", lua_GetPassability},
    {"getPosition", lua_GetPosition},
    {"getRoom", lua_GetRoom},
    {"getStrength", lua_GetStrength},
    {"getType", lua_GetType},
    {"isPassable", lua_IsPassable},
    {"isPassableBy", lua_IsPassableBy},
    {"isDestructible", lua_IsDestructible},
    {"isConvertible", lua_IsConvertible},

    {"at", lua_BlockAt},
    {"convert", lua_ConvertBlock},
    {"damage", lua_DamageBlock},
    {NULL, NULL}
};

int luaopen_block(lua_State* L) {
    luaL_newlib(L, lib);
    return 1;
}

void luaMP_pushblock(lua_State* L, MP_Block* block) {
    MP_Block** ud = (MP_Block**) lua_newuserdata(L, sizeof (MP_Block*));
    *ud = block;
    luaL_setmetatable(L, LUA_BLOCKLIBNAME);
}

MP_Block* luaMP_checkblock(lua_State* L, int narg) {
    void* ud = luaL_checkudata(L, narg, LUA_BLOCKLIBNAME);
    luaL_argcheck(L, ud != NULL, 1, "'" LUA_BLOCKLIBNAME "' expected");
    return *(MP_Block**) ud;
}
