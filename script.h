/* 
 * File:   job_script.h
 * Author: fnuecke
 *
 * Created on May 10, 2012, 6:10 PM
 */

#ifndef JOB_SCRIPT_H
#define	JOB_SCRIPT_H

#include "lua/lua.h"
#include "lua/lauxlib.h"

#include "types.h"

#ifdef	__cplusplus
extern "C" {
#endif

    ///////////////////////////////////////////////////////////////////////////
    // Libraries
    ///////////////////////////////////////////////////////////////////////////

#define LUA_BLOCKLIBNAME "Block"
    int (luaopen_block) (lua_State* L);

#define LUA_JOBLIBNAME "Job"
    int (luaopen_job) (lua_State* L);

#define LUA_ROOMLIBNAME "Room"
    int (luaopen_room) (lua_State* L);

#define LUA_UNITLIBNAME "Unit"
    int (luaopen_unit) (lua_State* L);

    ///////////////////////////////////////////////////////////////////////////
    // Type conversion
    ///////////////////////////////////////////////////////////////////////////

    void luaMP_pushblock(lua_State* L, MP_Block* block);
    bool luaMP_isblock(lua_State* L, int narg);
    MP_Block* luaMP_toblock(lua_State* L, int narg);
    MP_Block* luaMP_checkblock(lua_State* L, int narg);

    void luaMP_pushjob(lua_State* L, MP_Job* job);
    bool luaMP_isjob(lua_State* L, int narg);
    MP_Job* luaMP_tojob(lua_State* L, int narg);
    MP_Job* luaMP_checkjob(lua_State* L, int narg);

    void luaMP_pushroom(lua_State* L, MP_Room* room);
    bool luaMP_isroom(lua_State* L, int narg);
    MP_Room* luaMP_toroom(lua_State* L, int narg);
    MP_Room* luaMP_checkroom(lua_State* L, int narg);

    void luaMP_pushunit(lua_State* L, MP_Unit* unit);
    bool luaMP_isunit(lua_State* L, int narg);
    MP_Unit* luaMP_tounit(lua_State* L, int narg);
    MP_Unit* luaMP_checkunit(lua_State* L, int narg);

    MP_Passability luaMP_checkpassability(lua_State* L, int narg);
    MP_Player luaMP_checkplayer(lua_State* L, int narg);
    MP_BlockLevel luaMP_checklevel(lua_State* L, int narg);
    const MP_BlockMeta* luaMP_checkblockmeta(lua_State* L, int narg);
    const MP_JobMeta* luaMP_checkjobmeta(lua_State* L, int narg);
    const MP_RoomMeta* luaMP_checkroommeta(lua_State* L, int narg);
    const MP_UnitMeta* luaMP_checkunitmeta(lua_State* L, int narg);

    ///////////////////////////////////////////////////////////////////////////
    // Environment switching
    ///////////////////////////////////////////////////////////////////////////

    /**
     * Setup basic globals structure for a script VM. Shuts down a previous VM.
     */
    lua_State* MP_InitLua(void);
    
    /**
     * Shut down the Lua VM.
     */
    void MP_CloseLua(void);

    /**
     * Used for parsing map basics. Makes available meta, size and similar setters.
     */
    void MP_SetMapScriptGlobals(void);

    /**
     * Used for in-game script execution. Makes available interface methods for
     * querying/modifying game state.
     */
    void MP_SetGameScriptGlobals(void);

    /**
     * Creates a fresh local environment for the following lua calls. This can
     * be used when loading scripts to allow the scripts to fill in that local
     * environment.
     */
    void MP_NewSciptLocals(void);

    /**
     * Stores a local environment under the specified name for later retrieval.
     * This will not invalidate the current local environment.
     */
    void MP_RegisterScriptLocals(const char* type, const char* name);

    /**
     * Restore a local environment previously stored.
     */
    void MP_LoadScriptLocals(const char* type, const char* name);

    ///////////////////////////////////////////////////////////////////////////
    // Utility methods
    ///////////////////////////////////////////////////////////////////////////

    /** Same as lua_pcall, just that it produces a stack trace */
    int MP_Lua_pcall(lua_State* L, int nargs, int nresults);

    int MP_Lua_Import(lua_State* L);

    bool MP_RunJob(MP_Unit* unit, MP_Job* job, unsigned int* delay);

    float MP_GetDynamicPreference(MP_Unit* unit, const MP_JobMeta* meta);

    ///////////////////////////////////////////////////////////////////////////
    // Events
    ///////////////////////////////////////////////////////////////////////////

    void MP_Lua_OnUnitAdded(MP_Unit* unit);
    void MP_Lua_OnBlockSelectionChanged(MP_Player player, MP_Block* block, unsigned short x, unsigned short y);
    void MP_Lua_OnBlockMetaChanged(MP_Block* block, unsigned short x, unsigned short y);
    void MP_Lua_OnBlockOwnerChanged(MP_Block* block, unsigned short x, unsigned short y);

#ifdef	__cplusplus
}
#endif

#endif	/* JOB_SCRIPT_H */
