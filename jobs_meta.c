#include "jobs_meta.h"

static const char* JOB_EVENT_NAME[DK_JOB_EVENT_COUNT] = {
    [DK_JOB_EVENT_UNIT_ADDED] = "onUnitAdded",
    [DK_JOB_EVENT_BLOCK_SELECTION_CHANGED] = "onBlockSelectionChanged",
    [DK_JOB_EVENT_BLOCK_DESTROYED] = "onBlockDestroyed",
    [DK_JOB_EVENT_BLOCK_CONVERTED] = "onBlockConverted"
};

META_globals(DK_JobMeta)

/** Reset defaults on map change */
static void resetDefaults(void) {
    for (unsigned eventId = 0; eventId < DK_JOB_EVENT_COUNT; ++eventId) {
        gMetaDefaults.handledEvents[eventId] = false;
    }
    gMetaDefaults.hasDynamicPreference = false;
    gMetaDefaults.hasRunMethod = false;
}

/** New type registered */
inline static bool initMeta(DK_JobMeta* m, const DK_JobMeta* meta) {
    *m = *meta;

    return true;
}

/** Type override */
inline static bool updateMeta(DK_JobMeta* m, const DK_JobMeta* meta) {
    return true;
}

/** Clear up data for a meta on deletion */
inline static void deleteMeta(DK_JobMeta* m) {
    lua_close(m->L);
    m->L = NULL;
}

META_impl(DK_JobMeta, Job)

void DK_DisableJobEvent(const DK_JobMeta* meta, DK_JobEvent event) {
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

void DK_DisableDynamicPreference(const DK_JobMeta* meta) {
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

void DK_DisableRunMethod(const DK_JobMeta* meta) {
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

int DK_Lua_AddJobMeta(lua_State* L) {
    char filename[128];

    // New type, start with defaults.
    DK_JobMeta meta = gMetaDefaults;

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

    // Try to parse the file.
    if (luaL_dofile(meta.L, filename) != LUA_OK) {
        fprintf(DK_log_target, "ERROR: Failed parsing job file: %s\n", lua_tostring(meta.L, -1));
        lua_close(meta.L);
        return luaL_argerror(L, 1, "invalid job script");
    }

    // Check script capabilities. First, check for event callbacks.
    for (unsigned int jobEvent = 0; jobEvent < DK_JOB_EVENT_COUNT; ++jobEvent) {
        lua_getglobal(meta.L, JOB_EVENT_NAME[jobEvent]);
        meta.handledEvents[jobEvent] = lua_isfunction(meta.L, -1);
        lua_pop(meta.L, 1);
    }

    // Then check for dynamic preference callback.
    lua_getglobal(meta.L, "preference");
    meta.hasDynamicPreference = lua_isfunction(meta.L, -1);
    lua_pop(meta.L, 1);

    // And finally for the run callback (job active logic).
    lua_getglobal(meta.L, "run");
    meta.hasRunMethod = lua_isfunction(meta.L, -1);
    lua_pop(meta.L, 1);

    // Try to add the job.
    if (!DK_AddJobMeta(&meta)) {
        lua_close(meta.L);
        return luaL_argerror(L, 1, "bad job meta");
    }

    return 0;
}
