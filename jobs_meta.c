#include "jobs_meta.h"

META_globals(DK_JobMeta)

static bool loadAiFile(lua_State* L, const char* jobName) {
    char filename[128] = {0};
    sprintf(filename, "data/ai/%s.lua", jobName);
    if (luaL_dofile(L, filename) != LUA_OK) {
        // Something went wrong.
        return false;
    }
    return true;
}

/** Reset defaults on map change */
static void resetDefaults(void) {
    gMetaDefaults.hasRunMethod = false;
}

/** New type registered */
static bool initMeta(DK_JobMeta* m, const DK_JobMeta* meta) {
    *m = *meta;
    m->L = luaL_newstate();
    if (!loadAiFile(m->L, m->name)) {
        return false;
    }
    // Check script capabilities.

    return true;
}

/** Type override */
static bool updateMeta(DK_JobMeta* m, const DK_JobMeta* meta) {
    return true;
}

META_impl(DK_JobMeta, Job)
