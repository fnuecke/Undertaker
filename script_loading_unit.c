#include "unit.h"

#include "meta_impl.h"
#include "script.h"

static bool gForDefaults;

static float luaMP_checkthreshold(lua_State* L, int narg) {
    float value = luaL_checknumber(L, narg);
    if (value < 0.0f) {
        return 0.0f;
    } else if (value > 1.0f) {
        return 1.0f;
    }
    return value;
}

// <editor-fold defaultstate="collapsed" desc="job satisfaction parsers">
static void preference(lua_State* L, MP_UnitJobSaturationMeta* saturation) {
    saturation->preference = luaL_checknumber(L, -1);
}

static void performing(lua_State* L, MP_UnitJobSaturationMeta* saturation) {
    saturation->performingDelta = luaL_checknumber(L, -1);
}

static void notperforming(lua_State* L, MP_UnitJobSaturationMeta* saturation) {
    saturation->notPerformingDelta = luaL_checknumber(L, -1);
}

static void initial(lua_State* L, MP_UnitJobSaturationMeta* saturation) {
    saturation->initialValue = luaMP_checkthreshold(L, -1);
}

static void unsatisfied(lua_State* L, MP_UnitJobSaturationMeta* saturation) {
    saturation->unsatisfiedThreshold = luaMP_checkthreshold(L, -1);
}

static void satisfied(lua_State* L, MP_UnitJobSaturationMeta* saturation) {
    saturation->satisfiedThreshold = luaMP_checkthreshold(L, -1);
}

static void bored(lua_State* L, MP_UnitJobSaturationMeta* saturation) {
    saturation->boredThreshold = luaMP_checkthreshold(L, -1);
}

META_parser(MP_UnitJobSaturationMeta, Saturation,{
    // Get job meta data to get or add the job type to the list of jobs the unit
    // can perform.
    const MP_JobMeta* jobMeta;
    lua_getfield(L, -1, "name");
    jobMeta = luaMP_checkjobmeta(L, -1);
    lua_pop(L, 1);
    for (unsigned int number = 0; number < unitMeta->jobCount; ++number) {
        if (strcmp(unitMeta->jobs[number]->name, jobMeta->name) == 0) {
            target = &unitMeta->satisfaction.jobSaturation[number];
            break;
        }
    }
    if (!target) {
        ++unitMeta->jobCount;
        unitMeta->jobs = realloc(unitMeta->jobs, unitMeta->jobCount * sizeof (MP_UnitJobSaturationMeta));
        unitMeta->jobs[unitMeta->jobCount - 1] = jobMeta;
        unitMeta->satisfaction.jobSaturation = realloc(unitMeta->satisfaction.jobSaturation, unitMeta->jobCount * sizeof (MP_UnitJobSaturationMeta));
        unitMeta->satisfaction.jobSaturation[unitMeta->jobCount - 1].boredThreshold = 1;
        unitMeta->satisfaction.jobSaturation[unitMeta->jobCount - 1].initialValue = 0;
        unitMeta->satisfaction.jobSaturation[unitMeta->jobCount - 1].notPerformingDelta = 0;
        unitMeta->satisfaction.jobSaturation[unitMeta->jobCount - 1].performingDelta = 0;
        unitMeta->satisfaction.jobSaturation[unitMeta->jobCount - 1].preference = 0;
        unitMeta->satisfaction.jobSaturation[unitMeta->jobCount - 1].satisfiedThreshold = 1;
        unitMeta->satisfaction.jobSaturation[unitMeta->jobCount - 1].unsatisfiedThreshold = 0;
        target = &unitMeta->satisfaction.jobSaturation[unitMeta->jobCount - 1];
    }
}, false, MP_UnitMeta* unitMeta)
// </editor-fold>

static const SaturationParserEntry saturationParsers[] = {
    {"preference", preference},
    {"performing", performing},
    {"notperforming", notperforming},
    {"initial", initial},
    {"unsatisfied", unsatisfied},
    {"satisfied", satisfied},
    {"bored", bored},
    {NULL, NULL}
};

