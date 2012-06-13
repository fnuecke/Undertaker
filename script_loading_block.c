#include "block.h"

#include "meta_impl.h"

static bool gForDefaults;

// <editor-fold defaultstate="collapsed" desc="block parsers">
static void level(lua_State* L, MP_BlockMeta* meta) {
    meta->level = luaMP_checklevel(L, -1);
}

static void passability(lua_State* L, MP_BlockMeta* meta) {
    meta->passability = luaMP_checkpassability(L, -1);
}

static void lightfrequency(lua_State* L, MP_BlockMeta* meta) {
    meta->lightFrequency = luaL_checkunsigned(L, -1);
}

static void durability(lua_State* L, MP_BlockMeta* meta) {
    meta->durability = luaL_checkunsigned(L, -1);
}

static void strength(lua_State* L, MP_BlockMeta* meta) {
    meta->strength = luaL_checkunsigned(L, -1);
}

static void gold(lua_State* L, MP_BlockMeta* meta) {
    meta->gold = luaL_checkunsigned(L, -1);
}

static void becomes(lua_State* L, MP_BlockMeta* meta) {
    meta->becomes = luaMP_checkblockmeta(L, -1);
}

META_parser(MP_BlockMeta, Block, {
    // What do we use for defaults?
    if (!gForDefaults) {
        const MP_BlockMeta* existing;

        // Get the name.
        lua_getfield(L, -1, "name");
        // Check if that type is already known (override).
        if ((existing = MP_GetBlockMetaByName(luaL_checkstring(L, -1)))) {
            // Yes, this will be an override.
            *meta = *existing;
        } else {
            // New entry.
            *meta = gMetaDefaults;
            meta->name =  lua_tostring(L, -1);
        }
        lua_pop(L, 1); // pop name
    } // else meta already equals gMetaDefaults
    target = meta;
}, gForDefaults, MP_BlockMeta* meta)
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

int MP_Lua_BlockMetaDefaults(lua_State* L) {
    luaL_argcheck(L, lua_gettop(L) == 1, 1, "wrong number of arguments");

    // Build the block meta using the given properties.
    gForDefaults = true;
    parseBlockTable(L, blockParsers, MP_GetBlockMetaDefaults());

    return 0;
}

int MP_Lua_AddBlockMeta(lua_State* L) {
    // New type, start with defaults.
    MP_BlockMeta meta = *MP_GetBlockMetaDefaults();

    luaL_argcheck(L, lua_gettop(L) == 1, 1, "wrong number of arguments");

    // Build the block meta using the given properties.
    gForDefaults = false;
    parseBlockTable(L, blockParsers, &meta);

    // We require for at least the name to be set.
    luaL_argcheck(L, meta.name != NULL && strlen(meta.name) > 0, 1, "invalid or no 'name'");

    // All green, add the type.
    if (!MP_AddBlockMeta(&meta)) {
        return luaL_argerror(L, 1, "bad block meta");
    }

    return 0;
}
