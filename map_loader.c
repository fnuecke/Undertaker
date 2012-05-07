#include "map_loader.h"

#include <stdio.h>

#include "block_meta.h"
#include "config.h"
#include "events.h"
#include "jobs_meta.h"
#include "map.h"
#include "meta.h"
#include "room_meta.h"
#include "textures.h"
#include "units_meta.h"

///////////////////////////////////////////////////////////////////////////////
// Save / Load methods
///////////////////////////////////////////////////////////////////////////////

void DK_LoadMap(const char* mapname) {
    if (!mapname) {
        return;
    }

    fprintf(DK_log_target, "INFO: Loading map '%s'.\n", mapname);

    // Unload resources.
    DK_UnloadTextures();

    // Clear meta information.
    DK_ClearBlockMeta();
    DK_ClearJobMeta();
    DK_ClearRoomMeta();
    DK_ClearUnitMeta();

    // Load new meta information for the map.
    DK_LoadMeta(mapname);

    // Adjust map size and set default block type.
    DK_SetMapSize(128);
    DK_FillMap(DK_GetBlockMeta(1));

    // Load new resources onto GPU.
    DK_GL_GenerateTextures();
}

void DK_SaveMap(const char* filename) {

}
