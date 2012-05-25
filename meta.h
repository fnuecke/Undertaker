/* 
 * File:   meta.h
 * Author: fnuecke
 *
 * Created on May 6, 2012, 3:16 PM
 */

#ifndef META_H
#define	META_H

#include "lua/lua.h"
#include "lua/lauxlib.h"

#include "types.h"

#define META_header(TYPE, NAME) \
const TYPE* MP_Get##NAME##Meta(unsigned int id); \
const TYPE* MP_Get##NAME##MetaByName(const char* name); \
unsigned int MP_Get##NAME##MetaCount(void); \
bool MP_Add##NAME##Meta(const TYPE* meta); \
int MP_Lua_##NAME##MetaDefaults(lua_State* L); \
int MP_Lua_Add##NAME##Meta(lua_State* L); \
void MP_Clear##NAME##Meta(void)

#endif	/* META_H */
