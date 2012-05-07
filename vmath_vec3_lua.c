#include "vmath.h"

#include "lua/lua.h"
#include "lua/lauxlib.h"

static int vec3_get_x(lua_State* L) {
    lua_pushnumber(L, luaL_checkvec3(L, -1)->d.x);
    return 1;
}

static int vec3_get_y(lua_State* L) {
    lua_pushnumber(L, luaL_checkvec3(L, -1)->d.y);
    return 1;
}

static int vec3_get_z(lua_State* L) {
    lua_pushnumber(L, luaL_checkvec3(L, -1)->d.z);
    return 1;
}

static int vec3_set_x(lua_State* L) {
    luaL_checkvec3(L, -1)->d.x = luaL_checknumber(L, -2);
    return 0;
}

static int vec3_set_y(lua_State* L) {
    luaL_checkvec3(L, -1)->d.y = luaL_checknumber(L, -2);
    return 0;
}

static int vec3_set_z(lua_State* L) {
    luaL_checkvec3(L, -1)->d.z = luaL_checknumber(L, -2);
    return 0;
}

static int vec3_tostring(lua_State* L) {
    vec3* v = (vec3*) lua_touserdata(L, 1);
    lua_pushfstring(L, "{%f, %f, %f}", v->d.x, v->d.y, v->d.z);
    return 1;
}

static const luaL_Reg vec3index[] = {
    {"x", vec3_get_x},
    {"y", vec3_get_y},
    {"z", vec3_get_z},
    {NULL, NULL}
};

static const luaL_Reg vec3newindex[] = {
    {"x", vec3_set_x},
    {"y", vec3_set_y},
    {"z", vec3_set_z},
    {NULL, NULL}
};

static const luaL_Reg vec3meta[] = {
    {"__tostring", vec3_tostring},
    {NULL, NULL}
};

#define MT_VEC3 "vec3"

void lua_pushvec3(lua_State *L, vec3* v) {
    lua_pushlightuserdata(L, v);
    luaL_getmetatable(L, MT_VEC3);
    lua_setmetatable(L, -2);
}

vec3* luaL_checkvec3(lua_State *L, int narg) {
    void* ud = luaL_checkudata(L, narg, MT_VEC3);
    luaL_argcheck(L, ud, 1, "'vec3' expected");
    return (vec3*) ud;
}

int luaopen_vec3(lua_State* L) {
    luaL_newmetatable(L, MT_VEC3);
    luaL_setfuncs(L, vec3meta, 0);

    lua_pushvalue(L, -1);
    lua_setfield(L, -2, "__index");
    luaL_setfuncs(L, vec3index, 0);
    lua_pop(L, 1);

    lua_newtable(L);
    lua_setfield(L, -2, "__newindex");
    luaL_setfuncs(L, vec3newindex, 0);
    lua_pop(L, 2);

    return 1;
}

#undef MT_VEC3
