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
// Globals
///////////////////////////////////////////////////////////////////////////////

static lua_State* L = NULL;

///////////////////////////////////////////////////////////////////////////////
// Lua callbacks
///////////////////////////////////////////////////////////////////////////////

static int warnIndex(lua_State* L) {
    MP_log_warning("Trying to access undefined global '%s'.\n", luaL_checkstring(L, 2));
    return 0;
}

static int throwErrorNewIndex(lua_State* L) {
    lua_pushstring(L, "not allowed to change global state");
    return lua_error(L);
}

/**
 * Takes a table and wraps it in an immutable proxy:
 * 
 * T = input table
 * -> {} with metatable = {
 *      __index=T with metatable = {__index=error},
 *      __newindex=error
 *      }
 */
static void getImmutableProxy(lua_State* L) {
    // Create proxy table and metatable.
    lua_newtable(L);
    lua_newtable(L);

    // Make old table the index.
    lua_pushvalue(L, -3);
    lua_setfield(L, -2, "__index");

    // Disable writing (at least to unassigned indexes).
    lua_pushcfunction(L, throwErrorNewIndex);
    lua_setfield(L, -2, "__newindex");

    // Set the meta table for the value on the stack.
    lua_setmetatable(L, -2);

    // Replace original table with the proxy.
    lua_replace(L, -2);
}

int MP_Lua_Import(lua_State* L) {
    // Check if we have a valid name.
    const char* path = luaL_checkstring(L, 1);
    if (strlen(path) < 1) {
        return 0;
    }

    // Check if the import was already performed.
    luaL_getsubtable(L, LUA_REGISTRYINDEX, "_LOADED");
    lua_getfield(L, -1, path);
    if (lua_toboolean(L, -1)) {
        // Already loaded.
        lua_pop(L, 2); // pop field and table
        MP_log_info("Skipping already imported file '%s'.\n", path);
        return 0;
    }
    lua_pop(L, 1); // pop field

    // Mark loaded (either way, if we fail we won't try again).
    lua_pushboolean(L, true);
    lua_setfield(L, -2, path);
    lua_pop(L, 2); // pop table

    MP_log_info("Start parsing file '%s'.\n", path);

    // Try to parse the file.
    if (luaL_dofile(L, path) != LUA_OK) {
        // Throw up.
        return lua_error(L);
    }

    MP_log_info("Done parsing file '%s'.\n", path);

    return 0;
}

static int lua_Export(lua_State* L) {
    const char* name = luaL_checkstring(L, 1);
    luaL_checktype(L, 2, LUA_TFUNCTION);

    // Ignore empty names.
    if (strlen(name) < 1) {
        MP_log_warning("Ignoring empty name for 'export'.\n");
        return 0;
    }

    // Get the backing environment table (not the proxy!)
    lua_pushglobaltable(L); // proxy
    lua_getmetatable(L, -1); // proxy's meta
    lua_getfield(L, -1, "__index"); // actual globals table

    // Don't allow overwriting globals.
    lua_getfield(L, -1, name);
    if (lua_toboolean(L, -1)) {
        return luaL_argerror(L, 1, "globals can be overridden");
    }
    lua_pop(L, 1);

    // Get the locals table.
    lua_getmetatable(L, -1); // globals' meta
    lua_getfield(L, -1, "__index"); // locals

    // Check if there already is such a field.
    lua_getfield(L, -1, name);
    if (lua_isfunction(L, -1)) {
        MP_log_warning("Overriding already exported method '%s'.\n", name);
    }
    lua_pop(L, 1);

    // Bring function to top and register it in the globals table.
    lua_pushvalue(L, 2);
    lua_setfield(L, -2, name);

    lua_pop(L, 5); // pop the tables

    MP_log_info("Registered method named '%s'\n", name);

    return 0;
}

static int lua_BuildImportPath(lua_State* L, const char* pathfmt) {
    // Check if we have a valid name.
    const char* scriptname = luaL_checkstring(L, 1);
    if (strlen(scriptname) < 1) {
        luaL_error(L, "invalid or empty script name");
    }

    lua_pushfstring(L, pathfmt, scriptname);
    lua_remove(L, -2);

    return 0;
}

static const luaL_Reg metalib[] = {
    {"block", MP_Lua_AddBlockMeta},
    {"blockdefaults", MP_Lua_BlockMetaDefaults},
    {"job", MP_Lua_AddJobMeta},
    {"passability", MP_Lua_AddPassability},
    //lua_register(L, "room", MP_Lua_AddRoomMeta);
    {"unitdefaults", MP_Lua_UnitMetaDefaults},
    {"unit", MP_Lua_AddUnitMeta},
    {"import", lua_ImportMeta},
    {NULL, NULL}
};

static void setMetaScriptGlobals(lua_State* L) {
    setGlobals(L, metalib);
}

static const luaL_Reg abilitylib[] = {
    {"import", lua_ImportAbility},
    {"export", lua_Export},
    {NULL, NULL}
};

