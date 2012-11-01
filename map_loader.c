#include <assert.h>
#include <string.h>

#include "map_loader.h"

#include "config.h"
#include "events.h"
#include "job.h"
#include "log.h"
#include "map.h"
#include "type.h"
#include "block_type.h"
#include "job_type.h"
#include "room_type.h"
#include "unit_type.h"
#include "textures.h"
#include "unit.h"
#include "script.h"

///////////////////////////////////////////////////////////////////////////////
// Save / Load methods
///////////////////////////////////////////////////////////////////////////////

static void clear(void) {
    // Remove existing jobs.
    MP_ClearJobs();

    // Unload resources.
    MP_UnloadTextures();

    // Clear meta information.
    MP_ClearBlockTypes();
    MP_ClearJobTypes();
    MP_ClearRoomTypes();
    MP_ClearUnitTypes();

    // Clear unit list.
    MP_ClearUnits();
}

void MP_LoadMap(const char* name) {
    char path[256];

    assert(name);
    assert(strlen(name) > 0);

    MP_log_info("Loading map '%s'.\n", name);
    fflush(MP_logTarget);

    sprintf(path, "data/maps/%s.lua", name);

    // Clean up.
    clear();

    // Create the lua environment.
    MP_Lua_Init();

    // Try to load the map.
    if (MP_Lua_Load(path) != LUA_OK) {
        MP_log_error("Failed parsing map file:\n%s\n", lua_tostring(MP_Lua(), -1));

        // Remove anything we might have parsed.
        clear();

        // Shut down Lua VM.
        MP_Lua_Close();
    } else {
        MP_log_info("Done parsing map '%s'.\n", name);

        // Load new resources onto GPU.
        MP_GL_GenerateTextures();
    }

    fflush(MP_logTarget);
}

void MP_SaveMap(const char* filename) {

}
