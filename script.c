#include "script.h"

static lua_State *getthread(lua_State *L, int *arg) {
    if (lua_isthread(L, 1)) {
        *arg = 1;
        return lua_tothread(L, 1);
    } else {
        *arg = 0;
        return L;
    }
}

static int traceback(lua_State *L) {
    int arg;
    lua_State *L1 = getthread(L, &arg);
    const char *msg = lua_tostring(L, arg + 1);
    if (msg == NULL && !lua_isnoneornil(L, arg + 1)) /* non-string 'msg'? */
        lua_pushvalue(L, arg + 1); /* return it untouched */
    else {
        int level = luaL_optint(L, arg + 2, (L == L1) ? 1 : 0);
        luaL_traceback(L, L1, msg, level);
    }
    return 1;
}

int MP_Lua_pcall(lua_State* L, int nargs, int nresults) {
    int result;

    // Insert traceback method before actually called function.
    lua_pushcfunction(L, traceback);
    lua_insert(L, -(nargs + 2));

    // Run and check result to know where to remove traceback method.
    if ((result = lua_pcall(L, nargs, nresults, -(nargs + 2))) == LUA_OK) {
        // Success, there's nresult entries on the stack.
        lua_remove(L, -(nresults + 1));
    } else {
        // Error, means there's one string on the stack.
        lua_remove(L, -2);
    }
    return result;
}
