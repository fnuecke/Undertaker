/* 
 * File:   script_loading_aux.h
 * Author: fnuecke
 *
 * Created on May 26, 2012, 2:21 PM
 */

#ifndef SCRIPT_LOADING_AUX_H
#define	SCRIPT_LOADING_AUX_H

#include "lua/lua.h"
#include "lua/lauxlib.h"

#ifdef	__cplusplus
extern "C" {
#endif

    int MP_Lua_WarnIndex(lua_State* L);
    int MP_Lua_ThrowErrorNewIndex(lua_State* L);
    int MP_Lua_BuildImportPath(lua_State* L, const char* pathfmt);
    void MP_GetImmutableProxy(lua_State* L);

    void require(lua_State* L, const char *modname, lua_CFunction openf, bool mt);
    void MP_PushScriptGlobals(lua_State* L, const luaL_Reg* methods);
    void MP_PopScriptGlobals(lua_State* L);

    void luaMP_pushglobalstable(lua_State* L);
    void luaMP_pushlocalstable(lua_State* L);

#define MP_CreateScriptLocalsTable(L) (luaMP_pushlocalstable(L), lua_pop(L, 1))
    
#ifdef	__cplusplus
}
#endif

#endif	/* SCRIPT_LOADING_AUX_H */

