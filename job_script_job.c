#include "job_script.h"

#include "job.h"
#include "meta_job.h"

///////////////////////////////////////////////////////////////////////////////
// Getters
///////////////////////////////////////////////////////////////////////////////

typedef struct JobIter {
    MP_Job * const* jobs;
    unsigned int i;
} JobIter;

static int lua_JobIter(lua_State* L) {
    JobIter* iter = (JobIter*) lua_touserdata(L, lua_upvalueindex(1));

    // We iterate back to front, to allow deletion of current entry.
    if (iter->i > 0) {
        luaMP_pushjob(L, iter->jobs[--iter->i]);
        return 1;
    }

    return 0;
}

static int lua_GetByType(lua_State* L) {
    const MP_Player player = luaMP_checkplayer(L, 1, 1);
    const MP_JobMeta* meta = luaMP_checkjobmeta(L, 2, 2);

    JobIter* iter = (JobIter*) lua_newuserdata(L, sizeof (JobIter));
    iter->jobs = MP_GetJobs(player, meta, &iter->i);

    lua_pushcclosure(L, lua_JobIter, 1);
    return 1;
}

static int lua_GetOffset(lua_State* L) {
    MP_Job* job = luaMP_checkjob(L, 1, 1);
    lua_pushnumber(L, job->offset.d.x);
    lua_pushnumber(L, job->offset.d.y);
    return 2;
}

static int lua_GetPosition(lua_State* L) {
    MP_Job* job = luaMP_checkjob(L, 1, 1);
    vec2 p;
    MP_GetJobPosition(&p, job);
    lua_pushnumber(L, p.d.x);
    lua_pushnumber(L, p.d.y);
    return 2;
}

static int lua_GetTargetBlock(lua_State* L) {
    luaMP_pushblock(L, luaMP_checkjob(L, 1, 1)->block);
    return 1;
}

static int lua_GetTargetRoom(lua_State* L) {
    luaMP_pushroom(L, luaMP_checkjob(L, 1, 1)->room);
    return 1;
}

static int lua_GetTargetUnit(lua_State* L) {
    luaMP_pushunit(L, luaMP_checkjob(L, 1, 1)->unit);
    return 1;
}

///////////////////////////////////////////////////////////////////////////////
// Methods
///////////////////////////////////////////////////////////////////////////////

static int lua_CreateJob(lua_State* L) {
    MP_Player player = MP_PLAYER_NONE;
    MP_Job job = {NULL, NULL, NULL, NULL, NULL, ZERO_VEC2};

    // Keep track at which table entry we are.
    int narg = 1;

    // Validate input.
    luaL_argcheck(L, lua_gettop(L) == 1 && lua_istable(L, -1), 0, "one 'table' expected");

    // Get job meta data.
    lua_getfield(L, -1, "name");
    job.meta = luaMP_checkjobmeta(L, -1, 1);
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

        } else if (strcmp(key, "block") == 0) {
            job.block = luaMP_checkblock(L, -1, narg);

        } else if (strcmp(key, "room") == 0) {
            job.room = luaMP_checkroom(L, -1, narg);

        } else if (strcmp(key, "unit") == 0) {
            job.unit = luaMP_checkunit(L, -1, narg);

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
            player = luaMP_checkplayer(L, -1, narg);

        } else {
            return luaL_argerror(L, narg, "unknown key");
        }

        lua_pop(L, 1);
    }

    luaL_argcheck(L, job.block != NULL || job.room != NULL || job.unit != NULL, 1, "must set at least one target");
    luaL_argcheck(L, player != MP_PLAYER_NONE, 1, "must set 'player'");

    // Allocate job and set values.
    *MP_NewJob(player, job.meta) = job;

    return 0;
}

static int lua_DeleteJob(lua_State* L) {
    const MP_Player player = luaMP_checkplayer(L, 1, 1);
    MP_DeleteJob(player, luaMP_checkjob(L, 2, 2));
    return 0;
}

static int lua_DeleteJobWhereTarget(lua_State* L) {
    // Get player.
    const MP_Player player = luaMP_checkplayer(L, 1, 1);
    // Get job type to delete.
    const MP_JobMeta* meta = luaMP_checkjobmeta(L, 2, 2);
    // Get the target. Figure out what it is.
    if (luaMP_isblock(L, 3)) {
        // It's a block.
        MP_DeleteJobsTargetingBlock(player, meta, luaMP_toblock(L, 3));
    } else if (luaMP_isroom(L, 3)) {
        // It's a room.
        MP_DeleteJobsTargetingRoom(player, meta, luaMP_toroom(L, 3));
    } else if (luaMP_isunit(L, 3)) {
        // It's a unit.
        MP_DeleteJobsTargetingUnit(player, meta, luaMP_tounit(L, 3));
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
    {"getTargetBlock", lua_GetTargetBlock},
    {"getTargetRoom", lua_GetTargetRoom},
    {"getTargetUnit", lua_GetTargetUnit},

    {"create", lua_CreateJob},
    {"delete", lua_DeleteJob},
    {"deleteByTypeWhereTarget", lua_DeleteJobWhereTarget},
    {NULL, NULL}
};

int luaopen_job(lua_State* L) {
    luaL_newlib(L, lib);
    return 1;
}

void luaMP_pushjob(lua_State* L, MP_Job* job) {
    MP_Job** ud = (MP_Job**) lua_newuserdata(L, sizeof (MP_Job*));
    *ud = job;
    luaL_setmetatable(L, LUA_JOBLIBNAME);
}

bool luaMP_isjob(lua_State* L, int narg) {
    return luaL_checkudata(L, narg, LUA_JOBLIBNAME) != NULL;
}

MP_Job* luaMP_tojob(lua_State* L, int narg) {
    return *(MP_Job**) lua_touserdata(L, narg);
}

MP_Job* luaMP_checkjob(lua_State* L, int narg, int errarg) {
    void* ud = luaL_checkudata(L, narg, LUA_JOBLIBNAME);
    luaL_argcheck(L, ud != NULL, errarg, "'" LUA_JOBLIBNAME "' expected");
    return *(MP_Job**) ud;
}