static void setAbilityScriptGlobals(lua_State* L) {
    setGlobals(L, abilitylib);
}

static const luaL_Reg joblib[] = {
    {"import", lua_ImportJob},
    {"export", lua_Export},
    {NULL, NULL}
};

static void setJobScriptGlobals(lua_State* L) {
    setGlobals(L, joblib);
}

static int lua_ImportMeta(lua_State* L) {
    lua_BuildImportPath(L, "data/meta/%s.lua");
    setMetaScriptGlobals();
    return MP_Lua_Import(L);
}

static int lua_ImportAbility(lua_State* L) {
    lua_BuildImportPath(L, "data/abilities/%s.lua");
    setAbilityScriptGlobals();
    MP_NewSciptLocals();
    return MP_Lua_Import(L);
}

static int lua_ImportJob(lua_State* L) {
    lua_BuildImportPath(L, "data/jobs/%s.lua");
    setJobScriptGlobals();
    MP_NewSciptLocals();
    return MP_Lua_Import(L);
}

static int lua_SetMapSize(lua_State* L) {
    unsigned int size = luaL_checkunsigned(L, 1);
    const MP_BlockMeta* meta = luaMP_checkblockmeta(L, 2);
    if (size < 1) {
        return luaL_error(L, "invalid map size");
    }
    MP_SetMapSize(size, meta);
}

static void require(lua_State* L, const char *modname, lua_CFunction openf, bool mt) {
    // Set up the library.
    luaL_requiref(L, modname, openf, 0);

    // Make it a meta table.
    if (mt) {
        // Make it its own index, then register it.
        lua_pushvalue(L, -1);
        lua_setfield(L, -2, "__index");
        lua_pushvalue(L, -1);
        lua_setfield(L, LUA_REGISTRYINDEX, modname);
    }

    // Make it immutable (get an immutable proxy for it).
    getImmutableProxy(L);

    // Register it with the table on top before the call to this method.
    lua_setfield(L, -2, modname);
}

static void setGlobals(lua_State* L, const luaL_Reg* methods) {
    // Get globals' (proxy) metatable and create new table for globals.
    lua_pushglobaltable(L);
    lua_getmetatable(L, -1);
    lua_newtable(L);

    // Stack:
    // proxy
    // meta
    // globals

    for (luaL_Reg* m = methods; m->name != NULL; ++m) {
        lua_pushcfunction(L, m->func);
        lua_setfield(L, -2, m->name);
    }

    // Give it a metatable in which we may register an index table later on.
    lua_newtable(L);
    lua_pushcfunction(L, warnIndex);
    lua_setfield(L, -2, "__index");
    lua_setmetatable(L, -2);

    // Make the new globals the index.
    lua_setfield(L, -2, "__index"); // meta.__index = globals

    lua_pop(L, 2); // pop meta and proxy
}

///////////////////////////////////////////////////////////////////////////////
// Call with stack trace
///////////////////////////////////////////////////////////////////////////////

