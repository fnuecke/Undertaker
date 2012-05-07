#include "map_loader.h"

#include <string.h>
#include <stdlib.h>

#include "lua/lua.h"
#include "lua/lauxlib.h"

#include "block.h"
#include "config.h"
#include "map.h"
#include "room_meta.h"
#include "textures.h"
#include "units_meta.h"

///////////////////////////////////////////////////////////////////////////////
// Loader state
///////////////////////////////////////////////////////////////////////////////

static const char* gMapFile = 0;
static bool isLoading = false;

static char* gPassabilityTypes[32] = {NULL};
static unsigned int gPassabilityTypeCount = 0;

static unsigned int gBlockMetaCount = 0;

static DK_BlockMeta gBlockMetaDefaults;

///////////////////////////////////////////////////////////////////////////////
// Helper methods
///////////////////////////////////////////////////////////////////////////////

static void clearPassability(void) {
    for (unsigned int i = 0; i < gPassabilityTypeCount; ++i) {
        free(gPassabilityTypes[i]);
        gPassabilityTypes[i] = NULL;
    }
    gPassabilityTypeCount = 0;
}

static void resetDefaults(void) {
    gBlockMetaDefaults.id = 0;
    gBlockMetaDefaults.name = NULL;
    gBlockMetaDefaults.level = DK_BLOCK_LEVEL_HIGH;
    gBlockMetaDefaults.passability = 0;
    gBlockMetaDefaults.durability = 0;
    gBlockMetaDefaults.strength = 0;
    gBlockMetaDefaults.gold = 0;
    gBlockMetaDefaults.becomes = NULL;
}

static DK_Passability getPassability(const char* name) {
    for (unsigned int i = 0; i < gPassabilityTypeCount; ++i) {
        if (strcmp(name, gPassabilityTypes[i]) == 0) {
            return 1 << i;
        }
    }
    return 0;
}

static int tableToBlock(lua_State* L, DK_BlockMeta* meta, bool forDefaults) {
    int narg = 1;

    // Now loop through the table.
    lua_pushnil(L);
    while (lua_next(L, 1)) {
        // Key is at -2, value at -1.
        luaL_argcheck(L, lua_type(L, -2) == LUA_TSTRING, narg, "keys must be strings");
        {
            // See what we have.
            const char* key = lua_tostring(L, -2);
            if (strcmp(key, "name") == 0) {
                if (!forDefaults) {
                    const char* name = luaL_checkstring(L, -1);
                    luaL_argcheck(L, strlen(name), narg, "'name' must not be empty");
                    meta->name = name;
                } else {
                    luaL_argerror(L, narg, "'name' not allowed in defaults");
                }

            } else if (strcmp(key, "level") == 0) {
                const char* level = luaL_checkstring(L, -1);
                if (strcmp(level, "pit") == 0) {
                    meta->level = DK_BLOCK_LEVEL_PIT;
                } else if (strcmp(level, "lowered") == 0) {
                    meta->level = DK_BLOCK_LEVEL_LOWERED;
                } else if (strcmp(level, "normal") == 0) {
                    meta->level = DK_BLOCK_LEVEL_NORMAL;
                } else if (strcmp(level, "high") == 0) {
                    meta->level = DK_BLOCK_LEVEL_HIGH;
                } else {
                    luaL_argcheck(L, level && strlen(level), narg, "'level': invalid value (valid: pit, lowered, normal, high)");
                    return luaL_argerror(L, narg, "'level': unknown value (valid: pit, lowered, normal, high)");
                }

            } else if (strcmp(key, "passability") == 0) {
                const DK_Passability value = getPassability(luaL_checkstring(L, -1));
                luaL_argcheck(L, value, narg, "unknown passability type");
                meta->passability = value;

            } else if (strcmp(key, "durability") == 0) {
                meta->durability = luaL_checkunsigned(L, -1);

            } else if (strcmp(key, "strength") == 0) {
                meta->strength = luaL_checkunsigned(L, -1);

            } else if (strcmp(key, "gold") == 0) {
                meta->gold = luaL_checkunsigned(L, -1);

            } else if (strcmp(key, "becomes") == 0) {
                const DK_BlockMeta* value = DK_GetBlockMetaByName(luaL_checkstring(L, -1));
                luaL_argcheck(L, value, narg, "unknown block type");
                meta->becomes = value;

            } else {
                return luaL_argerror(L, narg, "unknown key name (valid: name, level, passability, durability, strength, gold, becomes)");
            }
        }

        // Pop 'value', keep key to get next entry.
        lua_pop(L, 1);

        ++narg;
    }

    return 0;
}

///////////////////////////////////////////////////////////////////////////////
// Lua callbacks
///////////////////////////////////////////////////////////////////////////////

static int Lua_Ability(lua_State* L) {
    return 0;
}

static int Lua_Block(lua_State* L) {
    DK_BlockMeta meta = gBlockMetaDefaults;

    // Get number of arguments.
    const int n = lua_gettop(L);

    // Validate input.
    luaL_argcheck(L, n == 1 && lua_istable(L, 1), 0, "must specify one table");

    // Build the block meta using the given properties. Default the rest.
    tableToBlock(L, &meta, false);

    // We require for at least the name to be set.
    luaL_argcheck(L, meta.name, 1, "name is required but not set");

    DK_AddBlockMeta(&meta);

    fprintf(DK_log_target, "INFO: Registered block type '%s'.\n", meta.name);

    ++gBlockMetaCount;

    return 0;
}

