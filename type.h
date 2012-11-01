/* 
 * Author: fnuecke
 *
 * Created on May 6, 2012, 3:16 PM
 */

#ifndef TYPE_H
#define	TYPE_H

#include "lua/lua.h"
#include "lua/lauxlib.h"

#include "types.h"

#ifdef	__cplusplus
extern "C" {
#endif

    struct MP_Type {
        /** The ID of this type */
        unsigned int id;

        /** The name of this type */
        const char* name;
    };

#ifdef	__cplusplus
}
#endif

#define TYPE_HEADER(TYPE, NAME) \
const TYPE* MP_Get##NAME##Type(unsigned int id); \
TYPE* MP_Get##NAME##TypeDefaults(void); \
const TYPE* MP_Get##NAME##TypeByName(const char* name); \
unsigned int MP_Get##NAME##TypeCount(void); \
bool MP_Add##NAME##Type(const TYPE* meta); \
int MP_LuaCallback_Set##NAME##TypeDefaults(lua_State* L); \
int MP_LuaCallback_Add##NAME##Type(lua_State* L); \
void MP_Clear##NAME##Types(void)

#endif
