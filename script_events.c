#include "script.h"

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
