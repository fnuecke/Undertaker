/* 
 * Author: fnuecke
 *
 * Created on May 10, 2012, 6:10 PM
 */

#ifndef SCRIPT_H
#define	SCRIPT_H

#include "lua/lua.h"
#include "lua/lauxlib.h"

#include "types.h"
#include "vmath.h"

#ifdef	__cplusplus
extern "C" {
#endif

    ///////////////////////////////////////////////////////////////////////////
    // Libraries and API extensions
    ///////////////////////////////////////////////////////////////////////////

#define MP_LUA_LIBRARY(NAME) \
    int MP_Lua_Open##NAME(lua_State*); \
    void MP_Lua_Push##NAME(lua_State*, MP_##NAME*); \
    bool MP_Lua_Is##NAME(lua_State*, int); \
    MP_##NAME* MP_Lua_To##NAME(lua_State*, int); \
    MP_##NAME* MP_Lua_Check##NAME(lua_State*, int); \
    const MP_##NAME##Type* MP_Lua_Check##NAME##Type(lua_State*, int)

#define LUA_ABILITYLIBNAME "Ability"
    MP_LUA_LIBRARY(Ability);

#define LUA_JOBLIBNAME "Job"
    MP_LUA_LIBRARY(Job);

#define LUA_BLOCKLIBNAME "Block"
    MP_LUA_LIBRARY(Block);

#define LUA_UNITLIBNAME "Unit"
    MP_LUA_LIBRARY(Unit);

#define LUA_ROOMLIBNAME "Room"
    MP_LUA_LIBRARY(Room);
    
#undef MP_LUA_LIBRARY    

    void MP_Lua_PushVec2(lua_State* L, vec2 v);
    MP_Passability MP_Lua_CheckPassability(lua_State* L, int narg);
    MP_Player MP_Lua_CheckPlayer(lua_State* L, int narg);
    MP_BlockLevel MP_Lua_CheckLevel(lua_State* L, int narg);

    /** Same as lua_pcall, just that it produces a stack trace */
    int MP_Lua_pcall(lua_State* L, int nargs, int nresults);

    void pushImmutableProxy(lua_State* L, int index);

    ///////////////////////////////////////////////////////////////////////////
    // State
    ///////////////////////////////////////////////////////////////////////////

    /**
     * Setup basic globals structure for a script VM. Shuts down a previous VM.
     */
    lua_State* MP_Lua_Init(void);

    /** Get the current Lua VM. */
    lua_State* MP_Lua(void);

    /** Shut down the Lua VM. */
    void MP_Lua_Close(void);

    /** Loads the script from the specified path. */
    int MP_Lua_Load(const char* string);

#ifdef	__cplusplus
}
#endif

#endif
