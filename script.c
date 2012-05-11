#include "script.h"

#include <string.h>

#include "block_meta.h"
#include "jobs_meta.h"
#include "passability.h"
#include "room_meta.h"
#include "units_meta.h"

static lua_State *getthread(lua_State *L, int *arg) {
    if (lua_isthread(L, 1)) {
        *arg = 1;
        return lua_tothread(L, 1);
    } else {
        *arg = 0;
        return L;
    }
}

static int traceback(lua_State *L) {
    int arg;
    lua_State *L1 = getthread(L, &arg);
    const char *msg = lua_tostring(L, arg + 1);
    if (msg == NULL && !lua_isnoneornil(L, arg + 1)) /* non-string 'msg'? */
        lua_pushvalue(L, arg + 1); /* return it untouched */
    else {
        int level = luaL_optint(L, arg + 2, (L == L1) ? 1 : 0);
        luaL_traceback(L, L1, msg, level);
    }
    return 1;
}

int MP_Lua_pcall(lua_State* L, int nargs, int nresults) {
    int result;

    // Insert traceback method before actually called function.
    lua_pushcfunction(L, traceback);
    lua_insert(L, -(nargs + 2));

    // Run and check result to know where to remove traceback method.
    if ((result = lua_pcall(L, nargs, nresults, -(nargs + 2))) == LUA_OK) {
        // Success, there's nresult entries on the stack.
        lua_remove(L, -(nresults + 1));
    } else {
        // Error, means there's one string on the stack.
        lua_remove(L, -2);
    }
    return result;
}

MP_Passability luaMP_checkpassability(lua_State* L, int narg, int errarg) {
    const MP_Passability passability = MP_GetPassability(luaL_checkstring(L, narg));
    luaL_argcheck(L, passability != MP_PASSABILITY_NONE, errarg, "invalid 'passability' value");
    return passability;
}

MP_Player luaMP_checkplayer(lua_State* L, int narg, int errarg) {
    const MP_Player player = luaL_checkunsigned(L, narg);
    luaL_argcheck(L, player != MP_PLAYER_NONE && player < MP_PLAYER_COUNT, errarg, "invalid 'player' value");
    return player;
}

MP_BlockLevel luaMP_checklevel(lua_State* L, int narg, int errarg) {
    const char* level = luaL_checkstring(L, narg);
    if (strcmp(level, "pit") == 0) {
        return MP_BLOCK_LEVEL_PIT;
    } else if (strcmp(level, "lowered") == 0) {
        return MP_BLOCK_LEVEL_LOWERED;
    } else if (strcmp(level, "normal") == 0) {
        return MP_BLOCK_LEVEL_NORMAL;
    } else if (strcmp(level, "high") == 0) {
        return MP_BLOCK_LEVEL_HIGH;
    }

    return luaL_argerror(L, errarg, "invalid 'level' value");
}

const MP_BlockMeta* luaMP_checkblockmeta(lua_State* L, int narg, int errarg) {
    const MP_BlockMeta* meta = MP_GetBlockMetaByName(luaL_checkstring(L, narg));
    luaL_argcheck(L, meta != NULL, errarg, "invalid block type");
    return meta;
}

const MP_JobMeta* luaMP_checkjobmeta(lua_State* L, int narg, int errarg) {
    const MP_JobMeta* meta = MP_GetJobMetaByName(luaL_checkstring(L, narg));
    luaL_argcheck(L, meta != NULL, errarg, "invalid job type");
    return meta;
}

const MP_RoomMeta* luaMP_checkroommeta(lua_State* L, int narg, int errarg) {
    const MP_RoomMeta* meta = MP_GetRoomMetaByName(luaL_checkstring(L, narg));
    luaL_argcheck(L, meta != NULL, errarg, "invalid room type");
    return meta;
}

const MP_UnitMeta* luaMP_checkunitmeta(lua_State* L, int narg, int errarg) {
    const MP_UnitMeta* meta = MP_GetUnitMetaByName(luaL_checkstring(L, narg));
    luaL_argcheck(L, meta != NULL, errarg, "invalid unit type");
    return meta;
}
