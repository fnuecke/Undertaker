#include "script_loading_aux.h"

#include "log.h"

///////////////////////////////////////////////////////////////////////////////
// Lua callbacks
///////////////////////////////////////////////////////////////////////////////

int MP_Lua_WarnIndex(lua_State* L) {
    MP_log_warning("Trying to access undefined global '%s'.\n", luaL_checkstring(L, 2));
    return 0;
}

int MP_Lua_ThrowErrorNewIndex(lua_State* L) {
    lua_pushstring(L, "not allowed to change global state");
    return lua_error(L);
}

int MP_Lua_BuildImportPath(lua_State* L, const char* pathfmt) {
    // Check if we have a valid name.
    const char* scriptname = luaL_checkstring(L, 1);
    if (strlen(scriptname) < 1) {
        luaL_error(L, "invalid or empty script name");
    }

    lua_pushfstring(L, pathfmt, scriptname);
    lua_remove(L, -2);

    return 0;
}

/**
 * Takes a table and wraps it in an immutable proxy:
 * 
 * T = input table
 * -> {} with metatable = {
 *      __index=T with metatable = {__index=error},
 *      __newindex=error
 *      }
 */
void MP_GetImmutableProxy(lua_State* L) {
    // Create proxy table and metatable.
    lua_newtable(L);
    lua_newtable(L);

    // Make old table the index.
    lua_pushvalue(L, -3);
    lua_setfield(L, -2, "__index");

    // Disable writing (at least to unassigned indexes).
    lua_pushcfunction(L, MP_Lua_ThrowErrorNewIndex);
    lua_setfield(L, -2, "__newindex");

    // Set the meta table for the value on the stack.
    lua_setmetatable(L, -2);

    // Replace original table with the proxy.
    lua_replace(L, -2);
}

///////////////////////////////////////////////////////////////////////////////
// Context builders
///////////////////////////////////////////////////////////////////////////////

void require(lua_State* L, const char *modname, lua_CFunction openf, bool mt) {
    // Set up the library.
    luaL_requiref(L, modname, openf, 0);

    // Make it a meta table.
    if (mt) {
        // Make it its own index, then register it.
        lua_pushvalue(L, -1);
        lua_setfield(L, -2, "__index");
        lua_pushvalue(L, -1);
        lua_setfield(L, LUA_REGISTRYINDEX, modname);
    }

    // Make it immutable (get an immutable proxy for it).
    MP_GetImmutableProxy(L);

    // Register it with the table on top before the call to this method.
    lua_setfield(L, -2, modname);
}

void MP_PushScriptGlobals(lua_State* L, const luaL_Reg* methods) {
    // Get globals' (proxy) metatable and create new table for globals.
    lua_pushglobaltable(L);
    lua_getmetatable(L, -1);
    lua_newtable(L);

    // Stack:
    // proxy
    // meta
    // globals

    for (luaL_Reg* m = methods; m->name != NULL; ++m) {
        lua_pushcfunction(L, m->func);
        lua_setfield(L, -2, m->name);
    }

    // Give it a metatable in which we may register an index table later on.
    lua_newtable(L);
    lua_pushcfunction(L, MP_Lua_WarnIndex);
    lua_setfield(L, -2, "__index");
    lua_setmetatable(L, -2);

    // Make the new globals the index.
    lua_setfield(L, -2, "__index"); // meta.__index = globals

    lua_pop(L, 2); // pop meta and proxy
}

void MP_PopScriptGlobals(lua_State* L) {
    
}

///////////////////////////////////////////////////////////////////////////////
// Context switchers
///////////////////////////////////////////////////////////////////////////////

void luaMP_pushglobalstable(lua_State* L) {
    lua_pushglobaltable(L); // proxy
    lua_getmetatable(L, -1); // proxy's meta
    lua_getfield(L, -1, "__index"); // actual globals
    lua_replace(L, -3); // move globals to proxy stack position
    lua_pop(L, 1); // pop proxy meta
}

void luaMP_pushlocalstable(lua_State* L) {
    luaMP_pushglobalstable(L);
    if (luaL_getmetafield(L, -1, "__index")) {
        // Exists, return them.
        lua_replace(L, -2);
    } else {
        // Don't exist, create them.
        lua_getmetatable(L, -1); // globals' meta

        lua_newtable(L); // new __index for globals (-> locals)

        // Give it a metatable to warn when accessing undefined names.
        lua_newtable(L);
        lua_pushcfunction(L, MP_Lua_WarnIndex);
        lua_setfield(L, -2, "__index");
        lua_setmetatable(L, -2);

        lua_pushvalue(L, -1); // duplicate index
        lua_setfield(L, -3, "__index"); // set it

        lua_replace(L, -3); // move index to bottom (replacing globals)

        lua_pop(L, 1); // pop globals meta
    }
}
