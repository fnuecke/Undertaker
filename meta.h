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

#include "events.h"

#define META_header(TYPE, NAME) \
const TYPE* DK_Get##NAME##Meta(unsigned int id); \
const TYPE* DK_Get##NAME##MetaByName(const char* name); \
void DK_Add##NAME##Meta(const TYPE* meta); \
void DK_Init##NAME##Meta(void);

#define META_globals(TYPE) \
static TYPE* gMetas = 0; \
static char** gMetaNames = 0; \
static unsigned int gMetaCount = 0; \
static unsigned int gMetaCapacity = 0; \

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

#define META_onMapSizeChange \
static void onMapSizeChange(void) { \
    for (unsigned int i = 0; i < gMetaCount; ++i) { \
        free(gMetaNames[i]); \
        gMetaNames[i] = NULL; \
        gMetas[i].name = NULL; \
    } \
    gMetaCount = 0; \
}

#define META_getMeta(TYPE, NAME) \
const TYPE* DK_Get##NAME##Meta(unsigned int id) { \
    if (id > 0 && id - 1 < gMetaCount) { \
        return &gMetas[id - 1]; \
    } \
    return NULL; \
}

#define META_getMetaByName(TYPE, NAME) \
const TYPE* DK_Get##NAME##MetaByName(const char* name) { \
    return findByName(name); \
}

#define META_addMeta(TYPE, NAME) \
void DK_Add##NAME##Meta(const TYPE* meta) { \
    TYPE* m; \
    if (!meta) { \
        return; \
    } \
    if ((m = findByName(meta->name))) { \
        updateMeta(m, meta); \
    } else { \
        m = getNextFreeEntry(); \
        initMeta(m, meta); \
        m->id = gMetaCount; \
        m->name = storeName(meta->name); \
    } \
}

#define META_initMeta(NAME) \
void DK_Init##NAME##Meta(void) { \
    DK_OnMapSizeChange(onMapSizeChange); \
}

#define META_impl(TYPE, NAME) \
META_globals(TYPE) \
META_getNextFreeEntry(TYPE) \
META_storeName \
META_findByName(TYPE) \
META_onMapSizeChange \
META_getMeta(TYPE, NAME) \
META_getMetaByName(TYPE, NAME) \
META_addMeta(TYPE, NAME) \
META_initMeta(NAME)

#endif	/* META_H */
