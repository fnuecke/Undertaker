#include "room_meta.h"

META_globals(DK_RoomMeta)

/** Reset defaults on map change */
static void resetDefaults(void) {
    gMetaDefaults.canBuildOn = 0;
    gMetaDefaults.passability = 0;
    gMetaDefaults.level = DK_BLOCK_LEVEL_NORMAL;
    gMetaDefaults.isDoor = false;
    gMetaDefaults.health = 0;
}

/** New type registered */
inline static bool initMeta(DK_RoomMeta* m, const DK_RoomMeta* meta) {
    *m = *meta;

    return true;
}

/** Type override */
inline static bool updateMeta(DK_RoomMeta* m, const DK_RoomMeta* meta) {
    m->canBuildOn = meta->canBuildOn;
    m->passability = meta->passability;
    m->level = meta->level;
    m->isDoor = meta->isDoor;
    m->health = meta->health;

    return true;
}

/** Clear up data for a meta on deletion */
inline static void deleteMeta(DK_RoomMeta* m) {
}

META_impl(DK_RoomMeta, Room)
