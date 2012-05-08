#include "jobs_meta.h"

#include "jobs_events.h"

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

int MP_Lua_AddJobMeta(lua_State* L) {
    char filename[128];

    // New type, start with defaults.
    MP_JobMeta meta = gMetaDefaults;

    // Validate input.
    luaL_argcheck(L, lua_gettop(L) == 1 && lua_isstring(L, 1), 0, "must specify one string");

    // Get name.
    meta.name = lua_tostring(L, 1);

    // Skip if we already know this job (no overrides for job types).
    if (MP_GetJobMetaByName(meta.name)) {
        fprintf(MP_log_target, "INFO: Duplicate job declaration for '%s', skipping.\n", meta.name);
        return 0;
    }

    // Load AI script.
    meta.L = luaL_newstate();

    // Build file name.
    if (snprintf(filename, sizeof (filename), "data/ai/%s.lua", meta.name) > (int) sizeof (filename)) {
        lua_close(meta.L);
        return luaL_argerror(L, 1, "job name too long");
    }

    fprintf(MP_log_target, "INFO: Start parsing job file '%s'.\n", filename);

    // Try to parse the file.
    if (luaL_dofile(meta.L, filename) != LUA_OK) {
        fprintf(MP_log_target, "ERROR: Failed parsing job file: %s\n", lua_tostring(meta.L, -1));
        lua_close(meta.L);
        return luaL_argerror(L, 1, "invalid job script");
    }

    // Check script capabilities. First, check for event callbacks.
    for (unsigned int jobEvent = 0; jobEvent < MP_JOB_EVENT_COUNT; ++jobEvent) {
        lua_getglobal(meta.L, JOB_EVENT_NAME[jobEvent]);
        meta.handlesEvent[jobEvent] = lua_isfunction(meta.L, -1);
        lua_pop(meta.L, 1);
        if (meta.handlesEvent[jobEvent]) {
            fprintf(MP_log_target, "INFO: Found event handler '%s'.\n", JOB_EVENT_NAME[jobEvent]);
        }
    }

    // Then check for dynamic preference callback.
    lua_getglobal(meta.L, "preference");
    meta.hasDynamicPreference = lua_isfunction(meta.L, -1);
    lua_pop(meta.L, 1);
    if (meta.hasDynamicPreference) {
        fprintf(MP_log_target, "INFO: Found dynamic preference callback.\n");
    }

    // And finally for the run callback (job active logic).
    lua_getglobal(meta.L, "run");
    meta.hasRunMethod = lua_isfunction(meta.L, -1);
    lua_pop(meta.L, 1);
    if (meta.hasRunMethod) {
        fprintf(MP_log_target, "INFO: Found run callback.\n");
    }

    // Try to add the job.
    if (!MP_AddJobMeta(&meta)) {
        lua_close(meta.L);
        return luaL_argerror(L, 1, "bad job meta");
    }

    fprintf(MP_log_target, "INFO: Done parsing job file '%s'.\n", filename);

    return 0;
}

///////////////////////////////////////////////////////////////////////////////
// Events
///////////////////////////////////////////////////////////////////////////////

static void onUnitAdded(MP_JobMeta* meta, MP_Unit* unit) {
    lua_State* L = meta->L;
    // Try to get the callback.
    lua_getglobal(L, "preference");
    if (lua_isfunction(L, -1)) {
        // Call it.
        // TODO parameters; at least the unit this concerns.
        if (lua_pcall(L, 0, 0, 0) == LUA_OK) {
            return;
        } else {
            // Something went wrong.
            fprintf(MP_log_target, "ERROR: Something bad happened in event handler '%s' for job '%s': %s\n", JOB_EVENT_NAME[MP_JOB_EVENT_UNIT_ADDED], meta->name, lua_tostring(L, -1));
            lua_pop(L, 1);
        }
    } else {
        fprintf(MP_log_target, "ERROR: Event handler '%s' for job '%s' isn't a function anymore.\n", JOB_EVENT_NAME[MP_JOB_EVENT_UNIT_ADDED], meta->name);
    }

    // We get here only on failure. In that case disable the event callback,
    // so we don't try this again.
    fprintf(MP_log_target, "INFO: Disabling event callback '%s' for job '%s'.\n", JOB_EVENT_NAME[MP_JOB_EVENT_UNIT_ADDED], meta->name);
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
