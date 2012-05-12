#include "meta_job.h"

#include "lua/lualib.h"

#include "block.h"
#include "job.h"
#include "job_script.h"
#include "job_script_eventnames.h"
#include "room.h"
#include "unit.h"

///////////////////////////////////////////////////////////////////////////////
// Constants and globals
///////////////////////////////////////////////////////////////////////////////

META_globals(MP_JobMeta)

///////////////////////////////////////////////////////////////////////////////
// Init / Update / Teardown
///////////////////////////////////////////////////////////////////////////////

/** Reset defaults on map change */
static void resetDefaults(void) {
    for (unsigned eventId = 0; eventId < MP_JOB_EVENT_COUNT; ++eventId) {
        gMetaDefaults.handlesEvent[eventId] = false;
    }
    gMetaDefaults.hasDynamicPreference = false;
    gMetaDefaults.hasRunMethod = false;
}

/** New type registered */
inline static bool initMeta(MP_JobMeta* m, const MP_JobMeta* meta) {
    *m = *meta;

    return true;
}

/** Type override */
inline static bool updateMeta(MP_JobMeta* m, const MP_JobMeta* meta) {
    return true;
}

/** Clear up data for a meta on deletion */
inline static void deleteMeta(MP_JobMeta* m) {
    lua_close(m->L);
    m->L = NULL;
}

META_impl(MP_JobMeta, Job)

///////////////////////////////////////////////////////////////////////////////
// Accessors
///////////////////////////////////////////////////////////////////////////////

void MP_DisableJobEvent(const MP_JobMeta* meta, MP_JobEvent event) {
    if (meta) {
        // Get non-const pointer...
        for (unsigned int i = 0; i < gMetaCount; ++i) {
            if (i == meta->id - 1) {
                // Modify.
                gMetas[i]->handlesEvent[event] = false;
                MP_log_info("Disabling '%s' for job '%s'.\n", JOB_EVENT_NAME[event], meta->name);
                return;
            }
        }
    }
}

void MP_DisableDynamicPreference(const MP_JobMeta* meta) {
    if (meta) {
        // Get non-const pointer...
        for (unsigned int i = 0; i < gMetaCount; ++i) {
            if (i == meta->id - 1) {
                // Modify.
                gMetas[i]->hasDynamicPreference = false;
                MP_log_info("Disabling 'preference' for job '%s'.\n", meta->name);
                return;
            }
        }
    }
}

