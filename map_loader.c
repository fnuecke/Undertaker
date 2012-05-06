#include "block.h"
#include "map.h"
#include "map_loader.h"
#include "textures.h"

static bool isLoading = false;

static const char* rock = "rock";

static void onMapSizeChange(void) {
    DK_BlockMeta m;

    if (!isLoading) {
        return;
    }

    DK_UnloadTextures();

    m.becomes = 0;
    m.name = rock;
    m.durability = 0;
    m.gold = 0;
    m.id = 0;
    m.level = DK_BLOCK_LEVEL_HIGH;
    m.passability = 0;
    m.strength = 0;

    DK_AddBlockMeta(&m);
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