// <editor-fold defaultstate="collapsed" desc="unit parsers">
static void canpass(lua_State* L, MP_UnitMeta* meta) {
    // This can be either a string or a table of strings.
    if (lua_istable(L, -1)) {
        // It's a table. Loop through that table in turn.
        meta->canPass = 0;
        lua_pushnil(L);
        while (lua_next(L, -2)) {
            meta->canPass |= luaMP_checkpassability(L, -1);
            lua_pop(L, 1); // pop value
        }
    } else {
        // Must be a single string in that case.
        meta->canPass = luaMP_checkpassability(L, -1);
    }
}

static void maxgold(lua_State* L, MP_UnitMeta* meta) {
    meta->goldCapacity = luaL_checkunsigned(L, -1);
}

static void movespeed(lua_State* L, MP_UnitMeta* meta) {
    meta->moveSpeed = luaL_checknumber(L, -1);
}

static void jobs(lua_State* L, MP_UnitMeta* meta) {
    if (gForDefaults) {
        luaL_argerror(L, 1, "'jobs' not allowed in defaults");
    } else {
        luaL_checktype(L, -1, LUA_TTABLE);
        lua_pushnil(L);
        while (lua_next(L, -2)) {
            luaL_checktype(L, -1, LUA_TTABLE);
            parseSaturationTable(L, saturationParsers, meta);
            lua_pop(L, 1); // pop value
        }
    }
}

static void angrybelow(lua_State* L, MP_UnitMeta* meta) {
    meta->satisfaction.angerThreshold = luaMP_checkthreshold(L, -1);
}

static void angerjob(lua_State* L, MP_UnitMeta* meta) {
    meta->satisfaction.angerJob = luaMP_checkjobmeta(L, -1);
}

META_parser(MP_UnitMeta, Unit,{
    // What do we use for defaults?
    if (!gForDefaults) {
        const MP_UnitMeta* existing;

        // Get the name.
        lua_getfield(L, -1, "name");
        // Check if that type is already known (override).
        if ((existing = MP_GetUnitMetaByName(luaL_checkstring(L, -1)))) {
            // Yes, this will be an override.
            *meta = *existing;
        } else {
            // New entry.
            *meta = gMetaDefaults;
            meta->name = lua_tostring(L, -1);
        }
        lua_pop(L, 1); // pop name
    } // else meta already equals gMetaDefaults
    target = meta;
}, gForDefaults, MP_UnitMeta* meta)
// </editor-fold>

static const UnitParserEntry unitParsers[] = {
    {"canpass", canpass},
    {"maxgold", maxgold},
    {"movespeed", movespeed},
    {"jobs", jobs},
    {"angrybelow", angrybelow},
    {"angerjob", angerjob},
    {NULL, NULL}
};

int MP_Lua_UnitMetaDefaults(lua_State* L) {
    luaL_argcheck(L, lua_gettop(L) == 1, 1, "wrong number of arguments");

    // Build the block meta using the given properties.
    gForDefaults = true;
    parseUnitTable(L, unitParsers, MP_GetUnitMetaDefaults());

    return 0;
}

int MP_Lua_AddUnitMeta(lua_State* L) {
    // New type, start with defaults.
    MP_UnitMeta meta = *MP_GetUnitMetaDefaults();

    luaL_argcheck(L, lua_gettop(L) == 1, 1, "wrong number of arguments");

    // Build the block meta using the given properties.
    gForDefaults = false;
    parseUnitTable(L, unitParsers, &meta);

    // We require for at least the name to be set.
    luaL_argcheck(L, meta.name != NULL && strlen(meta.name) > 0, 1, "invalid or no 'name'");
    luaL_argcheck(L, meta.canPass != MP_PASSABILITY_NONE, 1, "'canpass' is required but not set");
    luaL_argcheck(L, meta.jobCount > 0, 1, "unit must have at least one job");

    // All green, add the type.
    if (!MP_AddUnitMeta(&meta)) {
        return luaL_argerror(L, 1, "bad unit meta");
    }

    return 0;
}
