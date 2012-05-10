#include "block.h"

#include "room.h"
#include "units.h"
#include "map.h"

MP_Passability MP_GetBlockPassability(const MP_Block* block) {
    return block && block->meta && (block->room ? block->room->meta->passability : block->meta->passability);
}

MP_BlockLevel MP_GetBlockLevel(const MP_Block* block) {
    return block && block->meta && (block->room ? block->room->meta->level : block->meta->level);
}

bool MP_IsBlockPassable(const MP_Block* block) {
    return MP_GetBlockPassability(block) > 0;
}

bool MP_IsBlockPassableBy(const MP_Block* block, const MP_Unit* unit) {
    return unit && unit->meta && (MP_GetBlockPassability(block) & unit->meta->canPass) != 0;
}

bool MP_IsBlockDestructible(const MP_Block* block) {
    return block && block->meta && block->meta->durability > 0;
}

bool MP_IsBlockConvertible(const MP_Block* block) {
    return block && block->meta && block->meta->strength > 0;
}

static int lua_getOwner(lua_State* L) {
    lua_pushinteger(L, MP_Lua_checkblock(L, 1)->owner);
    return 1;
}

static int lua_getRoom(lua_State* L) {
    MP_Lua_pushroom(L, MP_Lua_checkblock(L, 1)->room);
    return 1;
}

static int lua_getDurability(lua_State* L) {
    lua_pushnumber(L, MP_Lua_checkblock(L, 1)->durability);
    return 1;
}

static int lua_getStrength(lua_State* L) {
    lua_pushnumber(L, MP_Lua_checkblock(L, 1)->strength);
    return 1;
}

static int lua_getGold(lua_State* L) {
    lua_pushinteger(L, MP_Lua_checkblock(L, 1)->gold);
    return 1;
}

static int lua_getType(lua_State* L) {
    lua_pushinteger(L, MP_Lua_checkblock(L, 1)->meta->id);
    return 1;
}

static int lua_getPassability(lua_State* L) {
    lua_pushinteger(L, MP_Lua_checkblock(L, 1)->meta->passability);
    return 1;
}

static int lua_getPosition(lua_State* L) {
    unsigned short x, y;
    MP_GetBlockCoordinates(&x, &y, MP_Lua_checkblock(L, 1));
    lua_pushinteger(L, x);
    lua_pushinteger(L, y);
    return 2;
}

static const luaL_Reg blockgetters[] = {
    {"owner", lua_getOwner},
    {"room", lua_getRoom},
    {"durability", lua_getDurability},
    {"strength", lua_getStrength},
    {"gold", lua_getGold},
    {"type", lua_getType},
    {"passability", lua_getPassability},
    {"position", lua_getPosition},
    {NULL, NULL}
};

static const luaL_Reg blockmethods[] = {
    {NULL, NULL}
};

#define LUA_MT_BLOCK "Block"

void MP_Lua_RegisterBlock(lua_State* L) {
    luaL_newmetatable(L, LUA_MT_BLOCK);

    // TODO register methods

    lua_pop(L, 1);
}

void MP_Lua_pushblock(lua_State* L, MP_Block* block) {
    MP_Block** b = (MP_Block**) lua_newuserdata(L, sizeof (MP_Block*));
    *b = block;
    luaL_setmetatable(L, LUA_MT_BLOCK);
}

MP_Block* MP_Lua_checkblock(lua_State* L, int narg) {
    void* b = luaL_checkudata(L, narg, LUA_MT_BLOCK);
    luaL_argcheck(L, b, 1, "'Block' expected");
    return *(MP_Block**) b;
}

#undef LUA_MT_BLOCK
