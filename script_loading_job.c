#include "job.h"
#include "script.h"
#include "type_impl.h"
#include "job_type.h"
#include "script_events.h"

// <editor-fold defaultstate="collapsed" desc="job parsers">

static void run(lua_State* L, MP_JobType* type, bool forDefaults) {
    luaL_checktype(L, -1, LUA_TFUNCTION);
    if (type->runMethod != LUA_REFNIL) {
        luaL_unref(L, LUA_REGISTRYINDEX, type->runMethod);
    }
    lua_pushvalue(L, -1);
    type->runMethod = luaL_ref(L, LUA_REGISTRYINDEX);
}

static void events(lua_State* L, MP_JobType* type, bool forDefaults) {
    luaL_checktype(L, -1, LUA_TTABLE);
    lua_pushnil(L);
    while (lua_next(L, -2)) {
        const char* event = luaL_checkstring(L, -2);
        if (lua_isfunction(L, -1)) {
            if (strcmp(event, "onUnitAdded") == 0) {
                MP_Lua_RemoveUnitAddedEventListeners(L, type->info.name);
                MP_Lua_AddUnitAddedEventListener(L, type->info.name);
            } else if (strcmp(event, "onBlockSelectionChanged") == 0) {
                MP_Lua_RemoveBlockSelectionChangedEventListeners(L, type->info.name);
                MP_Lua_AddBlockSelectionChangedEventListener(L, type->info.name);
            } else if (strcmp(event, "onBlockTypeChanged") == 0) {
                MP_Lua_RemoveBlockTypeChangedEventListeners(L, type->info.name);
                MP_Lua_AddBlockTypeChangedEventListener(L, type->info.name);
            } else if (strcmp(event, "onBlockOwnerChanged") == 0) {
                MP_Lua_RemoveBlockOwnerChangedEventListeners(L, type->info.name);
                MP_Lua_AddBlockOwnerChangedEventListener(L, type->info.name);
            } else {
                luaL_where(L, 1);
                MP_log_warning("%s: ignoring unknown event name '%s'.", lua_tostring(L, -1), event);
                lua_pop(L, 1);
            }
        } else {
            luaL_where(L, 1);
            MP_log_warning("%s: event handler of job '%s' for event '%s' is not a function.\n", lua_tostring(L, -1), type->info.name, event);
            lua_pop(L, 1);
        }
        lua_pop(L, 1);
    }
}

static MP_JobType* getJobTarget(lua_State* L, MP_JobType* type) {
    const MP_JobType* existing;

    // Get the name.
    lua_getfield(L, -1, "name");
    // Check if that type is already known (override).
    if ((existing = MP_GetJobTypeByName(luaL_checkstring(L, -1)))) {
        // Yes, this will be an override.
        *type = *existing;
    } else {
        // New entry.
        *type = *MP_GetJobTypeDefaults();
        type->info.name = lua_tostring(L, -1);
    }
    lua_pop(L, 1); // pop name

    return type;
}

TYPE_PARSER(MP_JobType, Job, getJobTarget(L, type), MP_JobType* type)
// </editor-fold>

static const JobParserEntry jobParsers[] = {
    {"run", run},
    {"events", events},
    {NULL, NULL}
};

int MP_LuaCallback_AddJobType(lua_State* L) {
    // New type, start with defaults.
    MP_JobType type;

    luaL_argcheck(L, lua_gettop(L) == 1, 1, "wrong number of arguments");

    // Build the block meta using the given properties.
    parseJobTable(L, jobParsers, false, &type);

    // We require for at least the name to be set.
    luaL_argcheck(L, type.info.name != NULL && strlen(type.info.name) > 0, 1, "invalid or no 'name'");

    // All green, add the type.
    if (!MP_AddJobType(&type)) {
        return luaL_argerror(L, 1, "bad job meta");
    }

    return 0;
}
