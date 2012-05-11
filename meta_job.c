#include "meta_job.h"

#include "lua/lualib.h"

#include "block.h"
#include "job.h"
#include "job_script.h"
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
            if (gMetas[i].id == meta->id) {
                // Modify.
                gMetas[i].handlesEvent[event] = false;
                MP_log_info("Disabling event callback '%s' for job '%s'.\n", JOB_EVENT_NAME[event], meta->name);
                return;
            }
        }
    }
}

void MP_DisableDynamicPreference(const MP_JobMeta* meta) {
    if (meta) {
        // Get non-const pointer...
        for (unsigned int i = 0; i < gMetaCount; ++i) {
            if (gMetas[i].id == meta->id) {
                // Modify.
                gMetas[i].hasDynamicPreference = false;
                MP_log_info("Disabling dynamic preference for '%s'.\n", meta->name);
                return;
            }
        }
    }
}

void MP_DisableRunMethod(const MP_JobMeta* meta) {
    if (meta) {
        // Get non-const pointer...
        for (unsigned int i = 0; i < gMetaCount; ++i) {
            if (gMetas[i].id == meta->id) {
                // Modify.
                gMetas[i].hasRunMethod = false;
                MP_log_info("Disabling run method for job '%s'.\n", meta->name);
                return;
            }
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
// Lua callbacks
///////////////////////////////////////////////////////////////////////////////

static int throwErrorIndex(lua_State* L) {
    lua_pushstring(L, "trying to access undefined global");
    return lua_error(L);
}

static int throwErrorNewIndex(lua_State* L) {
    lua_pushstring(L, "not allowed to change global state");
    return lua_error(L);
}

static void getImmutableProxy(lua_State* L) {
    // Make old table throw if indexing undefined entries.
    lua_newtable(L);
    lua_pushcfunction(L, throwErrorIndex);
    lua_setfield(L, -2, "__index");
    lua_setmetatable(L, -2);

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

static bool createEnv(lua_State* L) {
    // Find _ENV upvalue index.
    unsigned int n = 0;
    while (1) {
        const char* name = lua_getupvalue(L, -1, ++n);
        if (name == NULL) {
            // no _ENV upvalue
            return false;
        }
        lua_pop(L, 1);
        if (strcmp(name, "_ENV") == 0) {
            break;
        }
    }

    // Create table we use as an environment.
    lua_newtable(L);

    // Register types.
    require(L, LUA_BITLIBNAME, luaopen_bit32, false);
    require(L, LUA_MATHLIBNAME, luaopen_math, false);
    require(L, LUA_STRLIBNAME, luaopen_string, false);
    require(L, LUA_TABLIBNAME, luaopen_table, false);

    require(L, LUA_BLOCKLIBNAME, luaopen_block, true);
    require(L, LUA_JOBLIBNAME, luaopen_job, true);
    require(L, LUA_ROOMLIBNAME, luaopen_room, true);
    require(L, LUA_UNITLIBNAME, luaopen_unit, true);

    // Make the environment immutable.
    getImmutableProxy(L);

    // Set it as the new environment.
    lua_setupvalue(L, -2, n);

    return true;
}

int MP_Lua_AddJobMeta(lua_State* L) {
    char filename[128];
    bool isEnvSetup = false;

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

    // Load AI script.
    meta.L = luaL_newstate();

    // Build file name.
    if (snprintf(filename, sizeof (filename), "data/ai/%s.lua", meta.name) > (int) sizeof (filename)) {
        lua_close(meta.L);
        return luaL_argerror(L, 1, "job name too long");
    }

    MP_log_info("Start parsing job file '%s'.\n", filename);

    // Try to parse the file.
    if (luaL_dofile(meta.L, filename) != LUA_OK) {
        MP_log_error("Failed parsing job file:\n%s\n", lua_tostring(meta.L, -1));
        lua_close(meta.L);
        return luaL_argerror(L, 1, "invalid job script");
    }

    // Check script capabilities. First, check for event callbacks.
    for (unsigned int jobEvent = 0; jobEvent < MP_JOB_EVENT_COUNT; ++jobEvent) {
        lua_getglobal(meta.L, JOB_EVENT_NAME[jobEvent]);
        if (lua_isfunction(meta.L, -1)) {
            if (!isEnvSetup && !createEnv(meta.L)) {
                return luaL_error(L, "failed setting up environment for '%s'", JOB_EVENT_NAME[jobEvent]);
            }
            isEnvSetup = true;
            meta.handlesEvent[jobEvent] = true;
            MP_log_info("Found event handler '%s'.\n", JOB_EVENT_NAME[jobEvent]);
        }
        lua_pop(meta.L, 1);
    }

    // Then check for dynamic preference callback.
    lua_getglobal(meta.L, "preference");
    if (lua_isfunction(meta.L, -1)) {
        if (!isEnvSetup && !createEnv(meta.L)) {
            luaL_error(L, "failed setting up environment for '%s'", "preference");
        }
        isEnvSetup = true;
        meta.hasDynamicPreference = true;
        MP_log_info("Found dynamic preference callback.\n");
    }
    lua_pop(meta.L, 1);

    // And finally for the run callback (job active logic).
    lua_getglobal(meta.L, "run");
    if (lua_isfunction(meta.L, -1)) {
        if (!isEnvSetup && !createEnv(meta.L)) {
            luaL_error(L, "failed setting up environment for '%s'", "run");
        }
        isEnvSetup = true;
        meta.hasRunMethod = true;
        MP_log_info("Found run callback.\n");
    }
    lua_pop(meta.L, 1);

    // Try to add the job.
    if (!MP_AddJobMeta(&meta)) {
        lua_close(meta.L);
        return luaL_argerror(L, 1, "bad job meta");
    }

    MP_log_info("Done parsing job file '%s'.\n", filename);

    return 0;
}
