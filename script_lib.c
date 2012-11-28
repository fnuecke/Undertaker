#include "ability_type.h"
#include "block_type.h"
#include "job_type.h"
#include "room_type.h"
#include "script.h"
#include "unit_type.h"

#define MP_LUA_LIBRARY_IMPL(NAME, LIBNAME) \
void MP_Lua_Push##NAME(lua_State* L, MP_##NAME* value) { \
    if (value) { \
        MP_##NAME** ud = (MP_##NAME**) lua_newuserdata(L, sizeof (MP_##NAME*)); \
        *ud = value; \
        luaL_setmetatable(L, LIBNAME); \
    } else { \
        lua_pushnil(L); \
    } \
} \
bool MP_Lua_Is##NAME(lua_State* L, int narg) { \
    return luaL_testudata(L, narg, LIBNAME) != NULL; \
} \
MP_##NAME* MP_Lua_To##NAME(lua_State* L, int narg) { \
    return *(MP_##NAME**) lua_touserdata(L, narg); \
} \
MP_##NAME* MP_Lua_Check##NAME(lua_State* L, int narg) { \
    void* ud = luaL_checkudata(L, narg, LIBNAME); \
    luaL_argcheck(L, ud != NULL, narg, "'" LIBNAME "' expected"); \
    return *(MP_##NAME**) ud; \
} \
const MP_##NAME##Type* MP_Lua_Check##NAME##Type(lua_State* L, int narg) { \
    const MP_##NAME##Type* type = MP_Get##NAME##TypeByName(luaL_checkstring(L, narg)); \
    luaL_argcheck(L, type != NULL, narg, "invalid '" LIBNAME "' type"); \
    return type; \
}

MP_LUA_LIBRARY_IMPL(Ability, LUA_ABILITYLIBNAME)
MP_LUA_LIBRARY_IMPL(Job, LUA_JOBLIBNAME)
MP_LUA_LIBRARY_IMPL(Block, LUA_BLOCKLIBNAME)
MP_LUA_LIBRARY_IMPL(Unit, LUA_UNITLIBNAME)
MP_LUA_LIBRARY_IMPL(Room, LUA_ROOMLIBNAME)

#undef MP_LUA_LIBRARY_IMPL
