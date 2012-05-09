#include "map_loader.h"

#include "block_meta.h"
#include "config.h"
#include "events.h"
#include "jobs.h"
#include "jobs_meta.h"
#include "log.h"
#include "map.h"
#include "meta.h"
#include "room_meta.h"
#include "textures.h"
#include "units_meta.h"

///////////////////////////////////////////////////////////////////////////////
// Save / Load methods
///////////////////////////////////////////////////////////////////////////////

void MP_LoadMap(const char* mapname) {
    if (!mapname) {
        return;
    }

    MP_log_info("Loading map '%s'.\n", mapname);
    fflush(MP_log_target);

    // Kill remaining jobs.
    MP_ClearJobs();

    // Unload resources.
    MP_UnloadTextures();

    // Clear meta information.
    MP_ClearBlockMeta();
    MP_ClearJobMeta();
    MP_ClearRoomMeta();
    MP_ClearUnitMeta();

    // Load new meta information for the map.
    if (!MP_LoadMeta(mapname)) {
        return;
    }

    fflush(MP_log_target);

    // Adjust map size and set default block type.
    MP_SetMapSize(128, MP_GetBlockMeta(1));

    // Load new resources onto GPU.
    MP_GL_GenerateTextures();
}

void MP_SaveMap(const char* filename) {

}
