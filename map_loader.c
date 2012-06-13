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
    MP_ClearBlockMeta();
    MP_ClearJobMeta();
    MP_ClearRoomMeta();
    MP_ClearUnitMeta();

    // Clear unit list.
    MP_ClearUnits();
}

void MP_LoadMap(const char* mapname) {
    lua_State* L;

    if (!mapname) {
        return;
    }

    MP_log_info("Loading map '%s'.\n", mapname);
    fflush(MP_logTarget);

    // Clean up.
    clear();

    // Create the lua environment.
    L = MP_InitLua();

    // Set meta parsing environment.
    MP_SetLoadingScriptGlobals(L);

    // Try to parse the file.
    lua_pushcfunction(L, MP_Lua_Import);
    lua_pushfstring(L, "data/maps/%s.lua", mapname);
    if (MP_Lua_pcall(L, 1, 0) != LUA_OK) {
        MP_log_error("Failed parsing map file:\n%s\n", lua_tostring(L, -1));

        // Remove anything we might have parsed.
        clear();

        // Shut down Lua VM.
        MP_CloseLua(L);
    } else {
        MP_log_info("Done parsing map '%s'.\n", mapname);

        // Load new resources onto GPU.
        MP_GL_GenerateTextures();

        // Switch to ingame script environment.
        MP_SetGameScriptGlobals(L);
    }

    fflush(MP_logTarget);
}

void MP_SaveMap(const char* filename) {

}
