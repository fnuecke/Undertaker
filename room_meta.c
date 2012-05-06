#include "room_meta.h"

static void initMeta(DK_RoomMeta* m, const DK_RoomMeta* meta) {
    *m = *meta;
}

static void updateMeta(DK_RoomMeta* m, const DK_RoomMeta* meta) {
    m->canBuildOn = meta->canBuildOn;
    m->health = meta->health;
    m->isDoor = meta->isDoor;
    m->level = meta->level;
    m->passability = meta->passability;
}

META_impl(DK_RoomMeta, Room)
