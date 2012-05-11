/* 
 * File:   room_meta.h
 * Author: fnuecke
 *
 * Created on May 5, 2012, 5:27 PM
 */

#ifndef META_ROOM_H
#define	META_ROOM_H

#include "meta.h"
#include "types.h"

#ifdef	__cplusplus
extern "C" {
#endif

    /** Meta description of a room type */
    struct MP_RoomMeta {
        /** The ID of this block type */
        unsigned int id;

        /** The name of this block type */
        const char* name;

        /** The type of underground the room can be built on */
        MP_Passability canBuildOn;

        /** The type of passability the room provides */
        MP_Passability passability;

        /** The level (height) at which to render this block type */
        MP_BlockLevel level;

        /** Determines whether the room is a door (can be locked) */
        bool isDoor;

        /** Health of this block (conversion resistance or damage it can take) */
        unsigned int health;
    };

    META_header(MP_RoomMeta, Room);

#ifdef	__cplusplus
}
#endif

#endif	/* META_ROOM_H */
