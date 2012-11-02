#include <assert.h>

#include "ability.h"
#include "script.h"
#include "log.h"

float MP_UseAbility(MP_Ability* ability) {
    lua_State* L = MP_Lua();

    assert(ability);

    // Fail if we don't have a run method.
    if (ability->type->runMethod == LUA_REFNIL) {
        MP_log_warning("Trying to run ability '%s', which has no 'run' method.\n", ability->type->info.name);
        return -1;
    }

    // Skip if we're on cooldown.
    if (ability->cooldown > 0) {
        return ability->cooldown / (float) MP_FRAMERATE;
    }

    // Try to get the callback.
    lua_rawgeti(L, LUA_REGISTRYINDEX, ability->type->runMethod);
    if (lua_isfunction(L, -1)) {
        MP_Lua_PushAbility(L, ability);
        if (MP_Lua_pcall(L, 1, 1) == LUA_OK) {
            float cooldown = luaL_checknumber(L, -1);
            if (cooldown > 0) {
                // OK, multiply with frame rate to get tick count.
                ability->cooldown = (unsigned int) (MP_FRAMERATE * cooldown);
            }
            lua_pop(L, 1); // pop result

            return cooldown;
        } else {
            // Something went wrong.
            MP_log_error("In 'run' for ability '%s': %s\n", ability->type->info.name, lua_tostring(L, -1));
        }
    } else {
        MP_log_error("'run' for ability '%s' isn't a function anymore.\n", ability->type->info.name);
    }

    // Pop function or error message.
    lua_pop(L, 1);

    // We get here only on failure. In that case disable the run callback,
    // so we don't try this again.
    MP_DisableAbilityRunMethod(ability->type);

    return -1;
}
