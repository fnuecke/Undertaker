#include <stdlib.h>
#include <string.h>

#include "job_type.h"
#include "script.h"
#include "script_events.h"
#include "log.h"
#include "events.h"

/** List of event handlers provided by the script */
int eventHandlers[MP_JOB_EVENT_COUNT];

struct JobCallback {
    const char* job;
    int function;
};
typedef struct JobCallback JobCallback;

#define MP_LUA_EVENT_IMPL(NAME, PUSH, ...) \
static struct { \
    JobCallback* list; \
    unsigned int length; \
    unsigned int capacity; \
} g##NAME##Callbacks = {NULL, 0, 0}; \
void MP_Lua_Add##NAME##EventListener(lua_State* L, const char* jobName) { \
    if (g##NAME##Callbacks.length >= g##NAME##Callbacks.capacity) { \
        g##NAME##Callbacks.capacity = g##NAME##Callbacks.capacity * 2 + 1; \
        g##NAME##Callbacks.list = (JobCallback*)realloc(g##NAME##Callbacks.list, g##NAME##Callbacks.capacity * sizeof(JobCallback)); \
    } \
    g##NAME##Callbacks.list[g##NAME##Callbacks.length].job = jobName; \
    lua_pushvalue(L, -1); \
    g##NAME##Callbacks.list[g##NAME##Callbacks.length].function = luaL_ref(L, LUA_REGISTRYINDEX); \
    ++g##NAME##Callbacks.length; \
} \
void MP_Lua_Remove##NAME##EventListeners(lua_State* L, const char* jobName) { \
    for (int i = g##NAME##Callbacks.length - 1; i >= 0; --i) { \
        if (strcmp(g##NAME##Callbacks.list[i].job, jobName) == 0) { \
            luaL_unref(L, LUA_REGISTRYINDEX, g##NAME##Callbacks.list[i].function); \
            --g##NAME##Callbacks.length; \
            memmove(&g##NAME##Callbacks.list[i], &g##NAME##Callbacks.list[i + 1], \
                    (g##NAME##Callbacks.length - i) * sizeof (JobCallback)); \
        } \
    } \
} \
static void on##NAME##Event(__VA_ARGS__) { \
    lua_State* L = MP_Lua(); \
    for (unsigned int i = 0; i < g##NAME##Callbacks.length; ++i) { \
        lua_rawgeti(L, LUA_REGISTRYINDEX, g##NAME##Callbacks.list[i].function); \
        if (MP_Lua_pcall(L, PUSH, 0) != LUA_OK) { \
            MP_log_error("In '" #NAME "' event handler of job '%s': %s\n", g##NAME##Callbacks.list[i].job, lua_tostring(L, -1));\
            lua_pop(L, 1); \
        } \
    } \
}

static int pushUnit(lua_State* L, MP_Unit* unit) {
    MP_Lua_PushUnit(L, unit);
    return 1;
}

static int pushPlayerAndBlock(lua_State* L, MP_Player player, MP_Block* block) {
    lua_pushinteger(L, player);
    MP_Lua_PushBlock(L, block);
    return 2;
}

static int pushBlock(lua_State* L, MP_Block* block) {
    MP_Lua_PushBlock(L, block);
    return 1;
}

MP_LUA_EVENT_IMPL(UnitAdded, pushUnit(L, unit), MP_Unit* unit)

MP_LUA_EVENT_IMPL(BlockSelectionChanged, pushPlayerAndBlock(L, player, block), MP_Player player, MP_Block* block)

MP_LUA_EVENT_IMPL(BlockTypeChanged, pushBlock(L, block), MP_Block* block)

MP_LUA_EVENT_IMPL(BlockOwnerChanged, pushBlock(L, block), MP_Block* block)

#undef MP_LUA_EVENT_IMPL

void MP_InitLuaEvents(void) {
    MP_AddUnitAddedEventListener(onUnitAddedEvent);
    MP_AddBlockSelectionChangedEventListener(onBlockSelectionChangedEvent);
    MP_AddBlockTypeChangedEventListener(onBlockTypeChangedEvent);
    MP_AddBlockOwnerChangedEventListener(onBlockOwnerChangedEvent);
}
