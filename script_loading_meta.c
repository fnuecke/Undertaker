#include "script_loading.h"

#include "meta_ability.h"
#include "meta_block.h"
#include "meta_job.h"
#include "meta_room.h"
#include "meta_unit.h"
#include "script_loading_aux.h"

static int lua_Import(lua_State* L) {
    MP_Lua_BuildImportPath(L, "data/meta/%s.lua");
    MP_Lua_Import(L);
    return 0;
}

static const luaL_Reg metalib[] = {
    {"ability", MP_Lua_AddAbilityMeta},
    {"block", MP_Lua_AddBlockMeta},
    {"blockdefaults", MP_Lua_BlockMetaDefaults},
    {"import", lua_Import},
    {"job", MP_Lua_AddJobMeta},
    {"passability", MP_Lua_AddPassability},
    //{"room", MP_Lua_AddRoomMeta},
    {"unitdefaults", MP_Lua_UnitMetaDefaults},
    {"unit", MP_Lua_AddUnitMeta},
    {NULL, NULL}
};

int MP_Lua_ImportMeta(lua_State* L) {
    MP_PushScriptGlobals(L, metalib);
    MP_CreateScriptLocalsTable(L);

    lua_Import(L);

    MP_PopScriptGlobals(L);
    return 0;
}
