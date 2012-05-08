#include "jobs_meta.h"

static const char* JOB_EVENT_NAME[MP_JOB_EVENT_COUNT] = {
    [MP_JOB_EVENT_UNIT_ADDED] = "onUnitAdded",
    [MP_JOB_EVENT_BLOCK_SELECTION_CHANGED] = "onBlockSelectionChanged",
    [MP_JOB_EVENT_BLOCK_DESTROYED] = "onBlockDestroyed",
    [MP_JOB_EVENT_BLOCK_CONVERTED] = "onBlockConverted"
};

META_globals(MP_JobMeta)

/** Reset defaults on map change */
static void resetDefaults(void) {
    for (unsigned eventId = 0; eventId < MP_JOB_EVENT_COUNT; ++eventId) {
        gMetaDefaults.handledEvents[eventId] = false;
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

void MP_DisableJobEvent(const MP_JobMeta* meta, MP_JobEvent event) {
    if (meta) {
        // Get non-const pointer...
        for (unsigned int i = 0; i < gMetaCount; ++i) {
            if (gMetas[i].id == meta->id) {
                // Modify.
                gMetas[i].handledEvents[event] = false;
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

int MP_Lua_AddJobMeta(lua_State* L) {
    char filename[128];

    // New type, start with defaults.
    MP_JobMeta meta = gMetaDefaults;

    // Validate input.
    luaL_argcheck(L, lua_gettop(L) == 1 && lua_isstring(L, 1), 0, "must specify one string");

    // Get name.
    meta.name = lua_tostring(L, 1);

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
        meta.handledEvents[jobEvent] = lua_isfunction(meta.L, -1);
        lua_pop(meta.L, 1);
        if (meta.handledEvents[jobEvent]) {
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
