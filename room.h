/* 
 * File:   room.h
 * Author: fnuecke
 *
 * Created on May 2, 2012, 5:07 PM
 */

#ifndef ROOM_H
#define	ROOM_H

#include "room_meta.h"

#ifdef	__cplusplus
extern "C" {
#endif

    /** Represents a single room in the world */
    struct DK_Room {
        /** Info on the room type */
        const DK_RoomMeta* meta;

        /** Blocks this room covers */
        DK_Block* blocks;

        /** Number of blocks this room covers */
        unsigned int blockCount;
    };

#ifdef	__cplusplus
}
#endif

#endif	/* ROOM_H */

