#include "meta.h"

#include "block_meta.h"
#include "passability.h"
#include "jobs_meta.h"
#include "room_meta.h"
#include "units_meta.h"

bool DK_LoadMeta(const char* name) {
    char filename[128];
    lua_State* L;

    // Build the file name.
    if (snprintf(filename, sizeof (filename), "data/meta/%s.lua", name) > (int) sizeof (filename)) {
        fprintf(DK_log_target, "ERROR: Meta name too long: '%s'.\n", name);
        return false;
    }

    // Create the lua environment we use for parsing the meta data.
    L = luaL_newstate();

    // Register our callbacks to allow defining meta data.
    lua_register(L, "block", DK_Lua_AddBlockMeta);
    lua_register(L, "blockdefaults", DK_Lua_BlockMetaDefaults);
    lua_register(L, "job", DK_Lua_AddJobMeta);
    lua_register(L, "passability", DK_Lua_AddPassability);
    //lua_register(L, "room", DK_Lua_AddRoomMeta);
    //lua_register(L, "unit", DK_Lua_AddUnitMeta);

    fprintf(DK_log_target, "INFO: Start parsing meta file '%s'.\n", filename);

    // Try to parse the file.
    if (luaL_dofile(L, filename) != LUA_OK) {
        // Print error and clean up.
        fprintf(DK_log_target, "ERROR: Failed parsing meta file: %s\n", lua_tostring(L, -1));
        lua_close(L);
        return false;
    }

    fprintf(DK_log_target, "INFO: Done parsing meta file '%s'.\n", filename);

    // Clean up.
    lua_close(L);

    return true;
}
