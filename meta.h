/* 
 * File:   meta.h
 * Author: fnuecke
 *
 * Created on May 6, 2012, 3:16 PM
 */

#ifndef META_H
#define	META_H

#include <malloc.h>
#include <string.h>

#include "lua/lua.h"
#include "lua/lauxlib.h"

#include "config.h"
#include "events.h"
#include "log.h"
#include "types.h"

#ifdef	__cplusplus
extern "C" {
#endif

    ///////////////////////////////////////////////////////////////////////////
    // Parsing
    ///////////////////////////////////////////////////////////////////////////

    /**
     * Load meta descriptors from a file. This will also trigger loading any AI
     * jobs described in the meta file, as well as other related resources, such
     * as textures.
     * @param name the name of the meta file to load (without path / extension).
     * @return whether the file was loaded successfully.
     */
    bool MP_LoadMeta(const char* name);

#ifdef	__cplusplus
}
#endif

///////////////////////////////////////////////////////////////////////////////
// Macros
///////////////////////////////////////////////////////////////////////////////

#define META_header(TYPE, NAME) \
const TYPE* MP_Get##NAME##Meta(unsigned int id); \
const TYPE* MP_Get##NAME##MetaByName(const char* name); \
unsigned int MP_Get##NAME##MetaCount(void); \
bool MP_Add##NAME##Meta(const TYPE* meta); \
int MP_Lua_##NAME##MetaDefaults(lua_State* L); \
int MP_Lua_Add##NAME##Meta(lua_State* L); \
void MP_Clear##NAME##Meta(void)

#define META_globals(TYPE) \
static TYPE* gMetas = 0; \
static char** gMetaNames = 0; \
static unsigned int gMetaCount = 0; \
static unsigned int gMetaCapacity = 0; \
static TYPE gMetaDefaults;

#define META_getNextFreeEntry(TYPE) \
static TYPE* getNextFreeEntry(void) { \
    if (gMetaCount >= gMetaCapacity) { \
        gMetaCapacity = gMetaCapacity * 2 + 1; \
        gMetas = realloc(gMetas, gMetaCapacity * sizeof (TYPE)); \
        gMetaNames = realloc(gMetaNames, gMetaCapacity * sizeof (char*)); \
    } \
    return &gMetas[gMetaCount++]; \
}

#define META_storeName \
static const char* storeName(const char* name) { \
    gMetaNames[gMetaCount - 1] = calloc(strlen(name) + 1, sizeof (char)); \
    strcpy(gMetaNames[gMetaCount - 1], name); \
    return gMetaNames[gMetaCount - 1]; \
}

#define META_findByName(TYPE) \
static TYPE* findByName(const char* name) { \
    if (!name) { \
        return NULL; \
    } \
    for (unsigned int id = 0; id < gMetaCount; ++id) { \
        if (strcmp(name, gMetas[id].name) == 0) { \
            return &gMetas[id]; \
        } \
    } \
    return NULL; \
}

#define META_getById(TYPE, NAME) \
const TYPE* MP_Get##NAME##Meta(unsigned int id) { \
    if (id > 0 && id - 1 < gMetaCount) { \
        return &gMetas[id - 1]; \
    } \
    return NULL; \
}

#define META_getByName(TYPE, NAME) \
const TYPE* MP_Get##NAME##MetaByName(const char* name) { \
    return findByName(name); \
}

#define META_getCount(TYPE, NAME) \
unsigned int MP_Get##NAME##MetaCount(void) { \
    return gMetaCount; \
}

#define META_add(TYPE, NAME) \
bool MP_Add##NAME##Meta(const TYPE* meta) { \
    TYPE* m; \
    if (meta && meta->name) { \
        if ((m = findByName(meta->name))) { \
            if (updateMeta(m, meta)) { \
                MP_log_info("Successfully updated %s type '%s'.\n", #NAME, meta->name); \
                return true; \
            } else { \
                MP_log_error("Failed updating %s type '%s'.\n", #NAME, meta->name); \
            } \
        } else { \
            m = getNextFreeEntry(); \
            if (initMeta(m, meta)) { \
                m->id = gMetaCount; \
                m->name = storeName(meta->name); \
                MP_log_info("Successfully registered %s type '%s'.\n", #NAME, meta->name); \
                return true; \
            } else { \
                MP_log_error("Failed registering %s type '%s'.\n", #NAME, m->name); \
            } \
        } \
    } \
    return false; \
}

#define META_clear(NAME) \
void MP_Clear##NAME##Meta(void) { \
    for (unsigned int i = 0; i < gMetaCount; ++i) { \
        deleteMeta(&gMetas[i]); \
        free(gMetaNames[i]); \
        gMetaNames[i] = NULL; \
        gMetas[i].name = NULL; \
    } \
    gMetaCount = 0; \
    resetDefaults(); \
}

#define META_impl(TYPE, NAME) \
META_getNextFreeEntry(TYPE) \
META_storeName \
META_findByName(TYPE) \
META_getById(TYPE, NAME) \
META_getByName(TYPE, NAME) \
META_getCount(TYPE, NAME) \
META_add(TYPE, NAME) \
META_clear(NAME)

#endif	/* META_H */
