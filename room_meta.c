#include "room_meta.h"

META_globals(DK_RoomMeta)

/** Reset defaults on map change */
static void resetDefaults(void) {
}

/** New type registered */
static bool initMeta(DK_RoomMeta* m, const DK_RoomMeta* meta) {
    *m = *meta;

    return true;
}

/** Type override */
static bool updateMeta(DK_RoomMeta* m, const DK_RoomMeta* meta) {
    m->canBuildOn = meta->canBuildOn;
    m->health = meta->health;
    m->isDoor = meta->isDoor;
    m->level = meta->level;
    m->passability = meta->passability;

    return true;
}

META_impl(DK_RoomMeta, Room)
