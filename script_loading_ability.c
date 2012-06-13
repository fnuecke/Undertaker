#include "script_loading.h"

#include "ability.h"
#include "log.h"
#include "meta_ability.h"
#include "script_loading_aux.h"

int lua_ImportAbility(lua_State* L) {
    MP_Lua_BuildImportPath(L, "data/abilities/%s.lua");
    MP_Lua_Import(L);
    return 0;
}

static const luaL_Reg abilitylib[] = {
    {"import", lua_ImportAbility},
    {"export", MP_Lua_Export},
    {NULL, NULL}
};

int MP_Lua_AddAbilityMeta(lua_State* L) {
    // New type, start with defaults.
    MP_AbilityMeta meta = *MP_GetAbilityMetaDefaults();

    // Validate input.
    luaL_argcheck(L, lua_gettop(L) == 1 && lua_isstring(L, 1), 0, "one 'string' expected");

    // Get name.
    meta.name = lua_tostring(L, 1);

    // Skip if we already know this job (no overrides for job types).
    if (MP_GetAbilityMetaByName(meta.name)) {
        MP_log_info("Duplicate ability declaration for '%s', skipping.\n", meta.name);
        return 0;
    }

    MP_log_info("Start loading ability '%s'.\n", meta.name);

    // Load the script.
    MP_PushScriptGlobals(L, abilitylib);
    MP_CreateScriptLocalsTable(L);

    lua_ImportAbility(L);

    // Check script capabilities. Check for the run callback (job active logic).
    lua_getglobal(L, "run");
    if (lua_isfunction(L, -1)) {
        meta.hasRunMethod = true;
        MP_log_info("Found run callback.\n");
    }
    lua_pop(L, 1); // pop field

    // Try to add the job.
    if (!MP_AddAbilityMeta(&meta)) {
        return luaL_argerror(L, 1, "bad ability meta");
    }

    MP_log_info("Done loading job '%s'.\n", meta.name);

    MP_PopScriptGlobals(L);
    return 0;
}