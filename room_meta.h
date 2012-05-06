/* 
 * File:   room_meta.h
 * Author: fnuecke
 *
 * Created on May 5, 2012, 5:27 PM
 */

#ifndef ROOM_META_H
#define	ROOM_META_H

#include "types.h"

#ifdef	__cplusplus
extern "C" {
#endif

    /** Meta description of a room type */
    struct DK_RoomMeta {
        /** The ID of this block type */
        unsigned int id;

        /** The name of this block type */
        const char* name;

        /** The type of underground the room can be built on */
        DK_Passability canBuildOn;

        /** The type of passability the room provides */
        DK_Passability passability;

        /** The level (height) at which to render this block type */
        DK_BlockLevel level;

        /** Determines whether the room is a door (can be locked) */
        char isDoor;

        /** Health of this block (conversion resistance or damage it can take) */
        unsigned int health;
    };

    const DK_RoomMeta* DK_GetRoomMeta(unsigned int id);

    const DK_RoomMeta* DK_GetRoomMetaByName(const char* name);

    void DK_AddRoomMeta(const DK_RoomMeta* meta);

    void DK_InitRoomMeta(void);

#ifdef	__cplusplus
}
#endif

#endif	/* ROOM_META_H */

