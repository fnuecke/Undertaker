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

    /** Represents a single room in the world */
    struct MP_Room {
        /** Info on the room type */
        const MP_RoomType* type;

        /** Block this room is on */
        MP_Block* block;

        /**
         * Next room this room is connected to. A complete room may be composed
         * of multiple room instances (each on one block). We track which rooms
         * belong together by keeping them in a single linked list.
         */
        MP_Room* next;
    };

    /**
     * Set the room for the specified block. The block must be empty. Set to
     * null to remove a room. This will take care of creating/destroying joined
     * rooms.
     * @param block the block to set the room for.
     * @param type the type of room to set.
     * @return true if the room was set successfully, false otherwise.
     */
    bool MP_SetRoom(MP_Block* block, MP_RoomType* type);

    /**
     * Gets all rooms of the specified type for the specified player. This
     * refers to joined rooms, i.e. all connected (neighbored via edges) rooms
     * will count as one large room.
     * @param player the player to get the room for.
     * @param type the type of room to get.
     * @param count the number of elements in this room.
     * @return the first entry in the list of room elements.
     */
    MP_Room* MP_GetRoom(MP_Player player, MP_RoomType* type, unsigned int* count);

#ifdef	__cplusplus
}
#endif

#endif
