#include "vmath.h"

#include "lua/lua.h"
#include "lua/lauxlib.h"

static int vec2_get_x(lua_State* L) {
    lua_pushnumber(L, luaL_checkvec2(L, -1)->d.x);
    return 1;
}

static int vec2_get_y(lua_State* L) {
    lua_pushnumber(L, luaL_checkvec2(L, -1)->d.y);
    return 1;
}

static int vec2_set_x(lua_State* L) {
    vec2* v = luaL_checkvec2(L, -1);
    v->d.x = luaL_checknumber(L, -2);
    return 0;
}

static int vec2_set_y(lua_State* L) {
    vec2* v = luaL_checkvec2(L, -1);
    v->d.y = luaL_checknumber(L, -2);
    return 0;
}

static int vec2_tostring(lua_State* L) {
    vec2* v = (vec2*) lua_touserdata(L, 1);
    lua_pushfstring(L, "{%f, %f}", v->d.x, v->d.y);
    return 1;
}

static const luaL_Reg vec2index[] = {
    {"x", vec2_get_x},
    {"y", vec2_get_y},
    {NULL, NULL}
};

static const luaL_Reg vec2newindex[] = {
    {"x", vec2_set_x},
    {"y", vec2_set_y},
    {NULL, NULL}
};

static const luaL_Reg vec2meta[] = {
    {"__tostring", vec2_tostring},
    {NULL, NULL}
};

#define MT_VEC2 "vec2"

void lua_pushvec2(lua_State *L, vec2* v) {
    lua_pushlightuserdata(L, v);
    luaL_getmetatable(L, MT_VEC2);
    lua_setmetatable(L, -2);
}

vec2* luaL_checkvec2(lua_State *L, int narg) {
    void* ud = luaL_checkudata(L, narg, MT_VEC2);
    luaL_argcheck(L, ud, 1, "'vec2' expected");
    return (vec2*) ud;
}

int luaopen_vec2(lua_State* L) {
    luaL_newmetatable(L, MT_VEC2);
    luaL_setfuncs(L, vec2meta, 0);

    lua_pushvalue(L, -1);
    lua_setfield(L, -2, "__index");
    luaL_setfuncs(L, vec2index, 0);
    lua_pop(L, 1);

    lua_newtable(L);
    lua_setfield(L, -2, "__newindex");
    luaL_setfuncs(L, vec2newindex, 0);
    lua_pop(L, 2);

    return 1;
}

#undef MT_VEC2
