#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "log.h"
#include "room.h"
#include "block.h"
#include "map.h"

/** A room consists of multiple nodes, thus spanning multiple blocks */
struct MP_Room {
    /** Info on the room type */
    const MP_RoomType* type;

    /** First room in the linked list of rooms in this group */
    MP_RoomNode* head;

    /** The number of nodes in this room, to avoid counting the list */
    unsigned int count;

    /** Current health of the room (max is base value times node count) */
    float health;
};

/** List of all current room lists (head of each list) */
MP_Room** gRooms[MP_PLAYER_COUNT][MP_TYPE_ID_MAX];

/** Number of rooms for each room type */
static unsigned int gRoomCount[MP_PLAYER_COUNT][MP_TYPE_ID_MAX];

/** Number of rooms for each room type */
static unsigned int gRoomCapacity[MP_PLAYER_COUNT][MP_TYPE_ID_MAX];

static void ensureListCapacity(MP_Player player, unsigned int index) {
    if (gRoomCount[player][index] >= gRoomCapacity[player][index]) {
        gRoomCapacity[player][index] = gRoomCapacity[player][index] * 2 + 1;
        if (!(gRooms[player][index] =
            realloc(gRooms[player][index], gRoomCapacity[player][index] * sizeof (MP_Room**)))) {
            MP_log_fatal("Out of memory while resizing room list.\n");
        }
    }
}

inline static bool containsNode(const MP_Room* haystack, const MP_RoomNode* needle) {
    const MP_RoomNode* node = haystack->head;
    while (node) {
        if (node == needle) {
            return true;
        }
        node = node->next;
    }
    return false;
}

/** Get the head of the room list the specified block is in */
static MP_Room* getRoomFor(const MP_Block* block) {
    if (block && block->roomNode) {
        const MP_Player player = block->player;
        const unsigned int index = block->roomNode->room->type->info.id - 1;
        const unsigned int count = gRoomCount[player][index];
        for (unsigned int i = 0; i < count; ++i) {
            MP_Room* room = gRooms[player][index][i];
            if (containsNode(room, block->roomNode)) {
                return room;
            }
        }
    }
    return NULL;
}

inline static MP_Player getPlayer(const MP_Room* room) {
    return room->head->block->player;
}

/** Returns the head of the list all neighbors are in, or null if they differ */
static MP_Room* neighborFor(MP_Block* block, const MP_RoomType* type) {
    MP_Room* result = NULL;
    MP_Room* room = NULL;
    unsigned short x, y;

    assert(block);

    MP_GetBlockCoordinates(block, &x, &y);
    if (// Skip if there's no block here.
        (!(room = getRoomFor(MP_GetBlockAt(x - 1, y))) ||
        // Skip if the room is not of the type we care for.
        room->type != type ||
        // Otherwise set it as the group if it's not set yet.
        (!result && (result = room)) ||
        // Otherwise make sure it's the same group.
        (room->type == result->type &&
        getPlayer(room) == getPlayer(result))) &&

        // Skip if there's no block here.
        (!(room = getRoomFor(MP_GetBlockAt(x + 1, y))) ||
        // Skip if the room is not of the type we care for.
        room->type != type ||
        // Otherwise set it as the group if it's not set yet.
        (!result && (result = room)) ||
        // Otherwise make sure it's the same group.
        (room->type == result->type &&
        getPlayer(room) == getPlayer(result))) &&

        // Skip if there's no block here.
        (!(room = getRoomFor(MP_GetBlockAt(x, y - 1))) ||
        // Skip if the room is not of the type we care for.
        room->type != type ||
        // Otherwise set it as the group if it's not set yet.
        (!result && (result = room)) ||
        // Otherwise make sure it's the same group.
        (room->type == result->type &&
        getPlayer(room) == getPlayer(result))) &&

        // Skip if there's no block here.
        (!(room = getRoomFor(MP_GetBlockAt(x, y + 1))) ||
        // Skip if the room is not of the type we care for.
        room->type != type ||
        // Otherwise set it as the group if it's not set yet.
        (!result && (result = room)) ||
        // Otherwise make sure it's the same group.
        (room->type == result->type &&
        getPlayer(room) == getPlayer(result)))
        ) {
        // All blocks were either null or in the same group.
        return result;
    }

    // Blocks differed, or all were null.
    return NULL;
}

