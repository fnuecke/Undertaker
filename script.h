/* 
 * File:   script.h
 * Author: fnuecke
 *
 * Created on May 10, 2012, 6:10 PM
 */

#ifndef SCRIPT_H
#define	SCRIPT_H

#include "lua/lua.h"
#include "lua/lauxlib.h"

#include "types.h"

#ifdef	__cplusplus
extern "C" {
#endif

#define LUA_BLOCKLIBNAME "Block"
    int (luaopen_block) (lua_State* L);

#define LUA_JOBLIBNAME "Job"
    int (luaopen_job) (lua_State* L);

#define LUA_ROOMLIBNAME "Room"
    int (luaopen_room) (lua_State* L);

#define LUA_UNITLIBNAME "Unit"
    int (luaopen_unit) (lua_State* L);

    void luaMP_pushblock(lua_State* L, MP_Block* block);
    bool luaMP_isblock(lua_State* L, int narg);
    MP_Block* luaMP_toblock(lua_State* L, int narg);
    MP_Block* luaMP_checkblock(lua_State* L, int narg, int errarg);

    void luaMP_pushjob(lua_State* L, MP_Job* job);
    bool luaMP_isjob(lua_State* L, int narg);
    MP_Job* luaMP_tojob(lua_State* L, int narg);
    MP_Job* luaMP_checkjob(lua_State* L, int narg, int errarg);

    void luaMP_pushroom(lua_State* L, MP_Room* room);
    bool luaMP_isroom(lua_State* L, int narg);
    MP_Room* luaMP_toroom(lua_State* L, int narg);
    MP_Room* luaMP_checkroom(lua_State* L, int narg, int errarg);

    void luaMP_pushunit(lua_State* L, MP_Unit* unit);
    bool luaMP_isunit(lua_State* L, int narg);
    MP_Unit* luaMP_tounit(lua_State* L, int narg);
    MP_Unit* luaMP_checkunit(lua_State* L, int narg, int errarg);

    int MP_Lua_pcall(lua_State* L, int nargs, int nresults);

    MP_Passability luaMP_checkpassability(lua_State* L, int narg, int errarg);
    MP_Player luaMP_checkplayer(lua_State* L, int narg, int errarg);
    MP_BlockLevel luaMP_checklevel(lua_State* L, int narg, int errarg);
    const MP_BlockMeta* luaMP_checkblockmeta(lua_State* L, int narg, int errarg);
    const MP_JobMeta* luaMP_checkjobmeta(lua_State* L, int narg, int errarg);
    const MP_RoomMeta* luaMP_checkroommeta(lua_State* L, int narg, int errarg);
    const MP_UnitMeta* luaMP_checkunitmeta(lua_State* L, int narg, int errarg);

#ifdef	__cplusplus
}
#endif

#endif	/* SCRIPT_H */