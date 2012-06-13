#include "script_loading.h"

#include "job.h"
#include "log.h"
#include "meta_job.h"
#include "script_loading_aux.h"

static int lua_ImportJob(lua_State* L) {
    MP_Lua_BuildImportPath(L, "data/jobs/%s.lua");
    MP_Lua_Import(L);
    return 0;
}

static const luaL_Reg joblib[] = {
    {"import", lua_ImportJob},
    {"export", MP_Lua_Export},
    {NULL, NULL}
};

int MP_Lua_AddJobMeta(lua_State* L) {
    // New type, start with defaults.
    MP_JobMeta meta = *MP_GetJobMetaDefaults();

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

    // Load the script.
    MP_PushScriptGlobals(L, joblib);
    MP_CreateScriptLocalsTable(L);

    lua_ImportJob(L);

    // Check script capabilities. First, check for event callbacks.
    for (unsigned int jobEvent = 0; jobEvent < MP_JOB_EVENT_COUNT; ++jobEvent) {
        lua_getglobal(L, JOB_EVENT_NAME[jobEvent]);
        if (lua_isfunction(L, -1)) {
            meta.handlesEvent[jobEvent] = true;
            MP_log_info("Found event handler '%s'.\n", JOB_EVENT_NAME[jobEvent]);
        }
        lua_pop(L, 1); // pop field
    }

    // Then check for dynamic preference callback.
    lua_getglobal(L, "preference");
    if (lua_isfunction(L, -1)) {
        meta.hasDynamicPreference = true;
        MP_log_info("Found dynamic preference callback.\n");
    }
    lua_pop(L, 1); // pop field

    // And finally for the run callback (job active logic).
    lua_getglobal(L, "run");
    if (lua_isfunction(L, -1)) {
        meta.hasRunMethod = true;
        MP_log_info("Found run callback.\n");
    }
    lua_pop(L, 1); // pop field

    // Try to add the job.
    if (!MP_AddJobMeta(&meta)) {
        return luaL_argerror(L, 1, "bad job meta");
    }

    MP_log_info("Done loading job '%s'.\n", meta.name);

    MP_PopScriptGlobals(L);
    return 0;
}
