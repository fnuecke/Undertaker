#include "block.h"
#include "job.h"
#include "map.h"
#include "script.h"
#include "selection.h"
#include "unit.h"

///////////////////////////////////////////////////////////////////////////////
// Getters
///////////////////////////////////////////////////////////////////////////////

static int lua_GetDurability(lua_State* L) {
    const MP_Block* block = MP_Lua_CheckBlock(L, 1);

    lua_pushnumber(L, block->durability);

    return 1;
}

static int lua_GetGold(lua_State* L) {
    const MP_Block* block = MP_Lua_CheckBlock(L, 1);

    lua_pushinteger(L, block->gold);

    return 1;
}

static int lua_GetPlayer(lua_State* L) {
    const MP_Block* block = MP_Lua_CheckBlock(L, 1);

    lua_pushinteger(L, block->player);

    return 1;
}

static int lua_GetPassability(lua_State* L) {
    const MP_Block* block = MP_Lua_CheckBlock(L, 1);

    lua_pushinteger(L, block->type->passability);

    return 1;
}

static int lua_GetPosition(lua_State* L) {
    const MP_Block* block = MP_Lua_CheckBlock(L, 1);

    unsigned short x, y;
    MP_GetBlockCoordinates(block, &x, &y);
    lua_pushinteger(L, x);
    lua_pushinteger(L, y);

    return 2;
}

static int lua_GetRoom(lua_State* L) {
    const MP_Block* block = MP_Lua_CheckBlock(L, 1);

    MP_Lua_PushRoom(L, block->room);

    return 1;
}

static int lua_GetStrength(lua_State* L) {
    const MP_Block* block = MP_Lua_CheckBlock(L, 1);

    lua_pushnumber(L, block->strength);

    return 1;
}

static int lua_GetType(lua_State* L) {
    const MP_Block* block = MP_Lua_CheckBlock(L, 1);

    lua_pushinteger(L, block->type->info.id);

    return 1;
}

static int lua_IsPassable(lua_State* L) {
    const MP_Block* block = MP_Lua_CheckBlock(L, 1);

    lua_pushboolean(L, MP_IsBlockPassable(block));

    return 1;
}

static int lua_IsPassableBy(lua_State* L) {
    const MP_Block* block = MP_Lua_CheckBlock(L, 1);
    const MP_Unit* unit = MP_Lua_CheckUnit(L, 2);

    lua_pushboolean(L, MP_IsBlockPassableBy(block, unit->type));
    return 1;
}

static int lua_IsDestructible(lua_State* L) {
    const MP_Block* block = MP_Lua_CheckBlock(L, 1);

    lua_pushboolean(L, MP_IsBlockDestructible(block));

    return 1;
}

static int lua_IsConvertible(lua_State* L) {
    const MP_Block* block = MP_Lua_CheckBlock(L, 1);

    lua_pushboolean(L, MP_IsBlockConvertible(block));

    return 1;
}

static int lua_IsSelectable(lua_State* L) {
    const MP_Block* block = MP_Lua_CheckBlock(L, 1);
    const MP_Player player = MP_Lua_CheckPlayer(L, 2);

    if (player == MP_PLAYER_NONE) {
        return luaL_argerror(L, 2, "invalid player value");
    }

    lua_pushboolean(L, MP_IsBlockSelectable(block, player));

    return 1;
}

static int lua_IsSelectedBy(lua_State* L) {
    const MP_Block* block = MP_Lua_CheckBlock(L, 1);
    const MP_Player player = MP_Lua_CheckPlayer(L, 2);

    if (player == MP_PLAYER_NONE) {
        return luaL_argerror(L, 2, "invalid player value");
    }

    lua_pushboolean(L, MP_IsBlockSelected(block, player));

    return 1;
}

static int lua_IsTargetOfJobByType(lua_State* L) {
    const MP_Block* block = MP_Lua_CheckBlock(L, 1);
    const MP_Player player = MP_Lua_CheckPlayer(L, 2);
    const MP_JobType* meta = MP_Lua_CheckJobType(L, 3);
    unsigned int count;

    if (player == MP_PLAYER_NONE) {
        return luaL_argerror(L, 2, "invalid player value");
    } else {
        MP_JobList jobs = MP_GetJobs(meta, player, &count);
        for (unsigned int i = 0; i < count; ++i) {
            if (jobs[i]->targetType == MP_JOB_TARGET_BLOCK &&
                (const MP_Block*) jobs[i]->target == block) {
                lua_pushboolean(L, true);
                return 1;
            }
        }
        lua_pushboolean(L, false);
    }

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
    MP_Block* block = MP_Lua_CheckBlock(L, 1);
    const MP_Player player = MP_Lua_CheckPlayer(L, 2);
    const float strength = luaL_checknumber(L, 3);

    if (player == MP_PLAYER_NONE) {
        return luaL_argerror(L, 3, "invalid player value");
    }

    lua_pushboolean(L, MP_ConvertBlock(block, player, strength));

    return 1;
}

static int lua_DamageBlock(lua_State* L) {
    MP_Block* block = MP_Lua_CheckBlock(L, 1);
    const float damage = luaL_checknumber(L, 2);

    lua_pushboolean(L, MP_DamageBlock(block, damage));

    return 1;
}

///////////////////////////////////////////////////////////////////////////////
// Registration / Push / Check
///////////////////////////////////////////////////////////////////////////////

static const luaL_Reg lib[] = {
    {"getDurability", lua_GetDurability},
    {"getGold", lua_GetGold},
    {"getOwner", lua_GetPlayer},
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
