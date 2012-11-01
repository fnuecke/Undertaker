/* 
 * Author: fnuecke
 *
 * Created on May 7, 2012, 9:33 PM
 */

#ifndef PASSABILITY_H
#define	PASSABILITY_H

#include "lua/lua.h"
#include "lua/lauxlib.h"

#include "types.h"

#ifdef	__cplusplus
extern "C" {
#endif

    /**
     * Add a passability type. If the name is already known does nothing and
     * returns true. Fails if the name is null or empty, or there are already
     * too many types registered (max is 32). This limit exists because each
     * passability type must use an individual bit to allow bitwise operations
     * for combining and checking passability capabilities.
     * @param name the name of the passability type.
     * @return whether the type was added successfully or not.
     */
    bool MP_AddPassability(const char* name);

    /**
     * The the passability ID for a passability with the specified name.
     * @param name the name of the passability type.
     * @return the id for that type.
     */
    MP_Passability MP_GetPassability(const char* name);

    /** Lua callback for registering passability types */
    int MP_Lua_AddPassability(lua_State* L);

    /**
     * Initializes event handling for passability logic.
     */
    void MP_InitPassability(void);

#ifdef	__cplusplus
}
#endif

#endif
