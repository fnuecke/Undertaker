#include "block.h"
#include "script.h"
#include "type_impl.h"

// <editor-fold defaultstate="collapsed" desc="block parsers">
static void level(lua_State* L, MP_BlockType* type, bool forDefaults) {
    type->level = MP_Lua_CheckLevel(L, -1);
}

static void passability(lua_State* L, MP_BlockType* type, bool forDefaults) {
    type->passability = MP_Lua_CheckPassability(L, -1);
}

static void lightfrequency(lua_State* L, MP_BlockType* type, bool forDefaults) {
    type->lightFrequency = luaL_checkunsigned(L, -1);
}

static void durability(lua_State* L, MP_BlockType* type, bool forDefaults) {
    type->durability = luaL_checkunsigned(L, -1);
}

static void strength(lua_State* L, MP_BlockType* type, bool forDefaults) {
    type->strength = luaL_checkunsigned(L, -1);
}

static void gold(lua_State* L, MP_BlockType* type, bool forDefaults) {
    type->gold = luaL_checkunsigned(L, -1);
}

static void becomes(lua_State* L, MP_BlockType* type, bool forDefaults) {
    type->becomes = MP_Lua_CheckBlockType(L, -1);
}

static MP_BlockType* getBlockTarget(lua_State* L, MP_BlockType* type, bool forDefaults) {
    // What do we use for defaults?
    if (!forDefaults) {
        const MP_BlockType* existing;

        // Get the name.
        lua_getfield(L, -1, "name");
        // Check if that type is already known (override).
        if ((existing = MP_GetBlockTypeByName(luaL_checkstring(L, -1)))) {
            // Yes, this will be an override.
            *type = *existing;
        } else {
            // New entry.
            *type = *MP_GetBlockTypeDefaults();
            type->info.name = lua_tostring(L, -1);
        }
        lua_pop(L, 1); // pop name
    } // else: type already is a reference to the defaults.
    return type;
}

TYPE_PARSER(MP_BlockType, Block, getBlockTarget(L, type, forDefaults), MP_BlockType* type)
// </editor-fold>

static const BlockParserEntry blockParsers[] = {
    {"level", level},
    {"passability", passability},
    {"lightfrequency", lightfrequency},
    {"durability", durability},
    {"strength", strength},
    {"gold", gold},
    {"becomes", becomes},
    {NULL, NULL}
};

int MP_LuaCallback_SetBlockTypeDefaults(lua_State* L) {
    luaL_argcheck(L, lua_gettop(L) == 1, 1, "wrong number of arguments");

    // Build the block meta using the given properties.
    parseBlockTable(L, blockParsers, true, MP_GetBlockTypeDefaults());

    return 0;
}

int MP_LuaCallback_AddBlockType(lua_State* L) {
    MP_BlockType type;

    luaL_argcheck(L, lua_gettop(L) == 1, 1, "wrong number of arguments");

    // Build the block meta using the given properties.
    parseBlockTable(L, blockParsers, false, &type);

    // We require for at least the name to be set.
    luaL_argcheck(L, type.info.name != NULL && strlen(type.info.name) > 0, 1, "invalid or no 'name'");

    // All green, add the type.
    if (!MP_AddBlockType(&type)) {
        return luaL_argerror(L, 1, "bad block meta");
    }

    return 0;
}
