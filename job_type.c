#include <assert.h>

#include "job_type.h"
#include "script.h"
#include "type_impl.h"

///////////////////////////////////////////////////////////////////////////////
// Constants and globals
///////////////////////////////////////////////////////////////////////////////

TYPE_GLOBALS(MP_JobType)

///////////////////////////////////////////////////////////////////////////////
// Init / Update / Teardown
///////////////////////////////////////////////////////////////////////////////

/** Reset defaults on map change */
static void resetDefaults(void) {
    gTypeDefaults.runMethod = LUA_REFNIL;
    gTypeDefaults.dynamicPreference = LUA_REFNIL;
}

/** New type registered */
inline static bool initType(MP_JobType* stored, const MP_JobType* input) {
    *stored = *input;

    return true;
}

/** Type override */
inline static bool updateType(MP_JobType* stored, const MP_JobType* input) {
    if (stored->runMethod != LUA_REFNIL) {
        luaL_unref(MP_Lua(), LUA_REGISTRYINDEX, stored->runMethod);
    }
    stored->runMethod = input->runMethod;
    if (stored->dynamicPreference != LUA_REFNIL) {
        luaL_unref(MP_Lua(), LUA_REGISTRYINDEX, stored->dynamicPreference);
    }
    stored->dynamicPreference = input->dynamicPreference;
    
    return true;
}

/** Clear up data for a meta on deletion */
inline static void deleteType(MP_JobType* type) {
}

TYPE_IMPL(MP_JobType, Job)

///////////////////////////////////////////////////////////////////////////////
// Accessors
///////////////////////////////////////////////////////////////////////////////

void MP_DisableDynamicPreference(const MP_JobType* type) {
    assert(type);
    assert(type->info.id > 0 && type->info.id - 1 < gTypeCount);
    {
        MP_JobType* nonConstType = &gTypes[type->info.id - 1];
        if (nonConstType->dynamicPreference != LUA_REFNIL) {
            luaL_unref(MP_Lua(), LUA_REGISTRYINDEX, nonConstType->dynamicPreference);
            nonConstType->dynamicPreference = LUA_REFNIL;
            MP_log_info("Disabling 'preference' for job '%s'.\n", type->info.name);
        }
    }
}

void MP_DisableJobRunMethod(const MP_JobType* type) {
    assert(type);
    assert(type->info.id > 0 && type->info.id - 1 < gTypeCount);
    {
        MP_JobType* nonConstType = &gTypes[type->info.id - 1];
        if (nonConstType->runMethod != LUA_REFNIL) {
            luaL_unref(MP_Lua(), LUA_REGISTRYINDEX, nonConstType->runMethod);
            nonConstType->runMethod = LUA_REFNIL;
            MP_log_info("Disabling 'run' for job '%s'.\n", type->info.name);
        }
    }
}
