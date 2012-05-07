#include "meta.h"

#include "block_meta.h"
#include "passability.h"
#include "jobs_meta.h"
#include "room_meta.h"
#include "units_meta.h"

bool DK_LoadMeta(const char* name) {
    char filename[128];
    lua_State* L = luaL_newstate();

    lua_register(L, "block", DK_Lua_AddBlockMeta);
    lua_register(L, "blockdefaults", DK_Lua_BlockMetaDefaults);
    //lua_register(L, "job", DK_Lua_AddJobMeta);
    lua_register(L, "passability", DK_Lua_AddPassability);
    //lua_register(L, "room", DK_Lua_AddRoomMeta);
    //lua_register(L, "unit", DK_Lua_AddUnitMeta);

    sprintf(filename, "data/meta/%s.lua", name);

    fprintf(DK_log_target, "INFO: Start parsing meta file '%s'.\n", filename);

    if (luaL_dofile(L, filename) != LUA_OK) {
        fprintf(DK_log_target, "ERROR: Failed meta file: %s\n", lua_tostring(L, -1));
        return false;
    }

    fprintf(DK_log_target, "INFO: Done parsing meta file '%s'.\n", filename);

    lua_close(L);

    return true;
}