static int Lua_BlockDefaults(lua_State* L) {
    // Get number of arguments.
    const int n = lua_gettop(L);

    // Validate input.
    luaL_argcheck(L, n == 1 && lua_istable(L, 1), 0, "must specify one table");

    tableToBlock(L, &gBlockMetaDefaults, true);

    return 0;
}

static int Lua_Job(lua_State* L) {
    return 0;
}

static int Lua_Passability(lua_State* L) {
    // Get number of arguments.
    const int n = lua_gettop(L);

    // We can only take so much...
    if (gPassabilityTypeCount >= 31) {
        luaL_error(L, "too many passability types (maximum of 32)");
    }

    // Validate input.
    luaL_argcheck(L, n == 1 && lua_isstring(L, 1), 0, "must specify one string");

    // Get the name and store it.
    if (!getPassability(luaL_checkstring(L, 1))) {
        const char* name = luaL_checkstring(L, 1);
        gPassabilityTypes[gPassabilityTypeCount] = calloc(strlen(name) + 1, sizeof (char));
        strcpy(gPassabilityTypes[gPassabilityTypeCount], name);
        ++gPassabilityTypeCount;

        fprintf(DK_log_target, "INFO: Registered passability type '%s'.\n", name);
    }
    return 0;
}

static int Lua_Room(lua_State* L) {
    return 0;
}

static int Lua_Unit(lua_State* L) {
    return 0;
}

static int Lua_Panic(lua_State* L) {
    fprintf(DK_log_target, "ERROR: fatal error parsing map: %s", luaL_checkstring(L, 0));
    exit(EXIT_FAILURE);
}

///////////////////////////////////////////////////////////////////////////////
// Map resize callback (triggers actual loading)
///////////////////////////////////////////////////////////////////////////////

static void Lua_Meta(const char* name) {
    lua_State* L = luaL_newstate();
    lua_atpanic(L, &Lua_Panic);

    DK_UnloadTextures();
    clearPassability();
    resetDefaults();

    lua_pushcfunction(L, Lua_Ability);
    lua_setglobal(L, "ability");

    lua_pushcfunction(L, Lua_Block);
    lua_setglobal(L, "block");

    lua_pushcfunction(L, Lua_BlockDefaults);
    lua_setglobal(L, "blockdefaults");

    lua_pushcfunction(L, Lua_Job);
    lua_setglobal(L, "job");

    lua_pushcfunction(L, Lua_Passability);
    lua_setglobal(L, "passability");

    lua_pushcfunction(L, Lua_Room);
    lua_setglobal(L, "room");

    lua_pushcfunction(L, Lua_Unit);
    lua_setglobal(L, "unit");

    if (luaL_dofile(L, name) != LUA_OK) {
        fprintf(DK_log_target, "ERROR: Failed parsing map file: %s\n", lua_tostring(L, -1));
    }

    lua_close(L);

    fflush(DK_log_target);

    DK_GL_GenerateTextures();
}

static void onMapSizeChange(void) {
    if (isLoading) {
        Lua_Meta(gMapFile);
    }
}

///////////////////////////////////////////////////////////////////////////////
// Save / Load methods
///////////////////////////////////////////////////////////////////////////////

void DK_LoadMap(const char* filename) {
    if (!filename) {
        return;
    }

    fprintf(DK_log_target, "INFO: Loading map file '%s'.\n", filename);

    gMapFile = filename;
    isLoading = true;
    DK_SetMapSize(128);
    isLoading = false;

    // Load a test map.
    for (unsigned int i = 0; i < 7; ++i) {
        for (unsigned int j = 0; j < 7; ++j) {
            if (i <= 1 || j <= 1) {
                //DK_SetBlockOwner(DK_GetBlockAt(4 + i, 5 + j), DK_PLAYER_ONE);
            }
            if (i > 0 && j > 0 && i < 6 && j < 6) {
                //DK_SetBlockType(DK_GetBlockAt(4 + i, 5 + j), DK_BLOCK_NONE);
            }
        }
    }

    //DK_SetBlockType(DK_GetBlockAt(7, 8), DK_BLOCK_DIRT);
    //DK_SetBlockType(DK_GetBlockAt(8, 8), DK_BLOCK_DIRT);

    //DK_SetBlockType(DK_GetBlockAt(10, 8), DK_BLOCK_WATER);
    //DK_SetBlockType(DK_GetBlockAt(11, 8), DK_BLOCK_WATER);
    //DK_SetBlockType(DK_GetBlockAt(11, 9), DK_BLOCK_WATER);
    //DK_block_at(9, 8)->owner = DK_PLAYER_RED;

    for (unsigned int i = 0; i < 2; ++i) {

        //DK_AddUnit(DK_PLAYER_ONE, DK_UNIT_IMP, 5, 10);
    }
}

void DK_SaveMap(const char* filename) {

}

void DK_InitMapLoader(void) {
    DK_OnMapSizeChange(onMapSizeChange);
}