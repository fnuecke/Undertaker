#include <string.h>

#include "job.h"
#include "job_type.h"
#include "script.h"

///////////////////////////////////////////////////////////////////////////////
// Getters
///////////////////////////////////////////////////////////////////////////////

typedef struct JobIter {
    MP_JobList jobs;
    unsigned int i;
} JobIter;

static int lua_JobIter(lua_State* L) {
    JobIter* iter = (JobIter*) lua_touserdata(L, lua_upvalueindex(1));

    // We iterate back to front, to allow deletion of current entry.
    if (iter->i > 0) {
        MP_Lua_PushJob(L, iter->jobs[--iter->i]);
        return 1;
    }

    return 0;
}

static int lua_GetByType(lua_State* L) {
    const MP_JobType* type = MP_Lua_CheckJobType(L, 1);
    const MP_Player player = MP_Lua_CheckPlayer(L, 2);
    JobIter* iter;

    if (player == MP_PLAYER_NONE) {
        return luaL_argerror(L, 2, "invalid player value");
    }

    iter = (JobIter*) lua_newuserdata(L, sizeof (JobIter));
    iter->jobs = MP_GetJobs(player, type, &iter->i);

    lua_pushcclosure(L, lua_JobIter, 1);

    return 1;
}

static int lua_GetOffset(lua_State* L) {
    const MP_Job* job = MP_Lua_CheckJob(L, 1);

    MP_Lua_PushVec2(L, job->offset);

    return 2;
}

static int lua_GetPosition(lua_State* L) {
    const MP_Job* job = MP_Lua_CheckJob(L, 1);

    MP_Lua_PushVec2(L, MP_GetJobPosition(job));

    return 2;
}

static int lua_GetTargetType(lua_State* L) {
    const MP_Job* job = MP_Lua_CheckJob(L, 1);

    lua_pushinteger(L, job->targetType);

    return 1;
}

static int lua_GetTarget(lua_State* L) {
    const MP_Job* job = MP_Lua_CheckJob(L, 1);

    switch (job->targetType) {
        case MP_JOB_TARGET_BLOCK:
            MP_Lua_PushBlock(L, (MP_Block*) job->target);
            break;
        case MP_JOB_TARGET_ROOM:
            MP_Lua_PushRoom(L, (MP_Room*) job->target);
            break;
        case MP_JOB_TARGET_UNIT:
            MP_Lua_PushUnit(L, (MP_Unit*) job->target);
            break;
        default:
            lua_pushnil(L);
            break;
    }

    return 1;
}

///////////////////////////////////////////////////////////////////////////////
// Methods
///////////////////////////////////////////////////////////////////////////////

