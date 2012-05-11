#include "job_script.h"

#include <string.h>

#include "job.h"
#include "meta_block.h"
#include "meta_job.h"
#include "meta_room.h"
#include "meta_unit.h"
#include "passability.h"

///////////////////////////////////////////////////////////////////////////////
// Call with stack trace
///////////////////////////////////////////////////////////////////////////////

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

///////////////////////////////////////////////////////////////////////////////
// Validation
///////////////////////////////////////////////////////////////////////////////

MP_Passability luaMP_checkpassability(lua_State* L, int narg, int errarg) {
    const MP_Passability passability = MP_GetPassability(luaL_checkstring(L, narg));
    luaL_argcheck(L, passability != MP_PASSABILITY_NONE, errarg, "invalid 'passability' value");
    return passability;
}

MP_Player luaMP_checkplayer(lua_State* L, int narg, int errarg) {
    const MP_Player player = luaL_checkunsigned(L, narg);
    luaL_argcheck(L, player != MP_PLAYER_NONE && player < MP_PLAYER_COUNT, errarg, "invalid 'player' value");
    return player;
}

MP_BlockLevel luaMP_checklevel(lua_State* L, int narg, int errarg) {
    const char* level = luaL_checkstring(L, narg);
    if (strcmp(level, "pit") == 0) {
        return MP_BLOCK_LEVEL_PIT;
    } else if (strcmp(level, "lowered") == 0) {
        return MP_BLOCK_LEVEL_LOWERED;
    } else if (strcmp(level, "normal") == 0) {
        return MP_BLOCK_LEVEL_NORMAL;
    } else if (strcmp(level, "high") == 0) {
        return MP_BLOCK_LEVEL_HIGH;
    }

    return luaL_argerror(L, errarg, "invalid 'level' value");
}

const MP_BlockMeta* luaMP_checkblockmeta(lua_State* L, int narg, int errarg) {
    const MP_BlockMeta* meta = MP_GetBlockMetaByName(luaL_checkstring(L, narg));
    luaL_argcheck(L, meta != NULL, errarg, "invalid block type");
    return meta;
}

const MP_JobMeta* luaMP_checkjobmeta(lua_State* L, int narg, int errarg) {
    const MP_JobMeta* meta = MP_GetJobMetaByName(luaL_checkstring(L, narg));
    luaL_argcheck(L, meta != NULL, errarg, "invalid job type");
    return meta;
}

const MP_RoomMeta* luaMP_checkroommeta(lua_State* L, int narg, int errarg) {
    const MP_RoomMeta* meta = MP_GetRoomMetaByName(luaL_checkstring(L, narg));
    luaL_argcheck(L, meta != NULL, errarg, "invalid room type");
    return meta;
}

const MP_UnitMeta* luaMP_checkunitmeta(lua_State* L, int narg, int errarg) {
    const MP_UnitMeta* meta = MP_GetUnitMetaByName(luaL_checkstring(L, narg));
    luaL_argcheck(L, meta != NULL, errarg, "invalid unit type");
    return meta;
}

///////////////////////////////////////////////////////////////////////////////
// Run
///////////////////////////////////////////////////////////////////////////////

unsigned int MP_Lua_RunJob(MP_Unit* unit, MP_Job* job) {
    lua_State* L = job->meta->L;

    // Try to get the callback.
    lua_getglobal(L, "run");
    if (lua_isfunction(L, -1)) {
        // Call it with the unit that we want to execute the script for.
        luaMP_pushunit(L, unit);
        luaMP_pushjob(L, job);
        if (MP_Lua_pcall(L, 2, 1) == LUA_OK) {
            // We should have gotten a delay (in seconds) to wait.
            float delay = 0;
            if (lua_isnumber(L, -1)) {
                delay = lua_tonumber(L, -1);
            }
            lua_pop(L, 1);

            // Validate.
            if (delay < 0) {
                return 0;
            }

            // OK, multiply with frame rate to get tick count.
            return (unsigned int) (MP_FRAMERATE * delay);
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

    return 0;
}

///////////////////////////////////////////////////////////////////////////////
// Events
///////////////////////////////////////////////////////////////////////////////

#define FIRE_EVENT(event, push, nargs) \
{ \
    const char* eventName = JOB_EVENT_NAME[event]; \
    for (unsigned int metaId = 0; metaId < MP_GetJobMetaCount(); ++metaId) { \
        const MP_JobMeta* meta = MP_GetJobMeta(metaId + 1); \
        if (meta->handlesEvent[event]) { \
            lua_State* L = meta->L; \
            lua_getglobal(L, eventName); \
            if (lua_isfunction(L, -1)) { \
                push \
                if (MP_Lua_pcall(L, nargs, 0) == LUA_OK) { \
                    return; \
                } else { \
                    MP_log_error("In '%s' for job '%s': %s\n", eventName, meta->name, lua_tostring(L, -1)); \
                } \
            } else { \
                MP_log_error("'%s' for job '%s' isn't a function anymore.\n", eventName, meta->name); \
            } \
            lua_pop(L, 1); \
            MP_DisableJobEvent(meta, event); \
        } \
    } \
}

void MP_Lua_OnUnitAdded(MP_Unit* unit) {
    FIRE_EVENT(MP_JOB_EVENT_UNIT_ADDED,{
        luaMP_pushunit(L, unit);
    }, 1);
}

void MP_Lua_OnBlockSelectionChanged(MP_Player player, MP_Block* block, unsigned short x, unsigned short y) {
    FIRE_EVENT(MP_JOB_EVENT_BLOCK_SELECTION_CHANGED,{
        lua_pushunsigned(L, player);
        luaMP_pushblock(L, block);
        lua_pushunsigned(L, x);
        lua_pushunsigned(L, y);
    }, 4);
}

void MP_Lua_OnBlockMetaChanged(MP_Block* block, unsigned short x, unsigned short y) {
    FIRE_EVENT(MP_JOB_EVENT_BLOCK_META_CHANGED,{
        luaMP_pushblock(L, block);
        lua_pushunsigned(L, x);
        lua_pushunsigned(L, y);
    }, 3);
}

void MP_Lua_OnBlockOwnerChanged(MP_Block* block, unsigned short x, unsigned short y) {
    FIRE_EVENT(MP_JOB_EVENT_BLOCK_OWNER_CHANGED,{
        luaMP_pushblock(L, block);
        lua_pushunsigned(L, x);
        lua_pushunsigned(L, y);
    }, 3);
}
