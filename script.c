#include <string.h>
#include "lua/lualib.h"

#include "ability_type.h"
#include "block_type.h"
#include "job_type.h"
#include "log.h"
#include "map.h"
#include "passability.h"
#include "room_type.h"
#include "script.h"
#include "unit_type.h"

///////////////////////////////////////////////////////////////////////////////
// Utility methods
///////////////////////////////////////////////////////////////////////////////

static int undefinedIndexWarning(lua_State* L) {
    MP_log_warning("Tried to access undefined global '%s'.\n", luaL_checkstring(L, 2));
    return 0;
}

static int readOnlyError(lua_State* L) {
    return luaL_error(L, "Tried to modify an immutable table.");
}

/**
 * Wraps a table at the specified index with an immutable proxy table and
 * pushes it to the top of the stack.
 * T := input table
 * -> {} with metatable = {
 *      __index=T,
 *      __newindex=error
 *      }
 */
void pushImmutableProxy(lua_State* L, int index) {
    index = lua_absindex(L, index);

    // Create proxy table and meta table.
    lua_newtable(L);
    lua_newtable(L);

    // Make old table the index.
    lua_pushvalue(L, index);
    lua_setfield(L, -2, "__index");

    // Disable writing (at least to unassigned indexes).
    lua_pushcfunction(L, readOnlyError);
    lua_setfield(L, -2, "__newindex");

    // Set the meta table for the proxy on the stack.
    lua_setmetatable(L, -2);
}

///////////////////////////////////////////////////////////////////////////////
// Global state
///////////////////////////////////////////////////////////////////////////////

/** The Lua VM used for the currently running game */
static lua_State* gL = NULL;

static void require(lua_State* L, const char *modname, lua_CFunction openf, bool mt) {
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
    pushImmutableProxy(L, -1);
    lua_remove(L, -2);

    // Register it with the table on top before the call to this method.
    lua_setfield(L, -2, modname);
}

lua_State* MP_Lua_Init(void) {
    // Shutdown old VM if there was one.
    MP_Lua_Close();

    // Create new one.
    gL = luaL_newstate();

    // Get globals table and initialize to ingame scripting environment.
    lua_pushglobaltable(gL); // globals

    // Register libraries and types.
    require(gL, LUA_BITLIBNAME, luaopen_bit32, false);
    require(gL, LUA_MATHLIBNAME, luaopen_math, false);
    require(gL, LUA_STRLIBNAME, luaopen_string, false);
    require(gL, LUA_TABLIBNAME, luaopen_table, false);

    require(gL, LUA_ABILITYLIBNAME, MP_Lua_OpenAbility, true);
    require(gL, LUA_JOBLIBNAME, MP_Lua_OpenJob, true);
    require(gL, LUA_BLOCKLIBNAME, MP_Lua_OpenBlock, true);
    require(gL, LUA_UNITLIBNAME, MP_Lua_OpenUnit, true);
    require(gL, LUA_ROOMLIBNAME, MP_Lua_OpenRoom, true);

    // Log warnings if accessing undefined fields of our table. This is
    // to notify developers that their scripts do something most likely
    // unintended.
    lua_newtable(gL);
    lua_pushcfunction(gL, undefinedIndexWarning);
    lua_setfield(gL, -2, "__index");
    lua_setmetatable(gL, -2);

    // Create an immutable proxy for it so the API won't be messed up.
    pushImmutableProxy(gL, -1);
    lua_rawseti(gL, LUA_REGISTRYINDEX, LUA_RIDX_GLOBALS);

    // Pop globals.
    lua_pop(gL, 1);

    return gL;
}

lua_State* MP_Lua(void) {
    return gL;
}

void MP_Lua_Close(void) {
    if (gL) {
        lua_close(gL);
        gL = NULL;
    }
}

///////////////////////////////////////////////////////////////////////////////
// Validation
///////////////////////////////////////////////////////////////////////////////

MP_Passability MP_Lua_CheckPassability(lua_State* L, int narg) {
    const MP_Passability passability = MP_GetPassability(luaL_checkstring(L, narg));
    luaL_argcheck(L, passability != MP_PASSABILITY_NONE, narg, "invalid 'passability' value");
    return passability;
}

MP_Player MP_Lua_CheckPlayer(lua_State* L, int narg) {
    const MP_Player player = luaL_checkunsigned(L, narg);
    luaL_argcheck(L, player != MP_PLAYER_NONE && player < MP_PLAYER_COUNT, narg, "invalid 'player' value");
    return player;
}

