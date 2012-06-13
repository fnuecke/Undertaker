#include "script_loading.h"
#include "script_loading_aux.h"

static int lua_SetMapSize(lua_State* L) {
    unsigned int size = luaL_checkunsigned(L, 1);
    const MP_BlockMeta* meta = luaMP_checkblockmeta(L, 2);
    if (size < 1) {
        return luaL_error(L, "invalid map size");
    }
    MP_SetMapSize(size, meta);
}

static const luaL_Reg maplib[] = {
    {"meta", MP_Lua_ImportMeta},
    {"size", lua_SetMapSize},
    {NULL, NULL}
};

void MP_SetLoadingScriptGlobals(lua_State* L) {
    MP_PushScriptGlobals(L, maplib);
}
