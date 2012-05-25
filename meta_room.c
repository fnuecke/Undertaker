#include "meta_room.h"

#include "meta_impl.h"

META_globals(MP_RoomMeta)

/** Reset defaults on map change */
static void resetDefaults(void) {
    gMetaDefaults.canBuildOn = 0;
    gMetaDefaults.passability = 0;
    gMetaDefaults.level = MP_BLOCK_LEVEL_NORMAL;
    gMetaDefaults.isDoor = false;
    gMetaDefaults.health = 0;
}

/** New type registered */
inline static bool initMeta(MP_RoomMeta* m, const MP_RoomMeta* meta) {
    *m = *meta;

    return true;
}

/** Type override */
inline static bool updateMeta(MP_RoomMeta* m, const MP_RoomMeta* meta) {
    m->canBuildOn = meta->canBuildOn;
    m->passability = meta->passability;
    m->level = meta->level;
    m->isDoor = meta->isDoor;
    m->health = meta->health;

    return true;
}

/** Clear up data for a meta on deletion */
inline static void deleteMeta(MP_RoomMeta* m) {
}

META_impl(MP_RoomMeta, Room)
