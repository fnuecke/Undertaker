#include "script.h"

#include "jobs.h"
#include "jobs_meta.h"

///////////////////////////////////////////////////////////////////////////////
// Getters
///////////////////////////////////////////////////////////////////////////////

static int lua_GetTargetBlock(lua_State* L) {
    luaMP_pushblock(L, luaMP_checkjob(L, 1)->block);
    return 1;
}

static int lua_GetTargetRoom(lua_State* L) {
    luaMP_pushroom(L, luaMP_checkjob(L, 1)->room);
    return 1;
}

static int lua_GetTargetUnit(lua_State* L) {
    luaMP_pushunit(L, luaMP_checkjob(L, 1)->unit);
    return 1;
}

///////////////////////////////////////////////////////////////////////////////
// Methods
///////////////////////////////////////////////////////////////////////////////

static int lua_CreateJob(lua_State* L) {
    MP_Player player = MP_PLAYER_NONE;
    const char* name = NULL;
    MP_Job job = {NULL, NULL, NULL, NULL, NULL, ZERO_VEC2};

    // Keep track at which table entry we are.
    int narg = 1;

    // Validate input.
    luaL_argcheck(L, lua_gettop(L) == 1 && lua_istable(L, -1), 0, "one 'table' expected");

    // Get the name.
    lua_getfield(L, -1, "name");
    // -> table, table["name"]
    luaL_argcheck(L, lua_type(L, -1) == LUA_TSTRING, narg, "no 'name' or not a string");
    name = lua_tostring(L, -1);
    lua_pop(L, 1);
    // -> table

    // Get job meta data.
    job.meta = MP_GetJobMetaByName(name);
    luaL_argcheck(L, job.meta != NULL, narg, "unknown job type");

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
            job.block = luaMP_checkblock(L, -1);

        } else if (strcmp(key, "room") == 0) {
            job.room = luaMP_checkroom(L, -1);

        } else if (strcmp(key, "unit") == 0) {
            job.unit = luaMP_checkunit(L, -1);

        } else if (strcmp(key, "offset") == 0) {
            // TODO

        } else if (strcmp(key, "player") == 0) {
            player = (MP_Player) luaL_checkunsigned(L, -1);
            luaL_argcheck(L, player < MP_PLAYER_COUNT, narg, "invalid 'player' value");

        } else {
            return luaL_argerror(L, narg, "unknown key");
        }

        lua_pop(L, 1);
    }

    luaL_argcheck(L, job.block != NULL || job.room != NULL || job.unit != NULL, 1, "must set at least one target");
    luaL_argcheck(L, player != MP_PLAYER_NONE, 1, "missing 'player'");

    // Allocate job and set values.
    *MP_NewJob(player, job.meta) = job;

    return 0;
}

static int lua_DeleteJob(lua_State* L) {
    MP_Player player = luaL_checkunsigned(L, 1);
    luaL_argcheck(L, player != MP_PLAYER_NONE && player < MP_PLAYER_COUNT, 1, "invalid 'player' value");
    MP_DeleteJob(player, luaMP_checkjob(L, 2));
    return 0;
}

static int lua_DeleteJobWhereTarget(lua_State* L) {
    // Get player.
    MP_Player player = luaL_checkunsigned(L, 1);
    luaL_argcheck(L, player != MP_PLAYER_NONE && player < MP_PLAYER_COUNT, 1, "invalid 'player' value");
    // Get the target. Figure out what it is.
    if (luaMP_isblock(L, 2)) {
        // It's a block.
        MP_DeleteJobsTargetingBlock(player, luaMP_toblock(L, 2));
    } else if (luaMP_isroom(L, 2)) {
        // It's a room.
        MP_DeleteJobsTargetingRoom(player, luaMP_toroom(L, 2));
    } else if (luaMP_isunit(L, 2)) {
        // It's a unit.
        MP_DeleteJobsTargetingUnit(player, luaMP_tounit(L, 2));
    } else {
        // Invalid target.
        luaL_argerror(L, 2, "expected '" LUA_BLOCKLIBNAME "', '" LUA_ROOMLIBNAME "' or '" LUA_UNITLIBNAME "'");
    }

    return 0;
}

///////////////////////////////////////////////////////////////////////////////
// Registration / Push / Check
///////////////////////////////////////////////////////////////////////////////

static const luaL_Reg lib[] = {
    {"getTargetBlock", lua_GetTargetBlock},
    {"getTargetRoom", lua_GetTargetRoom},
    {"getTargetUnit", lua_GetTargetUnit},

    {"create", lua_CreateJob},
    {"delete", lua_DeleteJob},
    {"deleteWhereTarget", lua_DeleteJobWhereTarget},
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

MP_Job* luaMP_checkjob(lua_State* L, int narg) {
    void* ud = luaL_checkudata(L, narg, LUA_JOBLIBNAME);
    luaL_argcheck(L, ud != NULL, 1, "'" LUA_JOBLIBNAME "' expected");
    return *(MP_Job**) ud;
}
