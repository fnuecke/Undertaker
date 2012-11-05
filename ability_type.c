#include <assert.h>

#include "ability_type.h"
#include "script.h"
#include "type_impl.h"

///////////////////////////////////////////////////////////////////////////////
// Constants and globals
///////////////////////////////////////////////////////////////////////////////

TYPE_GLOBALS(MP_AbilityType)

///////////////////////////////////////////////////////////////////////////////
// Init / Update / Teardown
///////////////////////////////////////////////////////////////////////////////

/** Reset defaults on map change */
static void resetDefaults(void) {
    gTypeDefaults.runMethod = LUA_REFNIL;
}

/** New type registered */
inline static bool initType(MP_AbilityType* stored, const MP_AbilityType* input) {
    *stored = *input;

    return true;
}

/** Type override */
inline static bool updateType(MP_AbilityType* stored, const MP_AbilityType* input) {
    if (stored->runMethod != LUA_REFNIL) {
        luaL_unref(MP_Lua(), LUA_REGISTRYINDEX, stored->runMethod);
    }
    stored->runMethod = input->runMethod;

    return true;
}

/** Clear up data for a meta on deletion */
inline static void deleteType(MP_AbilityType* type) {
}

TYPE_IMPL(MP_AbilityType, Ability)

///////////////////////////////////////////////////////////////////////////////
// Accessors
///////////////////////////////////////////////////////////////////////////////

void MP_DisableAbilityRunMethod(const MP_AbilityType* type) {
    assert(type);
    assert(type->info.id > 0 && type->info.id - 1 < gTypeCount);
    {
        MP_AbilityType* nonConstType = &gTypes[type->info.id - 1];
        if (nonConstType->runMethod != LUA_REFNIL) {
            luaL_unref(MP_Lua(), LUA_REGISTRYINDEX, nonConstType->runMethod);
            nonConstType->runMethod = LUA_REFNIL;
            MP_log_info("Disabling 'run' for ability '%s'.\n", type->info.name);
        }
    }
}
