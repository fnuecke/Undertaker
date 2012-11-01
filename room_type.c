#include "room_type.h"

#include "type_impl.h"

TYPE_GLOBALS(MP_RoomType)

/** Reset defaults on map change */
static void resetDefaults(void) {
    gTypeDefaults.canBuildOn = 0;
    gTypeDefaults.passability = 0;
    gTypeDefaults.level = MP_BLOCK_LEVEL_NORMAL;
    gTypeDefaults.isDoor = false;
    gTypeDefaults.health = 0;
}

/** New type registered */
inline static bool initType(MP_RoomType* stored, const MP_RoomType* input) {
    *stored = *input;

    return true;
}

/** Type override */
inline static bool updateType(MP_RoomType* stored, const MP_RoomType* input) {
    stored->canBuildOn = input->canBuildOn;
    stored->passability = input->passability;
    stored->level = input->level;
    stored->isDoor = input->isDoor;
    stored->health = input->health;

    return true;
}

/** Clear up data for a meta on deletion */
inline static void deleteType(MP_RoomType* type) {
}

TYPE_IMPL(MP_RoomType, Room)
