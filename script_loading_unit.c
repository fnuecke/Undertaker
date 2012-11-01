#include "ability_type.h"
#include "job_type.h"
#include "script.h"
#include "type_impl.h"
#include "unit.h"

// <editor-fold defaultstate="collapsed" desc="ability parsers">

static void properties(lua_State* L, MP_UnitAbilityType* type, bool forDefaults) {
    luaL_checktype(L, -1, LUA_TTABLE);
    lua_rawgeti(L, LUA_REGISTRYINDEX, type->properties);
    // Copy / overwrite.
    lua_pushnil(L);
    while (lua_next(L, -3)) {
        lua_pushvalue(L, -2); // duplicate key
        lua_insert(L, -2); // and move it in front of our value
        lua_settable(L, -4); // then add it to the new table
    }
    lua_pop(L, 1); // pop ability type properties table
}

static MP_UnitAbilityType* getAbilityTarget(lua_State* L, MP_UnitType* unitType) {
    // Get ability type data to modify or add the ability type to the list of
    // abilities the unit can perform.
    const MP_AbilityType* abilityType;
    lua_getfield(L, -1, "name");
    abilityType = MP_Lua_CheckAbilityType(L, -1);
    lua_pop(L, 1);
    for (unsigned int number = 0; number < unitType->abilityCount; ++number) {
        if (strcmp(unitType->abilities[number].type->info.name, abilityType->info.name) == 0) {
            return &unitType->abilities[number];
        }
    }

    // Does not exist yet for this unit, expand the list and add it.
    ++unitType->abilityCount;
    unitType->abilities = realloc(unitType->abilities, unitType->abilityCount * sizeof (MP_UnitAbilityType));

    // Initialize it and return.
    unitType->abilities[unitType->abilityCount - 1].type = abilityType;
    lua_newtable(L);
    unitType->abilities[unitType->abilityCount - 1].properties = luaL_ref(L, LUA_REGISTRYINDEX);
    return &unitType->abilities[unitType->abilityCount - 1];
}

TYPE_PARSER(MP_UnitAbilityType, Ability, getAbilityTarget(L, unitType), MP_UnitType* unitType)
// </editor-fold>

static const AbilityParserEntry abilityParsers[] = {
    {"properties", properties},
    {NULL, NULL}
};

// <editor-fold defaultstate="collapsed" desc="job parsers">

static float checkthreshold(lua_State* L, int narg) {
    float value = luaL_checknumber(L, narg);
    if (value < 0.0f) {
        return 0.0f;
    } else if (value > 1.0f) {
        return 1.0f;
    }
    return value;
}

static void preference(lua_State* L, MP_UnitJobType* type, bool forDefaults) {
    type->preference = luaL_checknumber(L, -1);
}

static void initial(lua_State* L, MP_UnitJobType* type, bool forDefaults) {
    type->initialSaturation = checkthreshold(L, -1);
}

static void angrybelow(lua_State* L, MP_UnitJobType* type, bool forDefaults) {
    type->angerThreshold = checkthreshold(L, -1);
}

static void unsatisfied(lua_State* L, MP_UnitJobType* type, bool forDefaults) {
    type->unsatisfiedThreshold = checkthreshold(L, -1);
}

static void satisfied(lua_State* L, MP_UnitJobType* type, bool forDefaults) {
    type->satisfiedThreshold = checkthreshold(L, -1);
}

static void bored(lua_State* L, MP_UnitJobType* type, bool forDefaults) {
    type->boredThreshold = checkthreshold(L, -1);
}

static void performing(lua_State* L, MP_UnitJobType* type, bool forDefaults) {
    type->performingDelta = luaL_checknumber(L, -1);
}

static void notperforming(lua_State* L, MP_UnitJobType* type, bool forDefaults) {
    type->notPerformingDelta = luaL_checknumber(L, -1);
}

static MP_UnitJobType* getJobTarget(lua_State* L, MP_UnitType* unitType) {
    // Get job meta data to get or add the job type to the list of jobs the unit
    // can perform.
    const MP_JobType* jobType;
    lua_getfield(L, -1, "name");
    jobType = MP_Lua_CheckJobType(L, -1);
    lua_pop(L, 1);
    for (unsigned int number = 0; number < unitType->jobCount; ++number) {
        if (strcmp(unitType->jobs[number].type->info.name, jobType->info.name) == 0) {
            return &unitType->jobs[number];
            break;
        }
    }

    // Does not exist yet for this unit, expand the list and add it.
    ++unitType->jobCount;
    unitType->jobs = realloc(unitType->jobs, unitType->jobCount * sizeof (MP_UnitJobType));

    // Initialize it and return.
    unitType->jobs[unitType->jobCount - 1].type = jobType;
    unitType->jobs[unitType->jobCount - 1].preference = 0;
    unitType->jobs[unitType->jobCount - 1].initialSaturation = 0;
    unitType->jobs[unitType->jobCount - 1].angerThreshold = 0;
    unitType->jobs[unitType->jobCount - 1].unsatisfiedThreshold = 0;
    unitType->jobs[unitType->jobCount - 1].satisfiedThreshold = 1;
    unitType->jobs[unitType->jobCount - 1].boredThreshold = 1;
    unitType->jobs[unitType->jobCount - 1].performingDelta = 0;
    unitType->jobs[unitType->jobCount - 1].notPerformingDelta = 0;
    return &unitType->jobs[unitType->jobCount - 1];
}

