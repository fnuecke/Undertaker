#include "block_type.h"

#include <stdio.h>

#include "type_impl.h"
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

TYPE_GLOBALS(MP_BlockType)

///////////////////////////////////////////////////////////////////////////////
// Helper methods
///////////////////////////////////////////////////////////////////////////////

/** Texture loading for a single type */
static void loadTexturesFor(MP_BlockType* type) {
    char basename[128];
    MP_TextureID texturesTop[MP_BLOCK_TEXTURE_TOP_COUNT];
    MP_TextureID texturesSide[MP_BLOCK_LEVEL_COUNT][MP_BLOCK_TEXTURE_SIDE_COUNT];

    // Clear.
    memset(texturesTop, 0, MP_BLOCK_TEXTURE_TOP_COUNT * sizeof (MP_TextureID));
    memset(texturesSide, 0, MP_BLOCK_LEVEL_COUNT * MP_BLOCK_TEXTURE_SIDE_COUNT * sizeof (MP_TextureID));
    memset(type->texturesTop, 0, MP_BLOCK_TEXTURE_TOP_COUNT * sizeof (MP_TextureID));
    memset(type->texturesSide, 0, MP_BLOCK_LEVEL_COUNT * MP_BLOCK_TEXTURE_SIDE_COUNT * sizeof (MP_TextureID));

    // Load textures.
    for (unsigned int texture = 0, max = type->strength ? MP_BLOCK_TEXTURE_TOP_COUNT : 1; texture < max; ++texture) {
        // Build file name.
        snprintf(basename, sizeof (basename), "%s_%s", type->info.name, BLOCK_TEXTURE_SUFFIX_TOP[texture]);

        // Try to have it loaded.
        type->texturesTop[texture] = MP_LoadTexture(basename);

        // Remember the first existing type of a texture to fill up later.
        if (!texturesTop[texture]) {
            texturesTop[texture] = type->texturesTop[texture];
        }
    }
    for (unsigned int level = 0; level <= type->level; ++level) {
        for (unsigned int texture = 0, max = type->strength ? MP_BLOCK_TEXTURE_SIDE_COUNT : 1; texture < max; ++texture) {
            // Build file name.
            snprintf(basename, sizeof (basename), "%s_%s_%s", type->info.name, BLOCK_LEVEL_SUFFIX[level], BLOCK_TEXTURE_SUFFIX_SIDE[texture]);

            // Try to have it loaded.
            type->texturesSide[level][texture] = MP_LoadTexture(basename);

            // Remember the first existing type of a texture to fill up later.
            if (!texturesSide[level][texture]) {
                texturesSide[level][texture] = type->texturesSide[level][texture];
            }
        }
    }

    // Fill up gaps with the first texture we have at any level.
    for (unsigned int texture = 0; texture < MP_BLOCK_TEXTURE_TOP_COUNT; ++texture) {
        if (!type->texturesTop[texture]) {
            type->texturesTop[texture] = texturesTop[texture];
        }
    }
    for (unsigned int level = 0; level <= type->level; ++level) {
        for (unsigned int texture = 0; texture < MP_BLOCK_TEXTURE_SIDE_COUNT; ++texture) {
            if (!type->texturesSide[level][texture]) {
                type->texturesSide[level][texture] = texturesSide[level][texture];
            }
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
// Type implementation
///////////////////////////////////////////////////////////////////////////////

/** Reset defaults on map change */
static void resetDefaults(void) {
    gTypeDefaults.level = MP_BLOCK_LEVEL_HIGH;
    gTypeDefaults.passability = 0;
    gTypeDefaults.lightFrequency = 0;
    gTypeDefaults.durability = 0;
    gTypeDefaults.strength = 0;
    gTypeDefaults.gold = 0;
    gTypeDefaults.becomes = NULL;
}

/** New type registered */
inline static bool initType(MP_BlockType* stored, const MP_BlockType* input) {
    *stored = *input;
    loadTexturesFor(stored);

    return true;
}

/** Type override */
inline static bool updateType(MP_BlockType* stored, const MP_BlockType* input) {
    stored->level = input->level;
    stored->passability = input->passability;
    stored->lightFrequency = input->lightFrequency;
    stored->durability = input->durability;
    stored->strength = input->strength;
    stored->gold = input->gold;
    stored->becomes = input->becomes;

    return true;
}

/** Clear up data for a meta on deletion */
inline static void deleteType(MP_BlockType* type) {
}

TYPE_IMPL(MP_BlockType, Block)
