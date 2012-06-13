#include "script_context.h"

#include "lua/lualib.h"

#include "log.h"
#include "script.h"

///////////////////////////////////////////////////////////////////////////////
// Globals
///////////////////////////////////////////////////////////////////////////////

static lua_State* L = NULL;

///////////////////////////////////////////////////////////////////////////////
// Environment switching
///////////////////////////////////////////////////////////////////////////////

lua_State* MP_InitLua(void) {
    MP_CloseLua();
    L = luaL_newstate();
    // Get globals table and create an immutable proxy for it.
    lua_pushglobaltable(L);
    MP_Lua_GetImmutableProxy(L);
    lua_rawseti(L, LUA_REGISTRYINDEX, LUA_RIDX_GLOBALS);
    return L;
}

void MP_CloseLua(lua_State* L) {
    if (L) {
        lua_close(L);
        L = NULL;
    }
}

void MP_SetGameScriptGlobals(lua_State* L) {
    // Get the backing environment table (not the proxy!)
    lua_pushglobaltable(L); // proxy
    lua_getmetatable(L, -1); // proxy's meta
    lua_newtable(L); // new globals table

    // Register libraries and types.
    require(L, LUA_BITLIBNAME, luaopen_bit32, false);
    require(L, LUA_MATHLIBNAME, luaopen_math, false);
    require(L, LUA_STRLIBNAME, luaopen_string, false);
    require(L, LUA_TABLIBNAME, luaopen_table, false);

    require(L, LUA_BLOCKLIBNAME, luaopen_block, true);
    require(L, LUA_JOBLIBNAME, luaopen_job, true);
    require(L, LUA_ROOMLIBNAME, luaopen_room, true);
    require(L, LUA_UNITLIBNAME, luaopen_unit, true);

    // Log warnings if accessing undefined fields of old table. This is to
    // notify developers that their scripts do something most likely unintended.
    lua_newtable(L);
    lua_pushcfunction(L, MP_Lua_WarnIndex);
    lua_setfield(L, -2, "__index");
    lua_setmetatable(L, -2);

    // Make new globals table the index.
    lua_setfield(L, -2, "__index"); // meta.__index = globals

    lua_pop(L, 2); // pop meta and proxy
}

void MP_RegisterScriptLocals(const char* type, const char* name) {
    // Get table in registry we use to store locals.
    lua_getfield(L, LUA_REGISTRYINDEX, type);
    if (!lua_istable(L, -1)) {
        // Doesn't exist yet, create it.
        lua_pop(L, 1);
        lua_newtable(L);
        lua_pushvalue(L, -1);
        lua_setfield(L, LUA_REGISTRYINDEX, type);
    }

    luaMP_pushlocalstable(L);
    lua_setfield(L, -2, name);

    lua_pop(L, 1);
}

void MP_LoadScriptLocals(const char* type, const char* name) {
    // Get locals.
    luaMP_pushglobalstable(L);
    lua_getmetatable(L, -1); // globals' meta

    // Get table in registry we use to store locals.
    lua_getfield(L, LUA_REGISTRYINDEX, type);
    if (!lua_istable(L, -1)) {
        // Doesn't exist yet, create it.
        lua_pop(L, 1);
        lua_newtable(L);
        lua_pushvalue(L, -1);
        lua_setfield(L, LUA_REGISTRYINDEX, type);
    }
    lua_getfield(L, -1, name);
    lua_setfield(L, -3, "__index");

    lua_pop(L, 3);
}
