#include <malloc.h>
#include <string.h>

#include "map.h"
#include "units_meta.h"

static DK_UnitMeta* gMetas = 0;
static unsigned int gMetaCount = 0;
static unsigned int gMetaCapacity = 0;

static DK_UnitMeta* getNextFreeEntry(void) {
    if (gMetaCount >= gMetaCapacity) {
        gMetaCapacity = gMetaCapacity * 2 + 1;
        gMetas = realloc(gMetas, gMetaCapacity * sizeof (DK_UnitMeta));
    }
    return &gMetas[gMetaCount++];
}

static void onMapChange(void) {
    gMetaCount = 0;
}

const DK_UnitMeta* DK_GetUnitMeta(unsigned int id) {
    if (id < gMetaCount) {
        return &gMetas[id];
    }
    return NULL;
}

const DK_UnitMeta* DK_GetUnitMetaByName(const char* name) {
    for (unsigned int id = 0; id < gMetaCount; ++id) {
        if (strcmp(name, gMetas[id].name) == 0) {
            return &gMetas[id];
        }
    }
    return NULL;
}

void DK_AddUnitMeta(const DK_UnitMeta* meta) {
    // Create new entry and copy data.
    DK_UnitMeta* m = getNextFreeEntry();
    *m = *meta;
}

void DK_InitUnitMeta(void) {
    DK_OnMapSizeChange(onMapChange);
}
