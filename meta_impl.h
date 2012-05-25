/* 
 * File:   meta_impl.h
 * Author: fnuecke
 *
 * Created on May 18, 2012, 12:04 AM
 */

#ifndef META_IMPL_H
#define	META_IMPL_H

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

#define META_globals(TYPE) \
static TYPE** gMetas = 0; \
static char** gMetaNames = 0; \
static unsigned int gMetaCount = 0; \
static TYPE gMetaDefaults;

#define META_getNextFreeEntry(TYPE) \
static TYPE* getNextFreeEntry(void) { \
    ++gMetaCount; \
    gMetas = realloc(gMetas, gMetaCount * sizeof (TYPE*)); \
    gMetaNames = realloc(gMetaNames, gMetaCount * sizeof (char*)); \
    if (!(gMetas[gMetaCount - 1] = calloc(1, sizeof(TYPE)))) { \
        MP_log_fatal("Out of memory while allocating meta data.\n"); \
    } \
    gMetaNames[gMetaCount - 1] = NULL; \
    return gMetas[gMetaCount - 1]; \
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
        if (strcmp(name, gMetas[id]->name) == 0) { \
            return gMetas[id]; \
        } \
    } \
    return NULL; \
}

#define META_getById(TYPE, NAME) \
const TYPE* MP_Get##NAME##Meta(unsigned int id) { \
    if (id > 0 && id - 1 < gMetaCount) { \
        return gMetas[id - 1]; \
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
        deleteMeta(gMetas[i]); \
        free(gMetas[i]); \
        gMetas[i] = NULL; \
        free(gMetaNames[i]); \
        gMetaNames[i] = NULL; \
    } \
    free(gMetas); \
    gMetas = 0; \
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


#define META_parser(TYPE, NAME, GETTER, WARN_ON_NAME, ...) \
typedef void(*NAME##Parser)(lua_State*, TYPE*); \
typedef struct NAME##ParserEntry { \
    const char* name; \
    NAME##Parser parser; \
} NAME##ParserEntry; \
static void parse##NAME##Table(lua_State* L, const NAME##ParserEntry* parsers, __VA_ARGS__) { \
    TYPE* target = NULL; \
    luaL_argcheck(L, lua_istable(L, -1), 1, "'table' expected"); \
    GETTER \
    lua_pushnil(L); \
    while (lua_next(L, -2)) { \
        const char* key; \
        luaL_argcheck(L, lua_type(L, -2) == LUA_TSTRING, 1, "keys must be strings"); \
        key = lua_tostring(L, -2); \
        if (strcmp(key, "name") == 0) { \
            if (WARN_ON_NAME) {\
                MP_log_warning("'name' invalid in this context, ignored.\n"); \
            } \
        } else { \
            const NAME##ParserEntry* parser = parsers; \
            for (; parser->name != NULL; ++parser) { \
                if (strcmp(key, parser->name) == 0) { \
                    parser->parser(L, target); \
                    break; \
                } \
            } \
            if (parser->name == NULL) { \
                luaL_argerror(L, 1, "unknown key"); \
            } \
        } \
        lua_pop(L, 1); \
    } \
}

#ifdef	__cplusplus
}
#endif

#endif	/* META_IMPL_H */