MP_BlockLevel MP_Lua_CheckLevel(lua_State* L, int narg) {
    const char* level = luaL_checkstring(L, narg);
    if (strcmp(level, "pit") == 0) {
        return MP_BLOCK_LEVEL_PIT;
    } else if (strcmp(level, "lowered") == 0) {
        return MP_BLOCK_LEVEL_LOWERED;
    } else if (strcmp(level, "normal") == 0) {
        return MP_BLOCK_LEVEL_NORMAL;
    } else if (strcmp(level, "high") == 0) {
        return MP_BLOCK_LEVEL_HIGH;
    }

    return luaL_argerror(L, narg, "invalid 'level' value");
}

///////////////////////////////////////////////////////////////////////////////
// Call with stack trace
///////////////////////////////////////////////////////////////////////////////

static lua_State* getthread(lua_State* L, int* arg) {
    if (lua_isthread(L, 1)) {
        *arg = 1;
        return lua_tothread(L, 1);
    } else {
        *arg = 0;
        return L;
    }
}

static int traceback(lua_State *L) {
    int arg;
    lua_State *L1 = getthread(L, &arg);
    const char *msg = lua_tostring(L, arg + 1);
    if (msg == NULL && !lua_isnoneornil(L, arg + 1)) /* non-string 'msg'? */
        lua_pushvalue(L, arg + 1); /* return it untouched */
    else {
        int level = luaL_optint(L, arg + 2, (L == L1) ? 1 : 0);
        luaL_traceback(L, L1, msg, level);
    }
    return 1;
}

int MP_Lua_pcall(lua_State* L, int nargs, int nresults) {
    int result;

    // Insert traceback method before actually called function.
    lua_pushcfunction(L, traceback);
    lua_insert(L, -(nargs + 2));

    // Run and check result to know where to remove traceback method.
    if ((result = lua_pcall(L, nargs, nresults, -(nargs + 2))) == LUA_OK) {
        // Success, there's nresult entries on the stack.
        lua_remove(L, -(nresults + 1));
    } else {
        // Error, means there's one string on the stack.
        lua_remove(L, -2);
    }
    return result;
}

///////////////////////////////////////////////////////////////////////////////
// Utilities
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// Loading
///////////////////////////////////////////////////////////////////////////////
            
// Reserved words in lua, scripts cannot have these names.
static const char* gReserved[] = {"and", "break", "do", "else", "elseif",
                                  "end", "false", "for", "function", "if",
                                  "in", "local", "nil", "not", "or",
                                  "repeat", "return", "then", "true", "until",
                                  "while",
                                  LUA_COLIBNAME, LUA_TABLIBNAME,
                                  LUA_IOLIBNAME, LUA_OSLIBNAME,
                                  LUA_STRLIBNAME, LUA_BITLIBNAME,
                                  LUA_MATHLIBNAME, LUA_DBLIBNAME,
                                  LUA_LOADLIBNAME,
                                  LUA_BLOCKLIBNAME, LUA_JOBLIBNAME,
                                  LUA_ROOMLIBNAME, LUA_UNITLIBNAME,
                                  "import", "mapsize", "ability", "job",
                                  "passability", "blockdefaults", "block",
                                  "unitdefaults", "unit",
                                  NULL};

inline static int isempty(lua_State* L, int index) {
    index = lua_absindex(L, index);
    if (lua_type(L, index) == LUA_TTABLE) {
        lua_pushnil(L);
        if (lua_next(L, index)) {
            lua_pop(L, 2); // pop key and value
            return false;
        }
    }
    return true;
}

static void pushglobalsindex(lua_State* L) {
    lua_pushglobaltable(L); // proxy
    lua_getmetatable(L, -1); // proxy's meta
    lua_getfield(L, -1, "__index"); // actual globals
    lua_replace(L, -3); // move globals to proxy stack position
    lua_pop(L, 1); // pop proxy meta
}

