#include "ability.h"
#include "script.h"
#include "type_impl.h"

// <editor-fold defaultstate="collapsed" desc="ability parsers">

static void run(lua_State* L, MP_AbilityType* type, bool forDefaults) {
    luaL_checktype(L, -1, LUA_TFUNCTION);
    if (type->runMethod != LUA_REFNIL) {
        luaL_unref(L, LUA_REGISTRYINDEX, type->runMethod);
    }
    lua_pushvalue(L, -1);
    type->runMethod = luaL_ref(L, LUA_REGISTRYINDEX);
}

static MP_AbilityType* getAbilityTarget(lua_State* L, MP_AbilityType* type) {
    const MP_AbilityType* existing;

    // Get the name.
    lua_getfield(L, -1, "name");
    // Check if that type is already known (override).
    if ((existing = MP_GetAbilityTypeByName(luaL_checkstring(L, -1)))) {
        // Yes, this will be an override.
        *type = *existing;
    } else {
        // New entry.
        *type = *MP_GetAbilityTypeDefaults();
        type->info.name = lua_tostring(L, -1);
    }
    lua_pop(L, 1); // pop name

    return type;
}

TYPE_PARSER(MP_AbilityType, Ability, getAbilityTarget(L, type), MP_AbilityType* type)
// </editor-fold>

static const AbilityParserEntry abilityParsers[] = {
    {"run", run},
    {NULL, NULL}
};

int MP_LuaCallback_AddAbilityType(lua_State* L) {
    // New type, start with defaults.
    MP_AbilityType type;

    luaL_argcheck(L, lua_gettop(L) == 1, 1, "wrong number of arguments");

    // Build the block meta using the given properties.
    parseAbilityTable(L, abilityParsers, false, &type);

    // We require for at least the name and run method to be set.
    luaL_argcheck(L, type.info.name != NULL && strlen(type.info.name) > 0, 1, "invalid or no 'name'");
    luaL_argcheck(L, type.runMethod != LUA_REFNIL, 1, "'run' is required but not set");

    // All green, add the type.
    if (!MP_AddAbilityType(&type)) {
        return luaL_argerror(L, 1, "bad ability meta");
    }

    return 0;
}
