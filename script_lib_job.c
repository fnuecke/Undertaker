#include <string.h>

#include "job.h"
#include "job_type.h"
#include "script.h"

///////////////////////////////////////////////////////////////////////////////
// Getters
///////////////////////////////////////////////////////////////////////////////

typedef struct JobIter {
    MP_Job* const* jobs;
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
    const MP_Player player = MP_Lua_CheckPlayer(L, 1);
    const MP_JobType* meta = MP_Lua_CheckJobType(L, 2);

    JobIter* iter = (JobIter*) lua_newuserdata(L, sizeof (JobIter));
    iter->jobs = MP_GetJobs(player, meta, &iter->i);

    lua_pushcclosure(L, lua_JobIter, 1);
    return 1;
}

static int lua_GetOffset(lua_State* L) {
    MP_Job* job = MP_Lua_CheckJob(L, 1);
    lua_pushnumber(L, job->offset.d.x);
    lua_pushnumber(L, job->offset.d.y);
    return 2;
}

static int lua_GetPosition(lua_State* L) {
    MP_Job* job = MP_Lua_CheckJob(L, 1);
    vec2 p;
    MP_GetJobPosition(&p, job);
    lua_pushnumber(L, p.d.x);
    lua_pushnumber(L, p.d.y);
    return 2;
}

static int lua_GetTargetType(lua_State* L) {
    lua_pushinteger(L, MP_Lua_CheckJob(L, 1)->targetType);
    return 1;
}

static int lua_GetTarget(lua_State* L) {
    MP_Job* job = MP_Lua_CheckJob(L, 1);
    switch (job->targetType) {
        case MP_JOB_TARGET_BLOCK:
            MP_Lua_PushBlock(L, (MP_Block*)job->target);
            break;
        case MP_JOB_TARGET_ROOM:
            MP_Lua_PushRoom(L, (MP_Room*)job->target);
            break;
        case MP_JOB_TARGET_UNIT:
            MP_Lua_PushUnit(L, (MP_Unit*)job->target);
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
    MP_Player player = MP_PLAYER_NONE;
    MP_Job job = {NULL, NULL, MP_JOB_TARGET_NONE, NULL, ZERO_VEC2};

    // Keep track at which table entry we are.
    int narg = 1;

    // Validate input.
    luaL_argcheck(L, lua_gettop(L) == 1 && lua_istable(L, -1), 0, "one 'table' expected");

    // Get job meta data.
    lua_getfield(L, -1, "name");
    job.type = MP_Lua_CheckJobType(L, -1);
    lua_pop(L, 1);

    // Now loop through the table. Push initial 'key' -- nil means start.
    lua_pushnil(L);
    // -> table, nil
    while (lua_next(L, -2)) {
        // -> table, key, value
        // Get key as string.
        const char* key;
        luaL_argcheck(L, lua_type(L, -2) == LUA_TSTRING, narg, "keys must be strings");
        key = lua_tostring(L, -2);

        // See what we have.
        if (strcmp(key, "name") == 0) {
            // Silently skip it.

        } else if (strcmp(key, "target") == 0) {
            if (MP_Lua_IsBlock(L, -1)) {
                job.targetType = MP_JOB_TARGET_BLOCK;
                job.target = MP_Lua_ToBlock(L, -1);
            } else if (MP_Lua_IsRoom(L, -1)) {
                job.targetType = MP_JOB_TARGET_ROOM;
                job.target = MP_Lua_ToRoom(L, -1);
            } else if (MP_Lua_IsUnit(L, -1)) {
                job.targetType = MP_JOB_TARGET_UNIT;
                job.target = MP_Lua_ToUnit(L, -1);
            } else {
                return luaL_argerror(L, narg, "invalid target type");
            }

        } else if (strcmp(key, "offset") == 0) {
            vec2 p = ZERO_VEC2;
            luaL_checktype(L, narg, LUA_TTABLE);
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
            job.offset = p;

        } else if (strcmp(key, "player") == 0) {
            player = MP_Lua_CheckPlayer(L, -1);

        } else {
            return luaL_argerror(L, narg, "unknown key");
        }

        lua_pop(L, 1);

        ++narg;
    }

    luaL_argcheck(L, job.target != NULL, 1, "must set 'target'");
    luaL_argcheck(L, player != MP_PLAYER_NONE, 1, "must set 'player'");

    // Allocate job and set values.
    *MP_NewJob(player, job.type) = job;

    return 0;
}

static int lua_DeleteJob(lua_State* L) {
    const MP_Player player = MP_Lua_CheckPlayer(L, 1);
    MP_DeleteJob(player, MP_Lua_CheckJob(L, 2));
    return 0;
}

static int lua_DeleteJobWhereTarget(lua_State* L) {
    // Get player.
    const MP_Player player = MP_Lua_CheckPlayer(L, 1);
    // Get job type to delete.
    const MP_JobType* meta = MP_Lua_CheckJobType(L, 2);
    // Get the target. Figure out what it is.
    if (MP_Lua_IsBlock(L, 3)) {
        // It's a block.
        MP_DeleteJobsTargetingBlock(player, meta, MP_Lua_ToBlock(L, 3));
    } else if (MP_Lua_IsRoom(L, 3)) {
        // It's a room.
        MP_DeleteJobsTargetingRoom(player, meta, MP_Lua_ToRoom(L, 3));
    } else if (MP_Lua_IsUnit(L, 3)) {
        // It's a unit.
        MP_DeleteJobsTargetingUnit(player, meta, MP_Lua_ToUnit(L, 3));
    } else {
        // Invalid target.
        luaL_argerror(L, 3, "expected '" LUA_BLOCKLIBNAME "', '" LUA_ROOMLIBNAME "' or '" LUA_UNITLIBNAME "'");
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
