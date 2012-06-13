#include "script.h"

#include <string.h>

#include "lua/lualib.h"

#include "config.h"
#include "job.h"
#include "log.h"
#include "map.h"
#include "script_events.h"
#include "meta_block.h"
#include "meta_job.h"
#include "meta_room.h"
#include "meta_unit.h"
#include "passability.h"

///////////////////////////////////////////////////////////////////////////////
// Methods
///////////////////////////////////////////////////////////////////////////////

bool MP_RunJob(MP_Unit* unit, MP_Job* job, unsigned int* delay) {
    lua_State* L = job->meta->L;

    // Try to get the callback.
    lua_getglobal(L, "run");
    if (lua_isfunction(L, -1)) {
        // Call it with the unit that we want to execute the script for.
        luaMP_pushunit(L, unit);
        luaMP_pushjob(L, job);
        if (MP_Lua_pcall(L, 2, 2) == LUA_OK) {
            // We may have gotten a delay (in seconds) to wait, and an
            // indication of whether the job is active or not.
            float timeToWait = 0;
            bool active = false;
            if (lua_isnumber(L, -2)) {
                timeToWait = lua_tonumber(L, -2);
            }
            if (lua_isboolean(L, -1)) {
                active = lua_toboolean(L, -1);
            }
            lua_pop(L, 2); // pop results

            // Validate and return results.
            if (delay) {
                if (timeToWait < 0) {
                    *delay = 0;
                } else {
                    // OK, multiply with frame rate to get tick count.
                    *delay = (unsigned int) (MP_FRAMERATE * timeToWait);
                }
            }
            return active;
        } else {
            // Something went wrong.
            MP_log_error("In 'run' for job '%s': %s\n", job->meta->name, lua_tostring(L, -1));
        }
    } else {
        MP_log_error("'run' for job '%s' isn't a function anymore.\n", job->meta->name);
    }

    // Pop function or error message.
    lua_pop(L, 1);

    // We get here only on failure. In that case disable the run callback,
    // so we don't try this again.
    MP_DisableRunMethod(job->meta);

    return false;
}

/** Get preference value from AI script */
float MP_GetDynamicPreference(MP_Unit* unit, const MP_JobMeta* meta) {
    lua_State* L = meta->L;

    // Try to get the callback.
    lua_getglobal(L, "preference");
    if (lua_isfunction(L, -1)) {
        // Call it.
        luaMP_pushunit(L, unit);
        if (MP_Lua_pcall(L, 1, 1) == LUA_OK) {
            // OK, try to get the result as a float.
            if (lua_isnumber(L, -1)) {
                float preference = lua_tonumber(L, -1);
                lua_pop(L, 1);
                return preference;
            } else {
                MP_log_error("'preference' for job '%s' returned something that's not a number.\n", meta->name);
            }
        } else {
            // Something went wrong.
            MP_log_error("In 'preference' for job '%s': %s\n", meta->name, lua_tostring(L, -1));
        }
    } else {
        MP_log_error("'preference' for job '%s' isn't a function anymore.\n", meta->name);
    }

    // Pop function or error message or result.
    lua_pop(L, 1);

    // We get here only on failure. In that case disable the dynamic preference,
    // so we don't try this again.
    MP_DisableDynamicPreference(meta);

    return 0;
}