static lua_State* getthread(lua_State* L, int* arg) {
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
// Environment switching
///////////////////////////////////////////////////////////////////////////////

lua_State* MP_InitLua(void) {
    MP_CloseLua();
    L = luaL_newstate();
    // Get globals table and create an immutable proxy for it.
    lua_pushglobaltable(L);
    getImmutableProxy(L);
    lua_rawseti(L, LUA_REGISTRYINDEX, LUA_RIDX_GLOBALS);
    return L;
}

void MP_CloseLua(void) {
    if (L) {
        lua_close(L);
        L = NULL;
    }
}

static const luaL_Reg maplib[] = {
    {"meta", lua_ImportMeta},
    {"size", lua_SetMapSize},
    {NULL, NULL}
};

void MP_SetMapScriptGlobals(void) {
    setGlobals(L, maplib);
}

void MP_SetGameScriptGlobals(void) {
    // Get the backing environment table (not the proxy!)
    lua_pushglobaltable(L); // proxy
    lua_getmetatable(L, -1); // proxy's meta
    lua_newtable(L); // new globals table

    // Stack:
    // proxy
    // meta
    // globals

    // Register libraries and types.
    require(L, LUA_BITLIBNAME, luaopen_bit32, false);
    require(L, LUA_MATHLIBNAME, luaopen_math, false);
    require(L, LUA_STRLIBNAME, luaopen_string, false);
    require(L, LUA_TABLIBNAME, luaopen_table, false);

    require(L, LUA_BLOCKLIBNAME, luaopen_block, true);
    require(L, LUA_JOBLIBNAME, luaopen_job, true);
    require(L, LUA_ROOMLIBNAME, luaopen_room, true);
    require(L, LUA_UNITLIBNAME, luaopen_unit, true);

    // Log warnings if accessing undefined fields of old table. This is to
    // notify developers that their scripts do something most likely unintended.
    lua_newtable(L);
    lua_pushcfunction(L, warnIndex);
    lua_setfield(L, -2, "__index");
    lua_setmetatable(L, -2);

    // Make new globals table the index.
    lua_setfield(L, -2, "__index"); // meta.__index = globals

    lua_pop(L, 2); // pop meta and proxy
}

void MP_NewSciptLocals(void) {
    lua_pushglobaltable(L); // proxy
    lua_getmetatable(L, -1); // proxy's meta
    lua_getfield(L, -1, "__index"); // actual globals
    lua_getmetatable(L, -1); // globals' meta

    // Stack:
    // proxy
    // proxy meta
    // globals
    // globals meta

    lua_newtable(L); // new index for globals (-> locals)
    lua_setfield(L, -2, "__index");

    lua_pop(L, 4);
}

void MP_RegisterScriptLocals(const char* type, const char* name) {
    // Get locals.
    lua_pushglobaltable(L); // proxy
    lua_getmetatable(L, -1); // proxy's meta
    lua_getfield(L, -1, "__index"); // actual globals
    lua_getmetatable(L, -1); // globals' meta
    lua_getfield(L, -1, "__index"); // actual locals

    // Stack:
    // proxy
    // proxy meta
    // globals
    // globals meta
    // locals

    // Get table in registry we use to store locals.
    lua_getfield(L, LUA_REGISTRYINDEX, type);
    if (!lua_istable(L, -1)) {
        // Doesn't exist yet, create it.
        lua_pop(L, 1);
        lua_newtable(L);
        lua_pushvalue(L, -1);
        lua_setfield(L, LUA_REGISTRYINDEX, type);
    }

    // Stack:
    // proxy
    // proxy meta
    // globals
    // globals meta
    // locals
    // type registry

    // Push locals to top, then store them.
    lua_pushvalue(L, -2);
    lua_setfield(L, -2, name);

    lua_pop(L, 6);
}

void MP_LoadScriptLocals(const char* type, const char* name) {
    // Get locals.
    lua_pushglobaltable(L); // proxy
    lua_getmetatable(L, -1); // proxy's meta
    lua_getfield(L, -1, "__index"); // actual globals
    lua_getmetatable(L, -1); // globals' meta

    // Stack:
    // proxy
    // proxy meta
    // globals
    // globals meta

    // Get table in registry we use to store locals.
    lua_getfield(L, LUA_REGISTRYINDEX, type);
    if (!lua_istable(L, -1)) {
        // Doesn't exist yet, create it.
        lua_pop(L, 1);
        lua_newtable(L);
        lua_pushvalue(L, -1);
        lua_setfield(L, LUA_REGISTRYINDEX, type);
    }
    lua_getfield(L, -1, name);

    // Stack:
    // proxy
    // proxy meta
    // globals
    // globals meta
    // type registry
    // locals

    lua_setfield(L, -3, "__index");

    lua_pop(L, 5);
}

///////////////////////////////////////////////////////////////////////////////
// Validation
///////////////////////////////////////////////////////////////////////////////

MP_Passability luaMP_checkpassability(lua_State* L, int narg) {
    const MP_Passability passability = MP_GetPassability(luaL_checkstring(L, narg));
    luaL_argcheck(L, passability != MP_PASSABILITY_NONE, narg, "invalid 'passability' value");
    return passability;
}

MP_Player luaMP_checkplayer(lua_State* L, int narg) {
    const MP_Player player = luaL_checkunsigned(L, narg);
    luaL_argcheck(L, player != MP_PLAYER_NONE && player < MP_PLAYER_COUNT, narg, "invalid 'player' value");
    return player;
}

MP_BlockLevel luaMP_checklevel(lua_State* L, int narg) {
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

    return luaL_argerror(L, narg, "invalid 'level' value");
}

const MP_BlockMeta* luaMP_checkblockmeta(lua_State* L, int narg) {
    const MP_BlockMeta* meta = MP_GetBlockMetaByName(luaL_checkstring(L, narg));
    luaL_argcheck(L, meta != NULL, narg, "invalid block type");
    return meta;
}

const MP_JobMeta* luaMP_checkjobmeta(lua_State* L, int narg) {
    const MP_JobMeta* meta = MP_GetJobMetaByName(luaL_checkstring(L, narg));
    luaL_argcheck(L, meta != NULL, narg, "invalid job type");
    return meta;
}

const MP_RoomMeta* luaMP_checkroommeta(lua_State* L, int narg) {
    const MP_RoomMeta* meta = MP_GetRoomMetaByName(luaL_checkstring(L, narg));
    luaL_argcheck(L, meta != NULL, narg, "invalid room type");
    return meta;
}

const MP_UnitMeta* luaMP_checkunitmeta(lua_State* L, int narg) {
    const MP_UnitMeta* meta = MP_GetUnitMetaByName(luaL_checkstring(L, narg));
    luaL_argcheck(L, meta != NULL, narg, "invalid unit type");
    return meta;
}

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
                    continue; \
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
