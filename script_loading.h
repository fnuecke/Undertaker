/* 
 * File:   script_parsing.h
 * Author: fnuecke
 *
 * Created on May 25, 2012, 9:08 PM
 */

#ifndef SCRIPT_PARSING_H
#define	SCRIPT_PARSING_H

#include "lua/lua.h"
#include "lua/lauxlib.h"

#ifdef	__cplusplus
extern "C" {
#endif

    /**
     * Register a function in the local environment, where the function name is
     * at stack index index 1 and the function itself at index 2.
     */
    int MP_Lua_Export(lua_State* L);

    /**
     * Load the script file specified by the path on top of the stack.
     */
    int MP_Lua_Import(lua_State* L);
    int MP_Lua_ImportMeta(lua_State* L);

#ifdef	__cplusplus
}
#endif

#endif	/* SCRIPT_PARSING_H */
