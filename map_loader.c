#include <string.h>

#include "block.h"
#include "map.h"
#include "map_loader.h"
#include "textures.h"
#include "units_meta.h"

static bool isLoading = false;

static const char* rock = "rock";
static const char* imp = "imp";

static void onMapSizeChange(void) {
    DK_BlockMeta bm;
    DK_UnitMeta um;

    if (!isLoading) {
        return;
    }

    DK_UnloadTextures();

    bm.name = rock;
    bm.durability = 0;
    bm.strength = 0;
    bm.gold = 0;
    bm.passability = 1;
    bm.level = DK_BLOCK_LEVEL_HIGH;
    bm.becomes = 0;

    DK_AddBlockMeta(&bm);

    memset(&um, 0, sizeof (DK_UnitMeta));
    um.name = imp;
    um.passability = 1;

    DK_AddUnitMeta(&um);

    DK_GL_GenerateTextures();
}

void DK_LoadMap(const char* filename) {
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