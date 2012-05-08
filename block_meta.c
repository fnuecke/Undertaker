#include "block_meta.h"

#include <stdio.h>

#include "passability.h"
#include "textures.h"

/** File name suffixes for textures */
const char* BLOCK_LEVEL_SUFFIX[] = {
    [MP_BLOCK_LEVEL_PIT] = "pit",
    [MP_BLOCK_LEVEL_LOWERED] = "lowered",
    [MP_BLOCK_LEVEL_NORMAL] = "normal",
    [MP_BLOCK_LEVEL_HIGH] = "high"
};

const char* BLOCK_TEXTURE_SUFFIX_TOP[] = {
    [MP_BLOCK_TEXTURE_TOP] = "top",
    [MP_BLOCK_TEXTURE_TOP_OWNED_OVERLAY] = "top_o",
    [MP_BLOCK_TEXTURE_TOP_N] = "top_on",
    [MP_BLOCK_TEXTURE_TOP_NE] = "top_one",
    [MP_BLOCK_TEXTURE_TOP_NS] = "top_ons",
    [MP_BLOCK_TEXTURE_TOP_NES] = "top_ones",
    [MP_BLOCK_TEXTURE_TOP_NESW] = "top_onesw",
    [MP_BLOCK_TEXTURE_TOP_NE_CORNER] = "top_onec",
    [MP_BLOCK_TEXTURE_TOP_NES_CORNER] = "top_onesc",
    [MP_BLOCK_TEXTURE_TOP_NESW_CORNER] = "top_oneswc",
    [MP_BLOCK_TEXTURE_TOP_NESWN_CORNER] = "top_oneswnc"
};

const char* BLOCK_TEXTURE_SUFFIX_SIDE[] = {
    [MP_BLOCK_TEXTURE_SIDE] = "side",
    [MP_BLOCK_TEXTURE_SIDE_OWNED_OVERLAY] = "side_o"
};

META_globals(MP_BlockMeta)

///////////////////////////////////////////////////////////////////////////////
// Helper methods
///////////////////////////////////////////////////////////////////////////////

/** Lua table parsing */
static int tableToBlock(lua_State* L, MP_BlockMeta* meta, bool forDefaults) {
    // Keep track at which table entry we are.
    int narg = 1;

    // Now loop through the table. Push initial 'key' -- nil means start.
    lua_pushnil(L); // -> -2=table, -1=key
    while (lua_next(L, 1)) { // -> -3=table, -2=key, -1=value
        luaL_argcheck(L, lua_type(L, -2) == LUA_TSTRING, narg, "keys must be strings");
        {
            // We can be sure it's a string at this point.
            const char* key = lua_tostring(L, -2);

            // See what we have.
            if (strcmp(key, "name") == 0) {
                if (!forDefaults) {
                    const char* name = luaL_checkstring(L, -1);
                    luaL_argcheck(L, name && strlen(name), narg, "'name' must not be empty");
                    meta->name = name;
                } else {
                    return luaL_argerror(L, narg, "'name' not allowed in defaults");
                }

            } else if (strcmp(key, "level") == 0) {
                const char* level = luaL_checkstring(L, -1);
                luaL_argcheck(L, level && strlen(level), narg, "'level' must not be empty");
                if (strcmp(level, "pit") == 0) {
                    meta->level = MP_BLOCK_LEVEL_PIT;
                } else if (strcmp(level, "lowered") == 0) {
                    meta->level = MP_BLOCK_LEVEL_LOWERED;
                } else if (strcmp(level, "normal") == 0) {
                    meta->level = MP_BLOCK_LEVEL_NORMAL;
                } else if (strcmp(level, "high") == 0) {
                    meta->level = MP_BLOCK_LEVEL_HIGH;
                } else {
                    return luaL_argerror(L, narg, "unknown 'level' value");
                }

            } else if (strcmp(key, "passability") == 0) {
                const MP_Passability value = MP_GetPassability(luaL_checkstring(L, -1));
                luaL_argcheck(L, value, narg, "unknown 'passability' value");
                meta->passability = value;

            } else if (strcmp(key, "durability") == 0) {
                meta->durability = luaL_checkunsigned(L, -1);

            } else if (strcmp(key, "strength") == 0) {
                meta->strength = luaL_checkunsigned(L, -1);

            } else if (strcmp(key, "gold") == 0) {
                meta->gold = luaL_checkunsigned(L, -1);

            } else if (strcmp(key, "becomes") == 0) {
                const MP_BlockMeta* value = MP_GetBlockMetaByName(luaL_checkstring(L, -1));
                luaL_argcheck(L, value, narg, "unknown 'becomes' value");
                meta->becomes = value;

            } else {
                return luaL_argerror(L, narg, "unknown key");
            }
        }

        // Pop 'value', keep key to get next entry.
        lua_pop(L, 1); // -> -2=table, -1=key

        ++narg;
    } // -1=table

    return 0;
}

