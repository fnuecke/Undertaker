/* 
 * File:   room.h
 * Author: fnuecke
 *
 * Created on May 2, 2012, 5:07 PM
 */

#ifndef ROOM_H
#define	ROOM_H

#include "meta_room.h"

#ifdef	__cplusplus
extern "C" {
#endif

    ///////////////////////////////////////////////////////////////////////////
    // Types
    ///////////////////////////////////////////////////////////////////////////

    /** Represents a single room in the world */
    struct MP_Room {
        /** Info on the room type */
        const MP_RoomMeta* meta;

        /** Blocks this room covers */
        MP_Block* blocks;

        /** Number of blocks this room covers */
        unsigned int blockCount;
    };

#ifdef	__cplusplus
}
#endif

#endif	/* ROOM_H */

