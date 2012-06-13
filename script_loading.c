#include "script_loading.h"

#include "config.h"
#include "log.h"
#include "script.h"

int MP_Lua_Export(lua_State* L) {
    const char* name = luaL_checkstring(L, 1);
    luaL_checktype(L, 2, LUA_TFUNCTION);

    // Ignore empty names.
    if (strlen(name) < 1) {
        MP_log_warning("Ignoring empty name for 'export'.\n");
        return 0;
    }

    // Get the backing environment table (not the proxy!)
    luaMP_pushglobalstable();

    // Don't allow overwriting globals.
    lua_getfield(L, -1, name);
    if (lua_toboolean(L, -1)) {
        return luaL_argerror(L, 1, "globals can be overridden");
    }
    lua_pop(L, 2); // pop entry and globals

    // Get the locals table.
    luaMP_pushlocalstable();

    // Check if there already is such a field.
    lua_getfield(L, -1, name);
    if (lua_isfunction(L, -1)) {
        MP_log_warning("Overriding already exported method '%s'.\n", name);
    }
    lua_pop(L, 1);

    // Bring function to top and register it in the globals table.
    lua_pushvalue(L, 2);
    lua_setfield(L, -2, name);

    lua_pop(L, 1); // pop locals

    MP_log_info("Registered method named '%s'\n", name);

    return 0;
}

int MP_Lua_Import(lua_State* L) {
    // Check if we have a valid name.
    const char* path = luaL_checkstring(L, 1);
    if (strlen(path) < 1) {
        return 0;
    }

    // TODO verify name, must only contain [a-zA-Z0-9_-]

    // Check if the import was already performed.
    luaL_getmetafield(L, LUA_REGISTRYINDEX, "__index");
    lua_getmetatable(L, LUA_REGISTRYINDEX);
    luaL_getsubtable(L, -1, "_LOADED");
    lua_getfield(L, -1, path);
    if (lua_toboolean(L, -1)) {
        // Already loaded.
        lua_pop(L, 2); // pop field and table
        MP_log_info("Skipping already imported file '%s'.\n", path);
        return 0;
    }
    lua_pop(L, 1); // pop field

    // Mark loaded (either way, if we fail we won't try again).
    lua_pushboolean(L, true);
    lua_setfield(L, -2, path);
    lua_pop(L, 4); // pop table

    MP_log_info("Start parsing file '%s'.\n", path);

    // Try to parse the file.
    if (luaL_dofile(L, path) != LUA_OK) {
        // Throw up.
        return lua_error(L);
    }

    MP_log_info("Done parsing file '%s'.\n", path);

    return 0;
}
