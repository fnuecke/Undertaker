/* 
 * File:   script_context.h
 * Author: fnuecke
 *
 * Created on May 25, 2012, 9:19 PM
 */

#ifndef SCRIPT_CONTEXT_H
#define	SCRIPT_CONTEXT_H

#include "lua/lua.h"
#include "lua/lauxlib.h"

#ifdef	__cplusplus
extern "C" {
#endif

    ///////////////////////////////////////////////////////////////////////////
    // Initialization
    ///////////////////////////////////////////////////////////////////////////

    /**
     * Setup basic globals structure for a script VM. Shuts down a previous VM.
     */
    lua_State* MP_InitLua(void);

    /**
     * Shut down the Lua VM.
     */
    void MP_CloseLua(lua_State* L);

    ///////////////////////////////////////////////////////////////////////////
    // Environment switching
    ///////////////////////////////////////////////////////////////////////////

    /**
     * Used for parsing map basics. Makes available meta, size and similar setters.
     */
    void MP_SetLoadingScriptGlobals(lua_State* L);

    /**
     * Used for in-game script execution. Makes available interface methods for
     * querying/modifying game state.
     */
    void MP_SetGameScriptGlobals(lua_State* L);

    /**
     * Stores a local environment under the specified name for later retrieval.
     * This will not invalidate the current local environment.
     */
    void MP_RegisterScriptLocals(const char* type, const char* name);

    /**
     * Restore a local environment previously stored.
     */
    void MP_LoadScriptLocals(const char* type, const char* name);

#ifdef	__cplusplus
}
#endif

#endif	/* SCRIPT_CONTEXT_H */