/** Texture loading for a single type */
static void loadTexturesFor(MP_BlockMeta* m) {
    char basename[128];
    MP_TextureID texturesTop[MP_BLOCK_TEXTURE_TOP_COUNT];
    MP_TextureID texturesSide[MP_BLOCK_LEVEL_COUNT][MP_BLOCK_TEXTURE_SIDE_COUNT];

    // Clear.
    memset(texturesTop, 0, MP_BLOCK_TEXTURE_TOP_COUNT * sizeof (MP_TextureID));
    memset(texturesSide, 0, MP_BLOCK_LEVEL_COUNT * MP_BLOCK_TEXTURE_SIDE_COUNT * sizeof (MP_TextureID));
    memset(m->texturesTop, 0, MP_BLOCK_TEXTURE_TOP_COUNT * sizeof (MP_TextureID));
    memset(m->texturesSide, 0, MP_BLOCK_LEVEL_COUNT * MP_BLOCK_TEXTURE_SIDE_COUNT * sizeof (MP_TextureID));

    // Load textures.
    for (unsigned int texture = 0, max = m->strength ? MP_BLOCK_TEXTURE_TOP_COUNT : 1; texture < max; ++texture) {
        // Build file name.
        snprintf(basename, sizeof (basename), "%s_%s", m->name, BLOCK_TEXTURE_SUFFIX_TOP[texture]);

        // Try to have it loaded.
        m->texturesTop[texture] = MP_LoadTexture(basename);

        // Remember the first existing type of a texture to fill up later.
        if (!texturesTop[texture]) {
            texturesTop[texture] = m->texturesTop[texture];
        }
    }
    for (unsigned int level = 0; level <= m->level; ++level) {
        for (unsigned int texture = 0, max = m->strength ? MP_BLOCK_TEXTURE_SIDE_COUNT : 1; texture < max; ++texture) {
            // Build file name.
            snprintf(basename, sizeof (basename), "%s_%s_%s", m->name, BLOCK_LEVEL_SUFFIX[level], BLOCK_TEXTURE_SUFFIX_SIDE[texture]);

            // Try to have it loaded.
            m->texturesSide[level][texture] = MP_LoadTexture(basename);

            // Remember the first existing type of a texture to fill up later.
            if (!texturesSide[level][texture]) {
                texturesSide[level][texture] = m->texturesSide[level][texture];
            }
        }
    }

    // Fill up gaps with the first texture we have at any level.
    for (unsigned int texture = 0; texture < MP_BLOCK_TEXTURE_TOP_COUNT; ++texture) {
        if (!m->texturesTop[texture]) {
            m->texturesTop[texture] = texturesTop[texture];
        }
    }
    for (unsigned int level = 0; level <= m->level; ++level) {
        for (unsigned int texture = 0; texture < MP_BLOCK_TEXTURE_SIDE_COUNT; ++texture) {
            if (!m->texturesSide[level][texture]) {
                m->texturesSide[level][texture] = texturesSide[level][texture];
            }
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
// Meta implementation
///////////////////////////////////////////////////////////////////////////////

/** Reset defaults on map change */
static void resetDefaults(void) {
    gMetaDefaults.id = 0;
    gMetaDefaults.name = NULL;
    gMetaDefaults.level = MP_BLOCK_LEVEL_HIGH;
    gMetaDefaults.passability = 0;
    gMetaDefaults.durability = 0;
    gMetaDefaults.strength = 0;
    gMetaDefaults.gold = 0;
    gMetaDefaults.becomes = NULL;
}

/** New type registered */
inline static bool initMeta(MP_BlockMeta* m, const MP_BlockMeta* meta) {
    *m = *meta;
    loadTexturesFor(m);

    return true;
}

/** Type override */
inline static bool updateMeta(MP_BlockMeta* m, const MP_BlockMeta* meta) {
    m->durability = meta->durability;
    m->strength = meta->strength;
    m->gold = meta->gold;
    m->level = meta->level;
    m->passability = meta->passability;
    m->becomes = meta->becomes;

    return true;
}

/** Clear up data for a meta on deletion */
inline static void deleteMeta(MP_BlockMeta* m) {
}

META_impl(MP_BlockMeta, Block)

int MP_Lua_BlockMetaDefaults(lua_State* L) {
    // Validate input.
    luaL_argcheck(L, lua_gettop(L) == 1 && lua_istable(L, 1), 0, "must specify one table");

    // Build the block meta using the given properties.
    tableToBlock(L, &gMetaDefaults, true /* for defaults */);

    return 0;
}

int MP_Lua_AddBlockMeta(lua_State* L) {
    // New type, start with defaults.
    MP_BlockMeta meta = gMetaDefaults;

    // Validate input.
    luaL_argcheck(L, lua_gettop(L) == 1 && lua_istable(L, 1), 0, "must specify one table");

    // Build the block meta using the given properties.
    tableToBlock(L, &meta, false /* not for defaults */);

    // We require for at least the name to be set.
    luaL_argcheck(L, meta.name, 1, "name is required but not set");

    // All green, add the type.
    if (!MP_AddBlockMeta(&meta)) {
        return luaL_argerror(L, 1, "bad block meta");
    }

    return 0;
}
