#include "jobs_meta.h"

#include "jobs.h"
#include "jobs_events.h"
#include "block.h"
#include "room.h"
#include "script.h"
#include "units.h"
#include "lua/lualib.h"

///////////////////////////////////////////////////////////////////////////////
// Constants and globals
///////////////////////////////////////////////////////////////////////////////

static const char* JOB_EVENT_NAME[MP_JOB_EVENT_COUNT] = {
    [MP_JOB_EVENT_UNIT_ADDED] = "onUnitAdded",
    [MP_JOB_EVENT_BLOCK_SELECTION_CHANGED] = "onBlockSelectionChanged",
    [MP_JOB_EVENT_BLOCK_DESTROYED] = "onBlockDestroyed",
    [MP_JOB_EVENT_BLOCK_CONVERTED] = "onBlockConverted"
};

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

static int throwError(lua_State* L) {
    lua_pushstring(L, "not allowed to change global state");
    return lua_error(L);
}

static void getImmutableProxy(lua_State* L) {
    // Create proxy table and metatable.
    lua_newtable(L);
    lua_newtable(L);

    // Make old table the index.
    lua_pushvalue(L, -3);
    lua_setfield(L, -2, "__index");

    // Disable writing (at least to unassigned indexes).
    lua_pushcfunction(L, throwError);
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
                lua_pushfstring(L, "failed setting up environment for '%s'", JOB_EVENT_NAME[jobEvent]);
                return lua_error(L);
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
            lua_pushfstring(L, "failed setting up environment for '%s'", "preference");
            return lua_error(L);
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
            lua_pushfstring(L, "failed setting up environment for '%s'", "run");
            return lua_error(L);
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

///////////////////////////////////////////////////////////////////////////////
// Events
///////////////////////////////////////////////////////////////////////////////

static void onUnitAdded(MP_JobMeta* meta, MP_Unit* unit) {
    const char* eventName = JOB_EVENT_NAME[MP_JOB_EVENT_UNIT_ADDED];
    lua_State* L = meta->L;

    // Try to get the callback.
    lua_getglobal(L, eventName);
    if (lua_isfunction(L, -1)) {
        // Call it with the unit that was added as the parameter.
        luaMP_pushunit(L, unit);
        if (MP_Lua_pcall(L, 1, 0) == LUA_OK) {
            // OK, that's it, we're done.
            return;
        } else {
            // Something went wrong.
            MP_log_error("In '%s' for job '%s': %s\n", eventName, meta->name, lua_tostring(L, -1));
        }
    } else {
        MP_log_error("'%s' for job '%s' isn't a function anymore.\n", eventName, meta->name);
    }

    // Pop function or error message.
    lua_pop(L, 1);

    // We get here only on failure. In that case disable the event callback,
    // so we don't try this again.
    MP_DisableJobEvent(meta, MP_JOB_EVENT_UNIT_ADDED);
}

void MP_FireUnitAdded(MP_Unit* unit) {
    for (unsigned int metaId = 0; metaId < gMetaCount; ++metaId) {
        if (gMetas[metaId].handlesEvent[MP_JOB_EVENT_UNIT_ADDED]) {
            onUnitAdded(&gMetas[metaId], unit);
        }
    }
}

void MP_FireBlockSelectionChanged(MP_Block* block, unsigned short x, unsigned short y, bool selected) {

}

void MP_FireBlockDestroyed(MP_Block* block, unsigned short x, unsigned short y) {

}

void MP_FireBlockConverted(MP_Block* block, unsigned short x, unsigned short y) {

}

/*
static void MP_UpdateJobsForBlock(MP_Player player, unsigned short x, unsigned short y) {
    MP_Block* block = MP_GetBlockAt(x, y);

    // Remove all old jobs that had to do with this block.
    for (unsigned int metaId = 0; metaId < gJobTypeCapacity[MP_PLAYER_ONE]; ++metaId) {
        for (unsigned int number = gJobsCount[player][metaId]; number > 0; --number) {
            MP_Job* job = gJobs[player][metaId][number - 1];
            if (job->block == block) {
                // Remove this one. Notify worker that it's no longer needed.
                MP_StopJob(job);

                // Free memory.
                deleteJob(player, metaId, number - 1);
            }
        }
    }

    // Update / recreate jobs.
    addJobOpenings(player, x, y);
    if (x > 0) {
        addJobOpenings(player, x - 1, y);
    }
    if (x < MP_GetMapSize() - 1) {
        addJobOpenings(player, x + 1, y);
    }
    if (y > 0) {
        addJobOpenings(player, x, y - 1);
    }
    if (y < MP_GetMapSize() - 1) {
        addJobOpenings(player, x, y + 1);
    }
}
 */
