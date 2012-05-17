#include "meta_unit.h"

#include "meta_job.h"
#include "passability.h"
#include "job_script.h"

///////////////////////////////////////////////////////////////////////////////
// Constants / globals
///////////////////////////////////////////////////////////////////////////////

META_globals(MP_UnitMeta)

///////////////////////////////////////////////////////////////////////////////
// Helper methods
///////////////////////////////////////////////////////////////////////////////

static MP_UnitJobSaturationMeta* addOrGetJobSaturation(MP_UnitMeta* meta, const MP_JobMeta* job) {
    for (unsigned int number = 0; number < meta->jobCount; ++number) {
        if (strcmp(meta->jobs[number]->name, job->name) == 0) {
            return &meta->satisfaction.jobSaturation[number];
        }
    }
    ++meta->jobCount;
    meta->jobs = realloc(meta->jobs, meta->jobCount * sizeof (MP_UnitJobSaturationMeta));
    meta->jobs[meta->jobCount - 1] = job;
    meta->satisfaction.jobSaturation = realloc(meta->satisfaction.jobSaturation, meta->jobCount * sizeof (MP_UnitJobSaturationMeta));
    meta->satisfaction.jobSaturation[meta->jobCount - 1].boredThreshold = 1;
    meta->satisfaction.jobSaturation[meta->jobCount - 1].initialValue = 0;
    meta->satisfaction.jobSaturation[meta->jobCount - 1].notPerformingDelta = 0;
    meta->satisfaction.jobSaturation[meta->jobCount - 1].performingDelta = 0;
    meta->satisfaction.jobSaturation[meta->jobCount - 1].preference = 0;
    meta->satisfaction.jobSaturation[meta->jobCount - 1].satisfiedThreshold = 1;
    meta->satisfaction.jobSaturation[meta->jobCount - 1].unsatisfiedThreshold = 0;
    return &meta->satisfaction.jobSaturation[meta->jobCount - 1];
}

static float luaMP_checkthreshold(lua_State* L, int narg) {
    float value = luaL_checknumber(L, narg);
    if (value < 0.0f) {
        return 0.0f;
    } else if (value > 1.0f) {
        return 1.0f;
    }
    return value;
}

static int tableToJobInfo(lua_State* L, MP_UnitMeta* meta) {
    // Keep track at which table entry we are.
    int narg = 1;

    // What do we use for defaults?
    const MP_JobMeta* job;
    MP_UnitJobSaturationMeta* saturation;

    // Get job meta data.
    lua_getfield(L, -1, "name");
    job = luaMP_checkjobmeta(L, -1, narg);
    lua_pop(L, 1);

    // Get or add the job type to the list of jobs the unit can perform.
    saturation = addOrGetJobSaturation(meta, job);

    // Now loop through the table. Push initial 'key' -- nil means start.
    lua_pushnil(L);
    while (lua_next(L, -2)) {
        // Get key as string.
        const char* key;
        luaL_argcheck(L, lua_type(L, -2) == LUA_TSTRING, narg, "keys must be strings");
        key = lua_tostring(L, -2);

        // See what we have.
        if (strcmp(key, "name") == 0) {
            // Silently skip it.

        } else if (strcmp(key, "preference") == 0) {
            saturation->preference = luaL_checknumber(L, -1);

        } else if (strcmp(key, "performing") == 0) {
            saturation->performingDelta = luaL_checknumber(L, -1);

        } else if (strcmp(key, "notperforming") == 0) {
            saturation->notPerformingDelta = luaL_checknumber(L, -1);

        } else if (strcmp(key, "initial") == 0) {
            saturation->initialValue = luaMP_checkthreshold(L, -1);

        } else if (strcmp(key, "unsatisfied") == 0) {
            saturation->unsatisfiedThreshold = luaMP_checkthreshold(L, -1);

        } else if (strcmp(key, "satisfied") == 0) {
            saturation->satisfiedThreshold = luaMP_checkthreshold(L, -1);

        } else if (strcmp(key, "bored") == 0) {
            saturation->boredThreshold = luaMP_checkthreshold(L, -1);

        } else {
            return luaL_argerror(L, narg, "unknown key");
        }

        lua_pop(L, 1); // pop value

        ++narg;
    }

    return 0;
}