static int lua_CreateJob(lua_State* L) {
    MP_Job* job;
    const MP_JobType* type;
    MP_Player player = MP_PLAYER_NONE;
    MP_JobTargetType targetType = MP_JOB_TARGET_NONE;
    void* target = NULL;
    vec2 offset = ZERO_VEC2;

    // Validate input.
    luaL_argcheck(L, lua_gettop(L) == 1 && lua_istable(L, 1), 0, "one 'table' expected");

    // Get job meta data.
    lua_getfield(L, 1, "name");
    type = MP_Lua_CheckJobType(L, -1);
    lua_pop(L, 1);

    // Now loop through the table. Push initial 'key' -- nil means start.
    lua_pushnil(L);
    // -> table, nil
    while (lua_next(L, 1)) {
        // -> table, key, value
        // Get key as string.
        const char* key;
        luaL_argcheck(L, lua_type(L, -2) == LUA_TSTRING, 1, "keys must be strings");
        key = lua_tostring(L, -2);

        // See what we have.
        if (strcmp(key, "name") == 0) {
            // Silently skip it.

        } else if (strcmp(key, "target") == 0) {
            if (MP_Lua_IsBlock(L, -1)) {
                targetType = MP_JOB_TARGET_BLOCK;
                target = MP_Lua_ToBlock(L, -1);
            } else if (MP_Lua_IsRoom(L, -1)) {
                targetType = MP_JOB_TARGET_ROOM;
                target = MP_Lua_ToRoom(L, -1);
            } else if (MP_Lua_IsUnit(L, -1)) {
                targetType = MP_JOB_TARGET_UNIT;
                target = MP_Lua_ToUnit(L, -1);
            } else {
                return luaL_argerror(L, 1, "invalid target type");
            }

        } else if (strcmp(key, "offset") == 0) {
            vec2 p = ZERO_VEC2;
            luaL_checktype(L, 1, LUA_TTABLE);
            // Test if we have named arguments or should use the first two.
            lua_getfield(L, -1, "x");
            if (lua_isnil(L, -1)) {
                // Unnamed args, reuse the nil as first key.
                int i = 0;
                while (lua_next(L, -2)) {
                    // Get offset.
                    p.v[i] = luaL_checknumber(L, -1);

                    lua_pop(L, 1); // pop value

                    // Stop when we have x and y.
                    if (++i > 1) {
                        lua_pop(L, 1); // pop key
                        break;
                    }
                }
            } else {
                // Got named arguments.
                p.d.x = luaL_checknumber(L, -1);
                lua_pop(L, 1);
                // We require both to be named in that case.
                lua_getfield(L, -1, "y");
                p.d.y = luaL_checknumber(L, -1);
                lua_pop(L, 1);
            }

            // All OK.
            offset = p;

        } else if (strcmp(key, "player") == 0) {
            player = MP_Lua_CheckPlayer(L, -1);

        } else {
            return luaL_argerror(L, 1, "unknown key");
        }

        lua_pop(L, 1);
    }

    luaL_argcheck(L, target != NULL, 1, "must set 'target'");
    luaL_argcheck(L, player != MP_PLAYER_NONE, 1, "must set 'player'");

    // Allocate job and set values.
    job = MP_NewJob(type, player);
    job->targetType = targetType;
    job->target = target;
    job->offset = offset;

    return 0;
}

static int lua_DeleteJob(lua_State* L) {
    MP_Job* job = MP_Lua_CheckJob(L, 1);

    MP_DeleteJob(job);

    return 0;
}

static int lua_DeleteJobWhereTarget(lua_State* L) {
    const MP_JobType* jobType = MP_Lua_CheckJobType(L, 1);
    const MP_Player player = MP_Lua_CheckPlayer(L, 3);

    if (player == MP_PLAYER_NONE) {
        return luaL_argerror(L, 3, "invalid player value");
    }

    // Get the target. Figure out what it is.
    if (MP_Lua_IsBlock(L, 2)) {
        const MP_Block* target = MP_Lua_ToBlock(L, 2);
        MP_DeleteJobsTargetingBlock(player, jobType, target);
    } else if (MP_Lua_IsRoom(L, 2)) {
        const MP_Room* target = MP_Lua_ToRoom(L, 2);
        MP_DeleteJobsTargetingRoom(player, jobType, target);
    } else if (MP_Lua_IsUnit(L, 2)) {
        const MP_Unit* target = MP_Lua_ToUnit(L, 2);
        MP_DeleteJobsTargetingUnit(player, jobType, target);
    } else {
        // Invalid target.
        return luaL_argerror(L, 2, "expected '" LUA_BLOCKLIBNAME "', '" LUA_ROOMLIBNAME "' or '" LUA_UNITLIBNAME "'");
    }

    return 0;
}

///////////////////////////////////////////////////////////////////////////////
// Registration / Push / Check
///////////////////////////////////////////////////////////////////////////////

static const luaL_Reg lib[] = {
    {"getByType", lua_GetByType},
    {"getOffset", lua_GetOffset},
    {"getPosition", lua_GetPosition},
    {"getTargetType", lua_GetTargetType},
    {"getTarget", lua_GetTarget},

    {"create", lua_CreateJob},
    {"delete", lua_DeleteJob},
    {"deleteByTypeWhereTarget", lua_DeleteJobWhereTarget},
    {NULL, NULL}
};

int MP_Lua_OpenJob(lua_State* L) {
    luaL_newlib(L, lib);
    return 1;
}
