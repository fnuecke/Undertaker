#include <malloc.h>
#include <stdio.h>
#include <string.h>

#include "block_meta.h"
#include "map.h"
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

static DK_BlockMeta* gMetas = 0;
static unsigned int gMetaCount = 0;
static unsigned int gMetaCapacity = 0;

static DK_BlockMeta* getNextFreeEntry(void) {
    if (gMetaCount >= gMetaCapacity) {
        gMetaCapacity = gMetaCapacity * 2 + 1;
        gMetas = realloc(gMetas, gMetaCapacity * sizeof (DK_BlockMeta));
    }
    return &gMetas[gMetaCount++];
}

static void onMapChange(void) {
    gMetaCount = 0;
}

const DK_BlockMeta* DK_GetBlockMeta(unsigned int id) {
    if (id < gMetaCount) {
        return &gMetas[id];
    }
    return NULL;
}

const DK_BlockMeta* DK_GetBlockMetaByName(const char* name) {
    for (unsigned int id = 0; id < gMetaCount; ++id) {
        if (strcmp(name, gMetas[id].name) == 0) {
            return &gMetas[id];
        }
    }
    return NULL;
}

void DK_AddBlockMeta(const DK_BlockMeta* meta) {
    char basename[128];
    DK_TextureID textures[DK_BLOCK_TEXTURE_COUNT] = {0};

    // Create new entry and copy data.
    DK_BlockMeta* m = getNextFreeEntry();
    *m = *meta;

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

void DK_InitBlockMeta(void) {
    DK_OnMapSizeChange(onMapChange);
}