TYPE_PARSER(MP_UnitJobType, Job, getJobTarget(L, unitType), MP_UnitType* unitType)
// </editor-fold>

static const JobParserEntry jobParsers[] = {
    {"preference", preference},
    {"initial", initial},
    {"angry", angrybelow},
    {"unsatisfied", unsatisfied},
    {"satisfied", satisfied},
    {"bored", bored},
    {"performing", performing},
    {"notperforming", notperforming},
    {NULL, NULL}
};

// <editor-fold defaultstate="collapsed" desc="unit parsers">

static void canpass(lua_State* L, MP_UnitType* meta, bool forDefaults) {
    // This can be either a string or a table of strings.
    if (lua_istable(L, -1)) {
        // It's a table. Loop through that table in turn.
        meta->canPass = 0;
        lua_pushnil(L);
        while (lua_next(L, -2)) {
            meta->canPass |= MP_Lua_CheckPassability(L, -1);
            lua_pop(L, 1); // pop value
        }
    } else {
        // Must be a single string in that case.
        meta->canPass = MP_Lua_CheckPassability(L, -1);
    }
}

static void movespeed(lua_State* L, MP_UnitType* type, bool forDefaults) {
    type->moveSpeed = luaL_checknumber(L, -1);
}

static void abilities(lua_State* L, MP_UnitType* type, bool forDefaults) {
    if (forDefaults) {
        luaL_where(L, 0);
        MP_log_warning("%s: 'abilities' invalid in this context, ignored.\n", lua_tostring(L, -1));
        lua_pop(L, 1);
    } else {
        luaL_checktype(L, -1, LUA_TTABLE);
        lua_pushnil(L);
        while (lua_next(L, -2)) {
            luaL_checktype(L, -1, LUA_TTABLE);
            parseAbilityTable(L, abilityParsers, false, type);
            lua_pop(L, 1); // pop value
        }
    }
}

static void jobs(lua_State* L, MP_UnitType* type, bool forDefaults) {
    if (forDefaults) {
        luaL_where(L, 0);
        MP_log_warning("'jobs' invalid in this context, ignored.\n", lua_tostring(L, -1));
        lua_pop(L, 1);
    } else {
        luaL_checktype(L, -1, LUA_TTABLE);
        lua_pushnil(L);
        while (lua_next(L, -2)) {
            luaL_checktype(L, -1, LUA_TTABLE);
            parseJobTable(L, jobParsers, false, type);
            lua_pop(L, 1); // pop value
        }
    }
}

static void angerjob(lua_State* L, MP_UnitType* type, bool forDefaults) {
    type->angerJob = MP_Lua_CheckJobType(L, -1);
}

static MP_UnitType* getUnitTarget(lua_State* L, MP_UnitType* type, bool forDefaults) {
    // What do we use for defaults?
    if (!forDefaults) {
        // See if the type has been parsed before.
        const MP_UnitType* existing;

        // Get the name.
        lua_getfield(L, 1, "name");
        // Check if that type is already known (override).
        if ((existing = MP_GetUnitTypeByName(luaL_checkstring(L, -1)))) {
            // Yes, this will be an override. Copy known values.
            *type = *existing;
        } else {
            // New entry. Copy default values and set name.
            *type = *MP_GetUnitTypeDefaults();
            type->info.name = lua_tostring(L, -1);
        }
        lua_pop(L, 1); // pop name
    } // else: type already is a reference to the defaults.
    return type;
}

TYPE_PARSER(MP_UnitType, Unit, getUnitTarget(L, type, forDefaults), MP_UnitType* type)
// </editor-fold>

static const UnitParserEntry unitParsers[] = {
    {"canpass", canpass},
    {"movespeed", movespeed},
    {"abilities", abilities},
    {"jobs", jobs},
    {"angerjob", angerjob},
    {NULL, NULL}
};

int MP_LuaCallback_SetUnitTypeDefaults(lua_State* L) {
    // Build the block meta using the given properties.
    parseUnitTable(L, unitParsers, true, MP_GetUnitTypeDefaults());

    return 0;
}

int MP_LuaCallback_AddUnitType(lua_State* L) {
    // New type, start with defaults.
    MP_UnitType type;

    // Build the block meta using the given properties.
    parseUnitTable(L, unitParsers, false, &type);

    // We require for at least the name to be set.
    luaL_argcheck(L, type.info.name != NULL && strlen(type.info.name) > 0, 1, "invalid or no 'name'");
    luaL_argcheck(L, type.canPass != MP_PASSABILITY_NONE, 1, "'canpass' is required but not set");
    luaL_argcheck(L, type.jobCount > 0, 1, "unit must have at least one job");

    // All green, add the type.
    if (!MP_AddUnitType(&type)) {
        return luaL_argerror(L, 1, "bad unit meta");
    }

    return 0;
}
