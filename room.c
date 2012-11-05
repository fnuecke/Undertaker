#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "room.h"
#include "block.h"

/** List of all current room lists (head of each list) */
MP_Room** gRooms[MP_PLAYER_COUNT][MP_TYPE_ID_MAX];

/** Number of rooms for each room type */
static unsigned int gRoomCount[MP_PLAYER_COUNT][MP_TYPE_ID_MAX];

/** Number of rooms for each room type */
static unsigned int gRoomCapacity[MP_PLAYER_COUNT][MP_TYPE_ID_MAX];

bool MP_SetRoom(MP_Block* block, MP_RoomType* type) {
    assert(block);
    assert(!block->room);

    return true;
}

///////////////////////////////////////////////////////////////////////////////
// Accessors
///////////////////////////////////////////////////////////////////////////////

MP_RoomList MP_GetRooms(const MP_RoomType* type, MP_Player player, unsigned int* count) {
    unsigned int index;

    assert(player > MP_PLAYER_NONE && player < MP_PLAYER_COUNT);
    assert(type);
    assert(count);

    index = type->info.id - 1;
    *count = gRoomCount[player][index];
    return gRooms[player][index];
}

///////////////////////////////////////////////////////////////////////////////
// Init / Teardown
///////////////////////////////////////////////////////////////////////////////

void MP_ClearRooms(void) {
    for (unsigned int player = 0; player < MP_PLAYER_COUNT; ++player) {
        for (unsigned int typeId = 0; typeId < MP_TYPE_ID_MAX; ++typeId) {
            for (unsigned int number = 0; number < gRoomCount[player][typeId]; ++number) {
                free(gRooms[player][typeId][number]);
            }
            free(gRooms[player][typeId]);
            gRooms[player][typeId] = NULL;
            gRoomCount[player][typeId] = 0;
            gRoomCapacity[player][typeId] = 0;
        }
    }
}

void MP_InitRooms(void) {
    memset(gRooms, 0, MP_PLAYER_COUNT * MP_TYPE_ID_MAX * sizeof (MP_Job**));
    memset(gRoomCount, 0, MP_PLAYER_COUNT * MP_TYPE_ID_MAX * sizeof (unsigned int));
    memset(gRoomCapacity, 0, MP_PLAYER_COUNT * MP_TYPE_ID_MAX * sizeof (unsigned int));
}
