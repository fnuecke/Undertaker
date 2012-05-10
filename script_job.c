#include "script.h"

#include "jobs.h"
#include "jobs_meta.h"

///////////////////////////////////////////////////////////////////////////////
// Getters
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// Methods
///////////////////////////////////////////////////////////////////////////////

static int lua_CreateJob(lua_State* L) {
    MP_Job job = {NULL, NULL, NULL, NULL, NULL, {{0, 0}}};
    const char* name;
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

        } else {
            return luaL_argerror(L, narg, "unknown key");
        }

        lua_pop(L, 1);
    }

    return 0;
}

static int lua_DeleteJob(lua_State* L) {
    return 0;
}

///////////////////////////////////////////////////////////////////////////////
// Registration / Push / Check
///////////////////////////////////////////////////////////////////////////////

static const luaL_Reg lib[] = {
    {"create", lua_CreateJob},
    {"delete", lua_DeleteJob},
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

MP_Job* luaMP_checkjob(lua_State* L, int narg) {
    void* ud = luaL_checkudata(L, narg, LUA_JOBLIBNAME);
    luaL_argcheck(L, ud != NULL, 1, "'" LUA_JOBLIBNAME "' expected");
    return *(MP_Job**) ud;
}
