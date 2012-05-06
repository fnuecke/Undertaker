#include <malloc.h>
#include <string.h>

#include "map.h"
#include "room_meta.h"

static DK_RoomMeta* gMetas = 0;
static unsigned int gMetaCount = 0;
static unsigned int gMetaCapacity = 0;

static DK_RoomMeta* getNextFreeEntry(void) {
    if (gMetaCount >= gMetaCapacity) {
        gMetaCapacity = gMetaCapacity * 2 + 1;
        gMetas = realloc(gMetas, gMetaCapacity * sizeof (DK_RoomMeta));
    }
    return &gMetas[gMetaCount++];
}

static void onMapChange(void) {
    gMetaCount = 0;
}

const DK_RoomMeta* DK_GetRoomMeta(unsigned int id) {
    if (id > 0 && id - 1 < gMetaCount) {
        return &gMetas[id - 1];
    }
    return NULL;
}

const DK_RoomMeta* DK_GetRoomMetaByName(const char* name) {
    for (unsigned int id = 0; id < gMetaCount; ++id) {
        if (strcmp(name, gMetas[id].name) == 0) {
            return &gMetas[id];
        }
    }
    return NULL;
}

void DK_AddRoomMeta(const DK_RoomMeta* meta) {
    // Create new entry and copy data.
    DK_RoomMeta* m = getNextFreeEntry();
    *m = *meta;
    m->id = gMetaCount;
}

void DK_InitRoomMeta(void) {
    DK_OnMapSizeChange(onMapChange);
}
