/* 
 * Author: fnuecke
 *
 * Created on May 12, 2012, 1:28 AM
 */

#ifndef SCRIPT_EVENTS_H
#define	SCRIPT_EVENTS_H

#include "types.h"

#ifdef	__cplusplus
extern "C" {
#endif

#define MP_LUA_EVENT(NAME) \
void MP_Lua_Add##NAME##EventListener(lua_State* L, const char* jobName); \
void MP_Lua_Remove##NAME##EventListeners(lua_State* L, const char* jobName)

    MP_LUA_EVENT(UnitAdded);

    MP_LUA_EVENT(BlockSelectionChanged);

    MP_LUA_EVENT(BlockTypeChanged);

    MP_LUA_EVENT(BlockOwnerChanged);

#undef MP_LUA_EVENT

    void MP_InitLuaEvents(void);

#ifdef	__cplusplus
}
#endif

#endif
