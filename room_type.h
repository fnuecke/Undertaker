/* 
 * Author: fnuecke
 *
 * Created on May 5, 2012, 5:27 PM
 */

#ifndef ROOM_TYPE_H
#define	ROOM_TYPE_H

#include "type.h"
#include "types.h"

#ifdef	__cplusplus
extern "C" {
#endif

    /** Description of a room type */
    struct MP_RoomType {
        /** The type information */
        MP_Type info;

        /** The type of underground the room can be built on */
        MP_Passability canBuildOn;

        /** The type of passability the room provides */
        MP_Passability passability;

        /** The level (height) at which to render this block type */
        MP_BlockLevel level;

        /** Determines whether the room is a door (can be locked) */
        bool isDoor;

        /** Health of this room (conversion resistance or damage it can take) */
        unsigned int health;
    };

    TYPE_HEADER(MP_RoomType, Room);

#ifdef	__cplusplus
}
#endif

#endif
