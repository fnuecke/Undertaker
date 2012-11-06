/* 
 * Author: fnuecke
 *
 * Created on May 2, 2012, 5:07 PM
 */

#ifndef ROOM_H
#define	ROOM_H

#include "room_type.h"

#ifdef	__cplusplus
extern "C" {
#endif

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

    /** Represents a single room in the world */
    struct MP_RoomNode {
        /** Block this node is on */
        MP_Block* block;

        /** The room this node belongs to */
        MP_Room* room;

        /** Next node in the room (linked list) */
        MP_RoomNode* next;
    };

    /**
     * Set the room for the specified block. The block must be empty. Set to
     * null to remove a room. This will take care of creating/destroying joined
     * rooms.
     * @param block the block to set the room for.
     * @param type the type of room to set.
     */
    void MP_SetRoom(MP_Block* block, const MP_RoomType* type);

    /**
     * Gets all rooms of the specified type for the specified player. This
     * refers to joined rooms, i.e. all connected (neighbored via edges) rooms
     * will count as one large room.
     * @param player the player to get the room for.
     * @param type the type of room to get.
     * @param count the number of elements in this room.
     * @return the first entry in the list of room elements.
     */
    MP_RoomList MP_GetRooms(const MP_RoomType* type, MP_Player player, unsigned int* count);

    /**
     * Clear all room lists and free all additional memory.
     */
    void MP_ClearRooms(void);

    /**
     * Initialize room system.
     */
    void MP_InitRooms(void);

#ifdef	__cplusplus
}
#endif

#endif
