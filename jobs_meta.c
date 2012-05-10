#include "jobs_meta.h"

#include "jobs_events.h"
#include "jobs.h"
#include "block.h"
#include "room.h"
#include "units.h"

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
                return;
            }
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
// Lua callbacks
///////////////////////////////////////////////////////////////////////////////

static int addJob(lua_State* L) {
    MP_Job job = {NULL, NULL, NULL, NULL, NULL, {{0, 0}}};
    const char* name;
    // Keep track at which table entry we are.
    int narg = 1;

    // Validate input.
    luaL_argcheck(L, lua_gettop(L) == 1 && lua_istable(L, 1), 0, "must specify one table");

    // Get the name.
    lua_getfield(L, -1, "name");
    // -> table, table["name"]
    luaL_argcheck(L, lua_type(L, -1) == LUA_TSTRING, narg, "no 'name' or not a string");
    name = lua_tostring(L, -1);
    lua_pop(L, 1);
    // -> table

    // Get job meta data.
    job.meta = MP_GetJobMetaByName(name);
    luaL_argcheck(L, job.meta, narg, "unknown job type");

    // Now loop through the table. Push initial 'key' -- nil means start.
    lua_pushnil(L);
    // -> table, nil
    while (lua_next(L, -2)) {
        // -> table, key, value
        // Get key as string.
        const char* key;
        luaL_argcheck(L, lua_type(L, -2) == LUA_TSTRING, narg, "keys must be strings");
        key = lua_tostring(L, -2);

        // See what we have.
        if (strcmp(key, "name") == 0) {
            // Silently skip it.

        } else if (strcmp(key, "block") == 0) {
            job.block = MP_Lua_checkblock(L, -1);

        } else if (strcmp(key, "room") == 0) {
            job.room = MP_Lua_checkroom(L, -1);

        } else if (strcmp(key, "unit") == 0) {
            job.unit = MP_Lua_checkunit(L, -1);

        } else if (strcmp(key, "offset") == 0) {
            // TODO

        } else {
            return luaL_argerror(L, narg, "unknown key");
        }

        lua_pop(L, 1);
    }

    return 0;
}

static int removeJob(lua_State* L) {
    return 0;
}

static const luaL_Reg ailib[] = {
    {"addJob", addJob},
    {"removeJob", removeJob},
    {NULL, NULL}
};

static unsigned int findEnv(lua_State* L) {
    unsigned int i = 1;
    const char* name;
    while (1) {
        name = lua_getupvalue(L, -1, i);
        if (name == NULL) {
            return 0;
        }
        lua_pop(L, 1);
        if (strcmp(name, "_ENV") == 0) {
            return i;
        }
    }

    // We'll never get here, but suppress warnings for dump parsers...
    return 0;
}

static int blockNewIndex(lua_State* L) {
    lua_pushstring(L, "globals are disabled");
    return lua_error(L);
}

static bool createEnv(lua_State* L) {
    // Find _ENV upvalue to force a local, immutable environment.
    const int n = findEnv(L);
    if (!n) {
        return false;
    }

    // Create environment.
    lua_newtable(L);
    // -> f, env

    // Create metatable.
    lua_newtable(L);
    // -> f, env, meta
    lua_pushvalue(L, -1);
    // -> f, env, meta, meta

    // Register API methods.
    for (const luaL_Reg* l = ailib; l->name != NULL; ++l) {
        lua_pushcfunction(L, l->func);
        // -> f, env, meta, meta, function
        lua_setfield(L, -2, l->name);
        // -> f, env, meta, meta
    }

    // Make meta its own index.
    lua_setfield(L, -2, "__index");
    // -> f, env, meta

    // Disable adding new globals.
    lua_pushcfunction(L, blockNewIndex);
    // -> f, env, meta, function
    lua_setfield(L, -2, "__newindex");
    // -> f, env, meta

    // Set the meta table for our environment.
    lua_setmetatable(L, -2);
    // -> f, env

    // Apply the new environment.
    lua_setupvalue(L, -2, n);
    // -> f

    return true;
}

int MP_Lua_AddJobMeta(lua_State* L) {
    char filename[128];
    bool isEnvSetup = false;

    // New type, start with defaults.
    MP_JobMeta meta = gMetaDefaults;

    // Validate input.
    luaL_argcheck(L, lua_gettop(L) == 1 && lua_isstring(L, 1), 0, "must specify one string");

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

    // OK, register metatables for types.
    MP_Lua_RegisterBlock(meta.L);
    MP_Lua_RegisterRoom(meta.L);
    MP_Lua_RegisterUnit(meta.L);

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
        MP_Lua_pushunit(L, unit);
        if (lua_pcall(L, 1, 0, 0) == LUA_OK) {
            return;
        } else {
            // Something went wrong.
            MP_log_error("Something bad happened in event handler '%s' for job '%s':\n%s\n", eventName, meta->name, lua_tostring(L, -1));
        }
    } else {
        lua_pop(L, 1);
        MP_log_error("Event handler '%s' for job '%s' isn't a function anymore.\n", eventName, meta->name);
    }

    // We get here only on failure. In that case disable the event callback,
    // so we don't try this again.
    MP_log_info("Disabling event callback '%s' for job '%s'.\n", eventName, meta->name);
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
