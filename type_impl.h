/* 
 * Author: fnuecke
 *
 * Created on May 18, 2012, 12:04 AM
 */

#ifndef TYPE_IMPL_H
#define	TYPE_IMPL_H

#include <assert.h>
#include <malloc.h>
#include <string.h>

#include "lua/lua.h"
#include "lua/lauxlib.h"

#include "config.h"
#include "events.h"
#include "log.h"
#include "types.h"
#include "type.h"

#ifdef	__cplusplus
extern "C" {
#endif

#define TYPE_GLOBALS(TYPE) \
static TYPE gTypes[MP_TYPE_ID_MAX]; \
static char* gTypeNames[MP_TYPE_ID_MAX]; \
static unsigned int gTypeCount = 0; \
static TYPE gTypeDefaults;

#define TYPE_GET_NEXT_FREE_ENTRY(TYPE) \
static TYPE* getNextFreeEntry(void) { \
    assert(gTypeCount < MP_TYPE_ID_MAX); \
    memset(&gTypes[gTypeCount], 0, sizeof(TYPE)); \
    gTypeNames[gTypeCount] = NULL; \
    return &gTypes[gTypeCount++]; \
}

#define TYPE_STORE_NAME \
static const char* storeName(const char* name, unsigned int index) { \
    gTypeNames[index] = calloc(strlen(name) + 1, sizeof (char)); \
    strcpy(gTypeNames[index], name); \
    return gTypeNames[index]; \
}

#define TYPE_FIND_BY_NAME(TYPE) \
static TYPE* findByName(const char* name) { \
    for (unsigned int id = 0; id < gTypeCount; ++id) { \
        if (strcmp(name, gTypes[id].info.name) == 0) { \
            return &gTypes[id]; \
        } \
    } \
    return NULL; \
}

#define TYPE_GET_DEFAULTS(TYPE, NAME) \
TYPE* MP_Get##NAME##TypeDefaults(void) { \
    return &gTypeDefaults; \
}

#define TYPE_GET_BY_ID(TYPE, NAME) \
const TYPE* MP_Get##NAME##TypeById(unsigned int id) { \
    assert(id > 0 && id <= gTypeCount); \
    return &gTypes[id - 1]; \
}

#define TYPE_GET_BY_NAME(TYPE, NAME) \
const TYPE* MP_Get##NAME##TypeByName(const char* name) { \
    assert(name); \
    return findByName(name); \
}

#define TYPE_GET_COUNT(TYPE, NAME) \
unsigned int MP_Get##NAME##TypeCount(void) { \
    return gTypeCount; \
}

#define TYPE_ADD(TYPE, NAME) \
bool MP_Add##NAME##Type(const TYPE* type) { \
    TYPE* t; \
    assert(type); \
    assert(type->info.name); \
    if (gTypeCount > MP_TYPE_ID_MAX) { \
        MP_log_warning("Too many " #NAME " types, ignoring type '%s'.\n", type->info.name); \
    } else if ((t = findByName(type->info.name))) { \
        if (updateType(t, type)) { \
            MP_log_info("Successfully updated " #NAME " type '%s'.\n", type->info.name); \
            return true; \
        } else { \
            MP_log_error("Failed updating " #NAME " type '%s'.\n", type->info.name); \
        } \
    } else { \
        TYPE n; \
        if (initType(&n, type)) { \
            t = getNextFreeEntry(); \
            *t = n; \
            t->info.id = gTypeCount; \
            t->info.name = storeName(type->info.name, gTypeCount - 1); \
            MP_log_info("Successfully registered " #NAME " type '%s'.\n", type->info.name); \
            return true; \
        } else { \
            MP_log_error("Failed registering " #NAME " type '%s'.\n", type->info.name); \
        } \
    } \
    return false; \
}

#define TYPE_CLEAR(NAME) \
void MP_Clear##NAME##Types(void) { \
    for (unsigned int i = 0; i < gTypeCount; ++i) { \
        deleteType(&gTypes[i]); \
        free(gTypeNames[i]); \
    } \
    gTypeCount = 0; \
    resetDefaults(); \
}

#define TYPE_IMPL(TYPE, NAME) \
TYPE_GET_NEXT_FREE_ENTRY(TYPE) \
TYPE_STORE_NAME \
TYPE_FIND_BY_NAME(TYPE) \
TYPE_GET_DEFAULTS(TYPE, NAME) \
TYPE_GET_BY_ID(TYPE, NAME) \
TYPE_GET_BY_NAME(TYPE, NAME) \
TYPE_GET_COUNT(TYPE, NAME) \
TYPE_ADD(TYPE, NAME) \
TYPE_CLEAR(NAME)

    /**
     * Utility macro for defining table parsers.
     * 
     * This takes the type to parse to (i.e. the struct into which parsed data is
     * written to), the name to use for parser types and method, as well as wether
     * to warn if the key "name" appears in the table or not.
     * 
     * A parser method will be called if a key with it's associated name is found.
     * The value will be on top of the stack, but the stack will contain lots of
     * other stuff as well, so make sure not to corrupt it by not editing it below
     * initial index -1 and leaving it at the same length as it was when the parser
     * method was called.
     */
#define TYPE_PARSER(TYPE, NAME, GETTER, ...) \
typedef void(*NAME##Parser)(lua_State*, TYPE*, bool); \
typedef struct NAME##ParserEntry { \
    const char* name; \
    NAME##Parser parser; \
} NAME##ParserEntry; \
static void parse##NAME##Table(lua_State* L, const NAME##ParserEntry* parsers, bool forDefaults, __VA_ARGS__) { \
    TYPE* target = NULL; \
    luaL_checktype(L, -1, LUA_TTABLE); \
    target = GETTER; \
    lua_pushnil(L); \
    while (lua_next(L, -2)) { \
        const char* key; \
        if (lua_type(L, -2) == LUA_TSTRING) { \
            key = lua_tostring(L, -2); \
            if (strcmp(key, "name") == 0) { \
                if (forDefaults) {\
                    luaL_where(L, 1); \
                    MP_log_warning("%s: 'name' invalid in defaults, ignored.\n", lua_tostring(L, -1)); \
                    lua_pop(L, 1); \
                } \
            } else { \
                const NAME##ParserEntry* parser = parsers; \
                while (parser->name != NULL) { \
                    if (strcmp(key, parser->name) == 0) { \
                        parser->parser(L, target, forDefaults); \
                        break; \
                    } \
                    ++parser; \
                } \
                if (parser->name == NULL) { \
                    luaL_where(L, 1); \
                    MP_log_warning("%s: ignoring unknown key '%s'.\n", lua_tostring(L, -1), key); \
                    lua_pop(L, 1); \
                } \
            } \
        } else { \
            luaL_where(L, 1); \
            MP_log_warning("%s: ignoring non-string key.\n", lua_tostring(L, -1)); \
            lua_pop(L, 1); \
        } \
        lua_pop(L, 1); \
    } \
}

#ifdef	__cplusplus
}
#endif

#endif
