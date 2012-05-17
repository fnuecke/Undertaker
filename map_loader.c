#include "map_loader.h"

#include "config.h"
#include "events.h"
#include "job.h"
#include "log.h"
#include "map.h"
#include "meta.h"
#include "meta_block.h"
#include "meta_job.h"
#include "meta_room.h"
#include "meta_unit.h"
#include "textures.h"
#include "unit.h"

///////////////////////////////////////////////////////////////////////////////
// Save / Load methods
///////////////////////////////////////////////////////////////////////////////

static void clear(void) {
    // Unload resources.
    MP_UnloadTextures();

    // Clear meta information.
    MP_ClearBlockMeta();
    MP_ClearJobMeta();
    MP_ClearRoomMeta();
    MP_ClearUnitMeta();

    // Clear unit list.
    MP_ClearUnits();
}

void MP_LoadMap(const char* mapname) {
    if (!mapname) {
        return;
    }

    MP_log_info("Loading map '%s'.\n", mapname);
    fflush(MP_logTarget);

    // Kill remaining jobs.
    MP_ClearJobs();

    clear();

    // Load new meta information for the map.
    if (!MP_LoadMeta(mapname)) {
        // Remove anything we might have parsed.
        clear();
        return;
    }

    fflush(MP_logTarget);

    // Adjust map size and set default block type.
    MP_SetMapSize(32, MP_GetBlockMeta(1));

    // Load new resources onto GPU.
    MP_GL_GenerateTextures();
}

void MP_SaveMap(const char* filename) {

}