/** Builds a new group from a head and a list of existing groups */
static void addRoom(const MP_RoomType* type, MP_RoomNode* head, unsigned int count, float health) {
    unsigned int player, index;
    MP_Room* room;

    if (!(room = calloc(1, sizeof (MP_Room)))) {
        MP_log_fatal("Out of memory while allocating a room.\n");
    }
    room->type = type;
    room->head = head;
    room->count = count;
    room->health = health;

    // Store in room group list.
    player = getPlayer(room);
    index = type->info.id - 1;
    ensureListCapacity(player, index);
    gRooms[player][index][gRoomCount[player][index]++] = room;
}

/** Removes a group from the list of groups and frees it */
static void deleteRoom(MP_Room* room) {
    unsigned int player, index, count;

    assert(room);

    player = getPlayer(room);
    index = room->type->info.id - 1;
    count = gRoomCount[player][index];

    assert(gRoomCount[player][index] > 0);

    // Find the group.
    for (unsigned int i = 0; i < count; ++i) {
        if (gRooms[player][index][i] == room) {
            // Free the actual memory.
            free(room);

            // Move up the list entries to close the gap.
            --gRoomCount[player][index];
            memmove(&gRooms[player][index][i], &gRooms[player][index][i + 1],
                    (gRoomCount[player][index] - i) * sizeof (MP_Room*));
            return;
        }
    }

    assert(!"trying to remove unknown room group");
}

static void addNode(const MP_RoomType* type, MP_Block* block) {
    MP_Room* room;
    MP_RoomNode* node;

    assert(!block->roomNode);

    if (!(node = calloc(1, sizeof (MP_RoomNode)))) {
        MP_log_fatal("Out of memory while allocating a room node.\n");
    }
    node->block = block;

    // Check neighboring blocks. If they all belong to the same room group we
    // can just add this room to that list. Otherwise we'll dissolve those
    // groups temporarily and rebuild them starting with our newly placed room.
    if ((room = neighborFor(block, type))) {
        // Got an available group, add to that and we're done.
        node->room = room;
        node->next = room->head; // add as new head of list
        room->head = node;
    } else {
        // Differing groups we will merge. Dissolve them first.
        unsigned short x, y;
        MP_GetBlockCoordinates(block, &x, &y);
        {
            MP_Room * neighbors[4] = {getRoomFor(MP_GetBlockAt(x - 1, y)),
                                      getRoomFor(MP_GetBlockAt(x + 1, y)),
                                      getRoomFor(MP_GetBlockAt(x, y - 1)),
                                      getRoomFor(MP_GetBlockAt(x, y + 1))};
            // Accumulate count and health over all neighbors of the same type.
            unsigned int count = 0;
            float health = 0;
            MP_RoomNode* tail = node;
            for (unsigned int i = 0; i < 4; ++i) {
                if (neighbors[i] && neighbors[i]->type == type) {
                    count += neighbors[i]->count;
                    health += neighbors[i]->health;
                    // Also concatenate the node lists.
                    tail->next = neighbors[i]->head;
                    tail = neighbors[i]->head;
                    while (tail->next) {
                        tail = tail->next;
                    }
                    // Free this room instance.
                    deleteRoom(neighbors[i]);
                }
            }
            addRoom(type, node, count + 1, health + type->health);
        }
    }
}

static void removeNode(MP_Block* block) {
    assert(block->roomNode);


}

void MP_SetRoom(MP_Block* block, const MP_RoomType* type) {
    assert(block);

    if (type) {
        addNode(type, block);
    } else {
        removeNode(block);
    }
}

///////////////////////////////////////////////////////////////////////////////
// Accessors
///////////////////////////////////////////////////////////////////////////////

const MP_RoomType* MP_GetRoomType(const MP_Room* room) {
    assert(room);

    return room->type;
}

unsigned int MP_GetRoomSize(const MP_Room* room) {
    assert(room);

    return room->count;
}

float MP_GetRoomHealth(const MP_Room* room) {
    assert(room);

    return room->health;
}

void MP_SetRoomHealth(MP_Room* room, float value) {
    assert(room);

    if (value < 0) {
        value = 0;
    }
    if (value > room->count * room->type->health) {
        value = room->count * room->type->health;
    }
    room->health = value;
}

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
    memset(gRooms, 0, MP_PLAYER_COUNT * MP_TYPE_ID_MAX * sizeof (MP_Room**));
    memset(gRoomCount, 0, MP_PLAYER_COUNT * MP_TYPE_ID_MAX * sizeof (unsigned int));
    memset(gRoomCapacity, 0, MP_PLAYER_COUNT * MP_TYPE_ID_MAX * sizeof (unsigned int));
}
