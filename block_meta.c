#include <stdio.h>

#include "block_meta.h"
#include "textures.h"

/** File name suffixes for textures */
const char* BLOCK_LEVEL_SUFFIX[] = {
    [DK_BLOCK_LEVEL_PIT] = "pit",
    [DK_BLOCK_LEVEL_LOWERED] = "lowered",
    [DK_BLOCK_LEVEL_NORMAL] = "normal",
    [DK_BLOCK_LEVEL_HIGH] = "high"
};

const char* BLOCK_TEXTURE_SUFFIX_TOP[] = {
    [DK_BLOCK_TEXTURE_TOP] = "top",
    [DK_BLOCK_TEXTURE_TOP_OWNED_OVERLAY] = "top_o",
    [DK_BLOCK_TEXTURE_TOP_N] = "top_on",
    [DK_BLOCK_TEXTURE_TOP_NE] = "top_one",
    [DK_BLOCK_TEXTURE_TOP_NS] = "top_ons",
    [DK_BLOCK_TEXTURE_TOP_NES] = "top_ones",
    [DK_BLOCK_TEXTURE_TOP_NESW] = "top_onesw",
    [DK_BLOCK_TEXTURE_TOP_NE_CORNER] = "top_onec",
    [DK_BLOCK_TEXTURE_TOP_NES_CORNER] = "top_onesc",
    [DK_BLOCK_TEXTURE_TOP_NESW_CORNER] = "top_oneswc",
    [DK_BLOCK_TEXTURE_TOP_NESWN_CORNER] = "top_oneswnc"
};

const char* BLOCK_TEXTURE_SUFFIX_SIDE[] = {
    [DK_BLOCK_TEXTURE_SIDE] = "side",
    [DK_BLOCK_TEXTURE_SIDE_OWNED_OVERLAY] = "side_o"
};

static void loadTexturesFor(DK_BlockMeta* m) {
    char basename[128];
    DK_TextureID texturesTop[DK_BLOCK_TEXTURE_TOP_COUNT];
    DK_TextureID texturesSide[DK_BLOCK_LEVEL_COUNT][DK_BLOCK_TEXTURE_SIDE_COUNT];

    // Clear.
    memset(texturesTop, 0, DK_BLOCK_TEXTURE_TOP_COUNT * sizeof (DK_TextureID));
    memset(texturesSide, 0, DK_BLOCK_LEVEL_COUNT * DK_BLOCK_TEXTURE_SIDE_COUNT * sizeof (DK_TextureID));
    memset(m->texturesTop, 0, DK_BLOCK_TEXTURE_TOP_COUNT * sizeof (DK_TextureID));
    memset(m->texturesSide, 0, DK_BLOCK_LEVEL_COUNT * DK_BLOCK_TEXTURE_SIDE_COUNT * sizeof (DK_TextureID));

    // Load textures.
    for (unsigned int texture = 0, max = m->strength ? DK_BLOCK_TEXTURE_TOP_COUNT : 1; texture < max; ++texture) {
        // Build file name.
        sprintf(basename, "%s_%s", m->name, BLOCK_TEXTURE_SUFFIX_TOP[texture]);
        // Try to have it loaded.
        m->texturesTop[texture] = DK_LoadTexture(basename);

        // Remember the first existing type of a texture to fill up later.
        if (!texturesTop[texture]) {
            texturesTop[texture] = m->texturesTop[texture];
        }
    }
    for (unsigned int level = 0; level <= m->level; ++level) {
        for (unsigned int texture = 0, max = m->strength ? DK_BLOCK_TEXTURE_SIDE_COUNT : 1; texture < max; ++texture) {
            // Build file name.
            sprintf(basename, "%s_%s_%s", m->name, BLOCK_LEVEL_SUFFIX[level], BLOCK_TEXTURE_SUFFIX_SIDE[texture]);
            // Try to have it loaded.
            m->texturesSide[level][texture] = DK_LoadTexture(basename);

            // Remember the first existing type of a texture to fill up later.
            if (!texturesSide[level][texture]) {
                texturesSide[level][texture] = m->texturesSide[level][texture];
            }
        }
    }

    // Fill up gaps with the first texture we have at any level.
    for (unsigned int texture = 0; texture < DK_BLOCK_TEXTURE_TOP_COUNT; ++texture) {
        if (!m->texturesTop[texture]) {
            m->texturesTop[texture] = texturesTop[texture];
        }
    }
    for (unsigned int level = 0; level <= m->level; ++level) {
        for (unsigned int texture = 0; texture < DK_BLOCK_TEXTURE_SIDE_COUNT; ++texture) {
            if (!m->texturesSide[level][texture]) {
                m->texturesSide[level][texture] = texturesSide[level][texture];
            }
        }
    }
}

static void initMeta(DK_BlockMeta* m, const DK_BlockMeta* meta) {
    *m = *meta;
    loadTexturesFor(m);
}

static void updateMeta(DK_BlockMeta* m, const DK_BlockMeta* meta) {
    m->durability = meta->durability;
    m->strength = meta->strength;
    m->gold = meta->gold;
    m->level = meta->level;
    m->passability = meta->passability;
    m->becomes = meta->becomes;
}

META_impl(DK_BlockMeta, Block)