void MP_DisableRunMethod(const MP_JobMeta* meta) {
    if (meta) {
        // Get non-const pointer...
        for (unsigned int i = 0; i < gMetaCount; ++i) {
            if (i == meta->id - 1) {
                // Modify.
                gMetas[i]->hasRunMethod = false;
                MP_log_info("Disabling 'run' for job '%s'.\n", meta->name);
                return;
            }
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
// Lua callbacks
///////////////////////////////////////////////////////////////////////////////

static int lua_Export(lua_State* L) {
    const char* name = luaL_checkstring(L, 1);
    if (strlen(name) < 1) {
        return 0;
    }

    luaL_checktype(L, 2, LUA_TFUNCTION);

    // Get the backing environment table (not the proxy!)
    lua_pushglobaltable(L); // proxy
    lua_getmetatable(L, -1); // proxy's meta
    lua_getfield(L, -1, "__index"); // actual globals table

    // Bring function to top and register it in the globals table.
    lua_pushvalue(L, 2);
    lua_setfield(L, -2, name);

    lua_pop(L, 3); // pop the three tables

    MP_log_info("Registered method named '%s'\n", name);

    return 0;
}

static int lua_Import(lua_State* L) {
    char path[128];

    // Check if we have a valid name.
    const char* filename = luaL_checkstring(L, 1);
    if (strlen(filename) < 1) {
        return 0;
    }

    // Build path.
    if (snprintf(path, sizeof (path), "data/ai/%s.lua", filename) > (int) sizeof (path)) {
        return luaL_error(L, "job script '%s': name too long", filename);
    }

    // Check if the import was already performed.
    luaL_getsubtable(L, LUA_REGISTRYINDEX, "_LOADED");
    lua_getfield(L, -1, filename);
    if (lua_toboolean(L, -1)) {
        // Already loaded.
        lua_pop(L, 2); // pop field and table
        return 0;
    }
    lua_pop(L, 1); // pop field

    // Mark loaded (either way, if we fail we won't try again).
    lua_pushboolean(L, true);
    lua_setfield(L, -2, filename);
    lua_pop(L, 2); // pop table

    MP_log_info("Start parsing job file '%s'.\n", filename);

    // Try to parse the file.
    if (luaL_dofile(L, path) != LUA_OK) {
        // Throw up.
        return lua_error(L);
    }

    MP_log_info("Done parsing job file '%s'.\n", filename);

    return 0;
}

static int warnIndex(lua_State* L) {
    MP_log_warning("Trying to access undefined global '%s'.\n", luaL_checkstring(L, 2));
    return 0;
}

static int throwErrorNewIndex(lua_State* L) {
    lua_pushstring(L, "not allowed to change global state");
    return lua_error(L);
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
static void getImmutableProxy(lua_State* L) {
    // Create proxy table and metatable.
    lua_newtable(L);
    lua_newtable(L);

    // Make old table the index.
    lua_pushvalue(L, -3);
    lua_setfield(L, -2, "__index");

    // Disable writing (at least to unassigned indexes).
    lua_pushcfunction(L, throwErrorNewIndex);
    lua_setfield(L, -2, "__newindex");

    // Set the meta table for the value on the stack.
    lua_setmetatable(L, -2);

    // Replace original table with the proxy.
    lua_replace(L, -2);
}

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
    getImmutableProxy(L);

    // Register it with the table on top before the call to this method.
    lua_setfield(L, -2, modname);
}

static void setupForLoading(lua_State* L) {
    // Get globals table.
    lua_pushglobaltable(L);

    // Register functions.
    lua_pushcfunction(L, lua_Import);
    lua_setfield(L, -2, "import");

    lua_pushcfunction(L, lua_Export);
    lua_setfield(L, -2, "export");

    // Make the environment immutable.
    getImmutableProxy(L);

    // Replace globals table with the proxy.
    lua_rawseti(L, LUA_REGISTRYINDEX, LUA_RIDX_GLOBALS);
}

static void setupForExecution(lua_State* L) {
    // Get the backing environment table (not the proxy!)
    lua_pushglobaltable(L); // proxy
    lua_getmetatable(L, -1); // proxy's meta
    lua_getfield(L, -1, "__index"); // actual globals table

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
    lua_pushcfunction(L, warnIndex);
    lua_setfield(L, -2, "__index");
    lua_setmetatable(L, -2);

    lua_pop(L, 3); // pop three tables from beginning
}

int MP_Lua_AddJobMeta(lua_State* L) {
    // New type, start with defaults.
    MP_JobMeta meta = gMetaDefaults;

    // Validate input.
    luaL_argcheck(L, lua_gettop(L) == 1 && lua_isstring(L, 1), 0, "one 'string' expected");

    // Get name.
    meta.name = lua_tostring(L, 1);

    // Skip if we already know this job (no overrides for job types).
    if (MP_GetJobMetaByName(meta.name)) {
        MP_log_info("Duplicate job declaration for '%s', skipping.\n", meta.name);
        return 0;
    }

    MP_log_info("Start loading job '%s'.\n", meta.name);

    // Load AI script.
    meta.L = luaL_newstate();

    // Set up loading environment / sandbox.
    setupForLoading(meta.L);

    // Load the script.
    lua_pushcfunction(meta.L, lua_Import);
    lua_pushstring(meta.L, meta.name);
    if (MP_Lua_pcall(meta.L, 1, 0) != LUA_OK) {
        // Failed loading, pass error on.
        lua_pushstring(L, lua_tostring(meta.L, -1));
        lua_close(meta.L);
        return lua_error(L);
    }

    // Check script capabilities. First, check for event callbacks.
    for (unsigned int jobEvent = 0; jobEvent < MP_JOB_EVENT_COUNT; ++jobEvent) {
        lua_getglobal(meta.L, JOB_EVENT_NAME[jobEvent]);
        if (lua_isfunction(meta.L, -1)) {
            meta.handlesEvent[jobEvent] = true;
            MP_log_info("Found event handler '%s'.\n", JOB_EVENT_NAME[jobEvent]);
        }
        lua_pop(meta.L, 1); // pop field
    }

    // Then check for dynamic preference callback.
    lua_getglobal(meta.L, "preference");
    if (lua_isfunction(meta.L, -1)) {
        meta.hasDynamicPreference = true;
        MP_log_info("Found dynamic preference callback.\n");
    }
    lua_pop(meta.L, 1); // pop field

    // And finally for the run callback (job active logic).
    lua_getglobal(meta.L, "run");
    if (lua_isfunction(meta.L, -1)) {
        meta.hasRunMethod = true;
        MP_log_info("Found run callback.\n");
    }
    lua_pop(meta.L, 1); // pop field

    // Try to add the job.
    if (!MP_AddJobMeta(&meta)) {
        lua_close(meta.L);
        return luaL_argerror(L, 1, "bad job meta");
    }

    // Set up execution environment / sandbox.
    setupForExecution(meta.L);

    MP_log_info("Done loading job '%s'.\n", meta.name);

    return 0;
}
