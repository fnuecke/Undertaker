#include "ability.h"
#include "script.h"
#include "log.h"

bool MP_UseAbility(MP_Ability* ability) {
    lua_State* L = MP_Lua();

    // Fail if we don't have a run method.
    if (ability->type->runMethod == LUA_REFNIL) {
        MP_log_warning("Trying to run ability '%s', which has no 'run' method.\n", ability->type->info.name);
        return false;
    }

    // Skip if we're on cooldown.
    if (ability->cooldown > 0) {
        return false;
    }

    // Try to get the callback.
    lua_rawgeti(L, LUA_REGISTRYINDEX, ability->type->runMethod);
    if (lua_isfunction(L, -1)) {
        MP_Lua_PushAbility(L, ability);
        if (MP_Lua_pcall(L, 1, 1) == LUA_OK) {
            // We may have gotten a delay (in seconds) to wait (cooldown).
            bool success = false;
            if (lua_isnumber(L, -1)) {
                float cooldown = lua_tonumber(L, -1);
                if (cooldown > 0) {
                    // OK, multiply with frame rate to get tick count.
                    ability->cooldown = (unsigned int) (MP_FRAMERATE * cooldown);
                }
                success = true;
            } else if (lua_isboolean(L, -1)) {
                success = lua_toboolean(L, -1);
            } else {
                MP_log_warning("In 'run' for ability '%s': returned invalid result.\n", ability->type->info.name);
            }
            lua_pop(L, 2); // pop result

            return success;
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

    return false;
}