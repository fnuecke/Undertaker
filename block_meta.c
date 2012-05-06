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
const char* BLOCK_TEXTURE_SUFFIX[] = {
    [DK_BLOCK_TEXTURE_SIDE] = "side",
    [DK_BLOCK_TEXTURE_SIDE_OWNED_OVERLAY] = "side_owned",
    [DK_BLOCK_TEXTURE_TOP] = "top",
    [DK_BLOCK_TEXTURE_TOP_N] = "top_n",
    [DK_BLOCK_TEXTURE_TOP_NE] = "top_ne",
    [DK_BLOCK_TEXTURE_TOP_NS] = "top_ns",
    [DK_BLOCK_TEXTURE_TOP_NES] = "top_nes",
    [DK_BLOCK_TEXTURE_TOP_NESW] = "top_nesw",
    [DK_BLOCK_TEXTURE_TOP_NE_CORNER] = "top_nec",
    [DK_BLOCK_TEXTURE_TOP_NES_CORNER] = "top_nesc",
    [DK_BLOCK_TEXTURE_TOP_NESW_CORNER] = "top_neswc",
    [DK_BLOCK_TEXTURE_TOP_NESWN_CORNER] = "top_neswnc",
    [DK_BLOCK_TEXTURE_TOP_OWNED_OVERLAY] = "top_owned",
};

static void loadTexturesFor(DK_BlockMeta* m) {
    char basename[128];
    DK_TextureID textures[DK_BLOCK_TEXTURE_COUNT] = {0};

    // Clear.
    memset(m->textures, 0, DK_BLOCK_LEVEL_COUNT * DK_BLOCK_TEXTURE_COUNT * sizeof (DK_TextureID));

    // Load textures.
    for (unsigned int level = 0; level <= m->level; ++level) {
        for (unsigned int texture = 0; texture < DK_BLOCK_TEXTURE_COUNT; ++texture) {
            // Build file name.
            sprintf(basename, "%s_%s_%s", m->name, BLOCK_LEVEL_SUFFIX[level], BLOCK_TEXTURE_SUFFIX[texture]);
            // Try to have it loaded.
            m->textures[level][texture] = DK_LoadTexture(basename);

            // Remember the first existing type of a texture to fill up later.
            if (!textures[texture]) {
                textures[texture] = m->textures[level][texture];
            }
        }
    }

    // Fill up gaps with the first texture we have at any level.
    for (unsigned int level = 0; level <= m->level; ++level) {
        for (unsigned int texture = 0; texture < DK_BLOCK_TEXTURE_COUNT; ++texture) {
            if (!m->textures[level][texture]) {
                m->textures[level][texture] = textures[texture];
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