static int lua_Import(lua_State* L) {
    char path[256];
    const char* name = luaL_checkstring(L, 1);
    int level;

    // Check script name length.
    if (strlen(name) < 1) {
        return luaL_argerror(L, 1, "invalid or empty script name");
    }
    if (strlen(name) > 128) {
        return luaL_argerror(L, 1, "script name too long");
    }

    // Check for invalid characters.
    if (strspn(name, "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_") != strlen(name)) {
        return luaL_argerror(L, 1, "script name contains invalid characters");
    }
    // Check if it starts with a number.
    if (name[0] >= '0' && name[0] <= '9') {
        return luaL_argerror(L, 1, "script name must not start with a number");
    }
    // Check for reserved names.
    for (int i = 0; gReserved[i] != NULL; ++i) {
        if (strcmp(gReserved[i], name) == 0) {
            return luaL_argerror(L, 1, "script name is reserved");
        }
    }

    // Build actual path.
    snprintf(path, sizeof (path), "data/scripts/%s.lua", name);

    // Check if the import was already performed.
    luaL_getsubtable(L, LUA_REGISTRYINDEX, "MP_LOADED");
    lua_getfield(L, -1, path);
    if (lua_toboolean(L, -1)) {
        // Already loaded.
        lua_pop(L, 2); // pop field and table
        MP_log_info("Skipping already imported script file '%s'.\n", path);
        return 0;
    }
    lua_pop(L, 1); // pop field

    // Mark loaded (either way, if we fail we won't try again).
    lua_pushboolean(L, true);
    lua_setfield(L, -2, path);
    lua_pop(L, 1); // pop table

    // Create a table for globals the script may export.
    pushglobalsindex(L);
    lua_getfield(L, -1, name);
    if (!lua_istable(L, -1)) {
        lua_newtable(L);
        lua_setfield(L, -3, name);
    }
    lua_pop(L, 2);
    
    MP_log_info("Start parsing script file '%s'.\n", path);

    // Try to parse the file.
    level = lua_gettop(L);
    if (luaL_dofile(L, path) != LUA_OK) {
        // Remove globals table.
        pushglobalsindex(L);
        lua_pushnil(L);
        lua_setfield(L, -2, name);
        lua_pop(L, 1);

        // Throw up.
        return lua_error(L);
    } else {
        // Pop results.
        lua_pop(L, lua_gettop(L) - level);
    }

    MP_log_info("Done parsing file '%s'.\n", path);

    // Make the table with script globals immutable, or remove it if there's
    // nothing in it (script does not export globals).
    pushglobalsindex(L);
    if (isempty(L, -1)) {
        // Pop script table from stack and overwrite with nil in globals.
        lua_pushnil(L);
    } else {
        // Make script table immutable and overwrite with proxy in globals.
        luaL_getsubtable(L, -1, name);
        pushImmutableProxy(L, -1);
        lua_remove(L, -2);
    }
    lua_setfield(L, -2, name);
    lua_pop(L, 1);

    return 0;
}

static int lua_SetMapSize(lua_State* L) {
    unsigned int size = luaL_checkunsigned(L, 1);
    const MP_BlockType* type = MP_Lua_CheckBlockType(L, 2);
    if (size < 1) {
        return luaL_error(L, "invalid map size");
    }
    MP_SetMapSize(size, type);

    return 0;
}

static const luaL_Reg loadlib[] = {
    {"import", lua_Import},
    {"mapsize", lua_SetMapSize},
    {"ability", MP_LuaCallback_AddAbilityType},
    {"job", MP_LuaCallback_AddJobType},
    {"passability", MP_Lua_AddPassability},
    {"blockdefaults", MP_LuaCallback_SetBlockTypeDefaults},
    {"block", MP_LuaCallback_AddBlockType},
    {"unitdefaults", MP_LuaCallback_SetUnitTypeDefaults},
    {"unit", MP_LuaCallback_AddUnitType},
    {NULL, NULL}
};

int MP_Lua_Load(const char* path) {
    int result, level;
    lua_State* L = MP_Lua();

    // Get globals meta table and create new table for globals.
    lua_pushglobaltable(L);
    lua_getmetatable(L, -1);
    lua_getfield(L, -1, "__index"); // keep for restoring it later

    // Register all functions in our new index table.
    lua_newtable(L);
    for (const luaL_Reg* m = loadlib; m->name != NULL; ++m) {
        lua_pushcfunction(L, m->func);
        lua_setfield(L, -2, m->name);
    }
    // Copy for extracting globals after parsing.
    lua_pushvalue(L, -1);

    // Make the new globals the index.
    lua_setfield(L, -4, "__index");

    // Try to parse the script.
    level = lua_gettop(L);
    if ((result = luaL_dofile(L, path)) != LUA_OK) {
        // Save error message, push it below the globals table.
        lua_insert(L, -5);
    } else {
        // Pop results.
        lua_pop(L, lua_gettop(L) - level);

        // Copy globals from parsed scripts. Those are essentially the tables
        // that are not our own entries, so we delete those first, then copy
        // over what remains.
        for (const luaL_Reg* m = loadlib; m->name != NULL; ++m) {
            lua_pushnil(L);
            lua_setfield(L, -2, m->name);
        }

        lua_pushnil(L);
        while (lua_next(L, -2)) {
            lua_pushvalue(L, -2);
            lua_insert(L, -2);
            lua_settable(L, -5);
        }
    }
    
    // Pop our index.
    lua_pop(L, 1);

    // Restore global state.
    lua_setfield(L, -2, "__index");
    lua_pop(L, 2); // pop globals and its meta table

    return result;
}
