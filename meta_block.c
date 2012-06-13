#include "meta_block.h"

#include <stdio.h>

#include "meta_impl.h"
#include "passability.h"
#include "script.h"
#include "textures.h"

///////////////////////////////////////////////////////////////////////////////
// Constants / globals
///////////////////////////////////////////////////////////////////////////////

/** File name suffixes for textures */
const char* BLOCK_LEVEL_SUFFIX[] = {[MP_BLOCK_LEVEL_PIT] = "pit",
                                    [MP_BLOCK_LEVEL_LOWERED] = "lowered",
                                    [MP_BLOCK_LEVEL_NORMAL] = "normal",
                                    [MP_BLOCK_LEVEL_HIGH] = "high"};

const char* BLOCK_TEXTURE_SUFFIX_TOP[] = {[MP_BLOCK_TEXTURE_TOP] = "top",
                                          [MP_BLOCK_TEXTURE_TOP_OWNED_OVERLAY] = "top_o",
                                          [MP_BLOCK_TEXTURE_TOP_N] = "top_on",
                                          [MP_BLOCK_TEXTURE_TOP_NE] = "top_one",
                                          [MP_BLOCK_TEXTURE_TOP_NS] = "top_ons",
                                          [MP_BLOCK_TEXTURE_TOP_NES] = "top_ones",
                                          [MP_BLOCK_TEXTURE_TOP_NESW] = "top_onesw",
                                          [MP_BLOCK_TEXTURE_TOP_NE_CORNER] = "top_onec",
                                          [MP_BLOCK_TEXTURE_TOP_NES_CORNER] = "top_onesc",
                                          [MP_BLOCK_TEXTURE_TOP_NESW_CORNER] = "top_oneswc",
                                          [MP_BLOCK_TEXTURE_TOP_NESWN_CORNER] = "top_oneswnc"};

const char* BLOCK_TEXTURE_SUFFIX_SIDE[] = {[MP_BLOCK_TEXTURE_SIDE] = "side",
                                           [MP_BLOCK_TEXTURE_SIDE_OWNED_OVERLAY] = "side_o"};

META_globals(MP_BlockMeta)

///////////////////////////////////////////////////////////////////////////////
// Helper methods
///////////////////////////////////////////////////////////////////////////////

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
    gMetaDefaults.lightFrequency = 0;
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
    m->level = meta->level;
    m->passability = meta->passability;
    m->lightFrequency = meta->lightFrequency;
    m->durability = meta->durability;
    m->strength = meta->strength;
    m->gold = meta->gold;
    m->becomes = meta->becomes;

    return true;
}

/** Clear up data for a meta on deletion */
inline static void deleteMeta(MP_BlockMeta* m) {
}

META_impl(MP_BlockMeta, Block)
