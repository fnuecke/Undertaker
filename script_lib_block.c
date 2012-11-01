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
    lua_pushnumber(L, MP_Lua_CheckBlock(L, 1)->durability);
    return 1;
}

static int lua_GetGold(lua_State* L) {
    lua_pushinteger(L, MP_Lua_CheckBlock(L, 1)->gold);
    return 1;
}

static int lua_GetOwner(lua_State* L) {
    lua_pushinteger(L, MP_Lua_CheckBlock(L, 1)->owner);
    return 1;
}

static int lua_GetPassability(lua_State* L) {
    lua_pushinteger(L, MP_Lua_CheckBlock(L, 1)->type->passability);
    return 1;
}

static int lua_GetPosition(lua_State* L) {
    unsigned short x, y;
    if (!MP_GetBlockCoordinates(&x, &y, MP_Lua_CheckBlock(L, 1))) {
        return luaL_error(L, "invalid 'Block'");
    }
    lua_pushinteger(L, x);
    lua_pushinteger(L, y);
    return 2;
}

static int lua_GetRoom(lua_State* L) {
    MP_Lua_PushRoom(L, MP_Lua_CheckBlock(L, 1)->room);
    return 1;
}

static int lua_GetStrength(lua_State* L) {
    lua_pushnumber(L, MP_Lua_CheckBlock(L, 1)->strength);
    return 1;
}

static int lua_GetType(lua_State* L) {
    lua_pushinteger(L, MP_Lua_CheckBlock(L, 1)->type->info.id);
    return 1;
}

static int lua_IsPassable(lua_State* L) {
    lua_pushboolean(L, MP_IsBlockPassable(MP_Lua_CheckBlock(L, 1)));
    return 1;
}

static int lua_IsPassableBy(lua_State* L) {
    lua_pushboolean(L, MP_IsBlockPassableBy(MP_Lua_CheckBlock(L, 1),
                                            MP_Lua_CheckUnit(L, 2)->type));
    return 1;
}

static int lua_IsDestructible(lua_State* L) {
    lua_pushboolean(L, MP_IsBlockDestructible(MP_Lua_CheckBlock(L, 1)));
    return 1;
}

static int lua_IsConvertible(lua_State* L) {
    lua_pushboolean(L, MP_IsBlockConvertible(MP_Lua_CheckBlock(L, 1)));
    return 1;
}

static int lua_IsSelectable(lua_State* L) {
    lua_pushboolean(L, MP_IsBlockSelectable(MP_Lua_CheckPlayer(L, 2),
                                            MP_Lua_CheckBlock(L, 1)));
    return 1;
}

static int lua_IsSelectedBy(lua_State* L) {
    const MP_Block* block = MP_Lua_CheckBlock(L, 1);
    const MP_Player player = MP_Lua_CheckPlayer(L, 2);
    unsigned short x, y;
    if (!MP_GetBlockCoordinates(&x, &y, block)) {
        lua_pushboolean(L, false);
    } else {
        lua_pushboolean(L, MP_IsBlockSelected(player, x, y));
    }
    return 1;
}

static int lua_IsTargetOfJobByType(lua_State* L) {
    const MP_Block* block = MP_Lua_CheckBlock(L, 1);
    const MP_Player player = MP_Lua_CheckPlayer(L, 2);
    const MP_JobType* meta = MP_Lua_CheckJobType(L, 3);
    unsigned int count;
    MP_Job* const* jobs = MP_GetJobs(player, meta, &count);
    for (unsigned int i = 0; i < count; ++i) {
        if (jobs[i]->targetType == MP_JOB_TARGET_BLOCK &&
            (const MP_Block*)jobs[i]->target == block)
        {
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
    MP_Lua_PushBlock(L, MP_GetBlockAt(luaL_checkint(L, 1), luaL_checkint(L, 2)));
    return 1;
}

static int lua_ConvertBlock(lua_State* L) {
    lua_pushboolean(L, MP_ConvertBlock(MP_Lua_CheckBlock(L, 1),
                                       luaL_checkunsigned(L, 2),
                                       luaL_checknumber(L, 3)));
    return 1;
}

static int lua_DamageBlock(lua_State* L) {
    lua_pushboolean(L, MP_DamageBlock(MP_Lua_CheckBlock(L, 1),
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

int MP_Lua_OpenBlock(lua_State* L) {
    luaL_newlib(L, lib);
    return 1;
}