/** Lua table parsing */
static int tableToUnit(lua_State* L, MP_UnitMeta* meta, bool forDefaults) {
    // Keep track at which table entry we are.
    int narg = 1;

    // What do we use for defaults?
    if (!forDefaults) {
        const char* name;
        const MP_UnitMeta* existing;

        // Get the name.
        lua_getfield(L, -1, "name");
        luaL_argcheck(L, lua_type(L, -1) == LUA_TSTRING, narg, "no 'name' or not a string");
        name = lua_tostring(L, -1);
        lua_pop(L, 1);
        luaL_argcheck(L, strlen(name) > 0, narg, "'name' must not be empty");

        // Check if that type is already known (override).
        if ((existing = MP_GetUnitMetaByName(name))) {
            // Yes, this will be an override.
            *meta = *existing;
        } else {
            // New entry.
            *meta = gMetaDefaults;
            meta->name = name;
        }
    } // else meta already equals gMetaDefaults

    // Now loop through the table. Push initial 'key' -- nil means start.
    lua_pushnil(L);
    while (lua_next(L, -2)) {

        // Get key as string.
        const char* key;
        luaL_argcheck(L, lua_type(L, -2) == LUA_TSTRING, narg, "keys must be strings");
        key = lua_tostring(L, -2);

        // See what we have.
        if (strcmp(key, "name") == 0) {
            if (forDefaults) {
                return luaL_argerror(L, narg, "'name' not allowed in defaults");
            }

        } else if (strcmp(key, "canpass") == 0) {
            // This can be either a string or a table of strings.
            if (lua_istable(L, -1)) {
                // It's a table. Loop through that table in turn.
                meta->canPass = 0;
                lua_pushnil(L);
                while (lua_next(L, -2)) {
                    meta->canPass |= luaMP_checkpassability(L, -1, narg);
                    lua_pop(L, 1); // pop value
                }
            } else {
                // Must be a single string in that case.
                meta->canPass = luaMP_checkpassability(L, -1, narg);
            }

        } else if (strcmp(key, "maxgold") == 0) {
            meta->goldCapacity = luaL_checkunsigned(L, -1);

        } else if (strcmp(key, "movespeed") == 0) {
            meta->moveSpeed = luaL_checknumber(L, -1);

        } else if (strcmp(key, "jobs") == 0) {
            if (forDefaults) {
                return luaL_argerror(L, narg, "'jobs' not allowed in defaults");
            } else {
                luaL_checktype(L, -1, LUA_TTABLE);
                lua_pushnil(L);
                while (lua_next(L, -2)) {
                    luaL_checktype(L, -1, LUA_TTABLE);
                    tableToJobInfo(L, meta);
                    lua_pop(L, 1); // pop value
                }
            }

        } else if (strcmp(key, "angrybelow") == 0) {
            meta->satisfaction.angerThreshold = luaMP_checkthreshold(L, -1);

        } else if (strcmp(key, "angerjob") == 0) {
            meta->satisfaction.angerJob = luaMP_checkjobmeta(L, -1, narg);

        } else {
            return luaL_argerror(L, narg, "unknown key");
        }

        lua_pop(L, 1); // pop value

        ++narg;
    }

    return 0;
}

///////////////////////////////////////////////////////////////////////////////
// Meta implementation
///////////////////////////////////////////////////////////////////////////////

/** Reset defaults on map change */
static void resetDefaults(void) {
    gMetaDefaults.canPass = 0;
    gMetaDefaults.moveSpeed = 1.0f;
}

/** New type registered */
inline static bool initMeta(MP_UnitMeta* m, const MP_UnitMeta* meta) {
    *m = *meta;

    return true;
}

/** Type override */
inline static bool updateMeta(MP_UnitMeta* m, const MP_UnitMeta* meta) {
    m->canPass = meta->canPass;
    m->moveSpeed = meta->moveSpeed;
    m->satisfaction = meta->satisfaction;
    m->goldCapacity = meta->goldCapacity;

    return true;
}

/** Clear up data for a meta on deletion */
inline static void deleteMeta(MP_UnitMeta* m) {
    free(m->jobs);
    m->jobs = NULL;
    free(m->satisfaction.jobSaturation);
    m->satisfaction.jobSaturation = NULL;
    m->jobCount = 0;
}

META_impl(MP_UnitMeta, Unit)

int MP_Lua_UnitMetaDefaults(lua_State* L) {
    // Validate input.
    luaL_argcheck(L, lua_gettop(L) == 1 && lua_istable(L, 1), 0, "one 'table' expected");

    // Build the block meta using the given properties.
    tableToUnit(L, &gMetaDefaults, true /* for defaults */);

    return 0;
}

int MP_Lua_AddUnitMeta(lua_State* L) {
    // New type, start with defaults.
    MP_UnitMeta meta = gMetaDefaults;

    // Validate input.
    luaL_argcheck(L, lua_gettop(L) == 1 && lua_istable(L, 1), 0, "one 'table' expected");

    // Build the block meta using the given properties.
    tableToUnit(L, &meta, false /* not for defaults */);

    // We require for at least the name to be set.
    luaL_argcheck(L, meta.name != NULL, 1, "'name' is required but not set");
    luaL_argcheck(L, meta.canPass != MP_PASSABILITY_NONE, 1, "'canpass' is required but not set");
    luaL_argcheck(L, meta.jobCount > 0, 1, "unit must have at least one job");

    // All green, add the type.
    if (!MP_AddUnitMeta(&meta)) {
        return luaL_argerror(L, 1, "bad unit meta");
    }

    return 0;
}
