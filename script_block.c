#include "script.h"

#include "block.h"
#include "job.h"
#include "map.h"
#include "selection.h"
#include "unit.h"

///////////////////////////////////////////////////////////////////////////////
// Getters
///////////////////////////////////////////////////////////////////////////////

static int lua_GetDurability(lua_State* L) {
    lua_pushnumber(L, luaMP_checkblock(L, 1)->durability);
    return 1;
}

static int lua_GetGold(lua_State* L) {
    lua_pushinteger(L, luaMP_checkblock(L, 1)->gold);
    return 1;
}

static int lua_GetOwner(lua_State* L) {
    lua_pushinteger(L, luaMP_checkblock(L, 1)->owner);
    return 1;
}

static int lua_GetPassability(lua_State* L) {
    lua_pushinteger(L, luaMP_checkblock(L, 1)->meta->passability);
    return 1;
}

static int lua_GetPosition(lua_State* L) {
    unsigned short x, y;
    if (!MP_GetBlockCoordinates(&x, &y, luaMP_checkblock(L, 1))) {
        return luaL_error(L, "invalid 'Block'");
    }
    lua_pushinteger(L, x);
    lua_pushinteger(L, y);
    return 2;
}

static int lua_GetRoom(lua_State* L) {
    luaMP_pushroom(L, luaMP_checkblock(L, 1)->room);
    return 1;
}

static int lua_GetStrength(lua_State* L) {
    lua_pushnumber(L, luaMP_checkblock(L, 1)->strength);
    return 1;
}

static int lua_GetType(lua_State* L) {
    lua_pushinteger(L, luaMP_checkblock(L, 1)->meta->id);
    return 1;
}

static int lua_IsPassable(lua_State* L) {
    lua_pushboolean(L, MP_IsBlockPassable(luaMP_checkblock(L, 1)));
    return 1;
}

static int lua_IsPassableBy(lua_State* L) {
    lua_pushboolean(L, MP_IsBlockPassableBy(luaMP_checkblock(L, 1),
                                            luaMP_checkunit(L, 2)->meta));
    return 1;
}

static int lua_IsDestructible(lua_State* L) {
    lua_pushboolean(L, MP_IsBlockDestructible(luaMP_checkblock(L, 1)));
    return 1;
}

static int lua_IsConvertible(lua_State* L) {
    lua_pushboolean(L, MP_IsBlockConvertible(luaMP_checkblock(L, 1)));
    return 1;
}

static int lua_IsSelectable(lua_State* L) {
    lua_pushboolean(L, MP_IsBlockSelectable(luaMP_checkplayer(L, 2),
                                            luaMP_checkblock(L, 1)));
    return 1;
}

static int lua_IsSelectedBy(lua_State* L) {
    const MP_Block* block = luaMP_checkblock(L, 1);
    const MP_Player player = luaMP_checkplayer(L, 2);
    unsigned short x, y;
    if (!MP_GetBlockCoordinates(&x, &y, block)) {
        lua_pushboolean(L, false);
    } else {
        lua_pushboolean(L, MP_IsBlockSelected(player, x, y));
    }
    return 1;
}

static int lua_IsTargetOfJobByType(lua_State* L) {
    const MP_Block* block = luaMP_checkblock(L, 1);
    const MP_Player player = luaMP_checkplayer(L, 2);
    const MP_JobMeta* meta = luaMP_checkjobmeta(L, 3);
    unsigned int count;
    MP_Job * const* jobs = MP_GetJobs(player, meta, &count);
    for (unsigned int i = 0; i < count; ++i) {
        if (jobs[i]->block == block) {
            lua_pushboolean(L, true);
            return 1;
        }
    }
    lua_pushboolean(L, false);
    return 1;
}

///////////////////////////////////////////////////////////////////////////////
// Methods
///////////////////////////////////////////////////////////////////////////////

static int lua_BlockAt(lua_State* L) {
    luaMP_pushblock(L, MP_GetBlockAt(luaL_checkint(L, 1), luaL_checkint(L, 2)));
    return 1;
}

static int lua_ConvertBlock(lua_State* L) {
    lua_pushboolean(L, MP_ConvertBlock(luaMP_checkblock(L, 1),
                                       luaL_checkunsigned(L, 2),
                                       luaL_checknumber(L, 3)));
    return 1;
}

static int lua_DamageBlock(lua_State* L) {
    lua_pushboolean(L, MP_DamageBlock(luaMP_checkblock(L, 1),
                                      luaL_checknumber(L, 2)));
    return 1;
}

///////////////////////////////////////////////////////////////////////////////
// Registration / Push / Check
///////////////////////////////////////////////////////////////////////////////

static const luaL_Reg lib[] = {
    {"getDurability", lua_GetDurability},
    {"getGold", lua_GetGold},
    {"getOwner", lua_GetOwner},
    {"getPassability", lua_GetPassability},
    {"getPosition", lua_GetPosition},
    {"getRoom", lua_GetRoom},
    {"getStrength", lua_GetStrength},
    {"getType", lua_GetType},
    {"isPassable", lua_IsPassable},
    {"isPassableBy", lua_IsPassableBy},
    {"isDestructible", lua_IsDestructible},
    {"isConvertible", lua_IsConvertible},
    {"isSelectable", lua_IsSelectable},
    {"isSelectedBy", lua_IsSelectedBy},
    {"isTargetOfJobByType", lua_IsTargetOfJobByType},

    {"at", lua_BlockAt},
    {"convert", lua_ConvertBlock},
    {"damage", lua_DamageBlock},
    {NULL, NULL}
};

int luaopen_block(lua_State* L) {
    luaL_newlib(L, lib);
    return 1;
}

void luaMP_pushblock(lua_State* L, MP_Block* block) {
    MP_Block** ud = (MP_Block**) lua_newuserdata(L, sizeof (MP_Block*));
    *ud = block;
    luaL_setmetatable(L, LUA_BLOCKLIBNAME);
}

bool luaMP_isblock(lua_State* L, int narg) {
    return luaL_checkudata(L, narg, LUA_BLOCKLIBNAME) != NULL;
}

MP_Block* luaMP_toblock(lua_State* L, int narg) {
    return *(MP_Block**) lua_touserdata(L, narg);
}

MP_Block* luaMP_checkblock(lua_State* L, int narg) {
    void* ud = luaL_checkudata(L, narg, LUA_BLOCKLIBNAME);
    luaL_argcheck(L, ud != NULL, narg, "'" LUA_BLOCKLIBNAME "' expected");
    return *(MP_Block**) ud;
}
