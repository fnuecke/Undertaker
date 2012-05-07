/* 
 * File:   types.h
 * Author: fnuecke
 *
 * Created on May 5, 2012, 4:32 PM
 */

#ifndef TYPES_H
#define	TYPES_H

#ifdef	__cplusplus
extern "C" {
#endif

    ///////////////////////////////////////////////////////////////////////////
    // Struct typedefs / forward declarations
    ///////////////////////////////////////////////////////////////////////////

    typedef struct DK_Ability DK_Ability;
    typedef struct DK_AbilityMeta DK_AbilityMeta;

    typedef struct DK_Block DK_Block;
    typedef struct DK_BlockMeta DK_BlockMeta;

    typedef struct DK_Job DK_Job;
    typedef struct DK_JobMeta DK_JobMeta;

    typedef struct DK_Room DK_Room;
    typedef struct DK_RoomMeta DK_RoomMeta;

    typedef struct DK_Unit DK_Unit;
    typedef struct DK_UnitMeta DK_UnitMeta;
    typedef struct DK_UnitSatisfaction DK_UnitSatisfaction;
    typedef struct DK_UnitSatisfactionMeta DK_UnitSatisfactionMeta;
    typedef struct DK_UnitJobSatisfactionMeta DK_UnitJobSatisfactionMeta;
    typedef struct DK_AI_Info DK_AI_Info;

    ///////////////////////////////////////////////////////////////////////////
    // value typedefs
    ///////////////////////////////////////////////////////////////////////////

    /**
     * Possible passability types. These are defined by single bits, so they can
     * be combined as a bit field.
     */
    typedef unsigned int DK_Passability;

    /** ID for textures that are loaded via the LoadTexture facilities */
    typedef unsigned int DK_TextureID;

    /** Boolean type for readability */
    typedef unsigned char bool;

#define true ((bool)1)
#define false ((bool)0)

    ///////////////////////////////////////////////////////////////////////////
    // Enums
    ///////////////////////////////////////////////////////////////////////////

    /** Possible block levels */
    typedef enum DK_BlockLevel {
        DK_BLOCK_LEVEL_PIT,
        DK_BLOCK_LEVEL_LOWERED,
        DK_BLOCK_LEVEL_NORMAL,
        DK_BLOCK_LEVEL_HIGH,

        DK_BLOCK_LEVEL_COUNT
    } DK_BlockLevel;

    typedef enum DK_BlockTextureTop {
        DK_BLOCK_TEXTURE_TOP,
        DK_BLOCK_TEXTURE_TOP_N,
        DK_BLOCK_TEXTURE_TOP_NE,
        DK_BLOCK_TEXTURE_TOP_NS,
        DK_BLOCK_TEXTURE_TOP_NES,
        DK_BLOCK_TEXTURE_TOP_NESW,
        DK_BLOCK_TEXTURE_TOP_NE_CORNER,
        DK_BLOCK_TEXTURE_TOP_NES_CORNER,
        DK_BLOCK_TEXTURE_TOP_NESW_CORNER,
        DK_BLOCK_TEXTURE_TOP_NESWN_CORNER,
        DK_BLOCK_TEXTURE_TOP_OWNED_OVERLAY,
        DK_BLOCK_TEXTURE_TOP_COUNT
    } DK_BlockTextureTop;

    typedef enum DK_BlockTextureSide {
        DK_BLOCK_TEXTURE_SIDE,
        DK_BLOCK_TEXTURE_SIDE_OWNED_OVERLAY,
        DK_BLOCK_TEXTURE_SIDE_COUNT
    } DK_BlockTextureSide;

    /** Possible events to which a job may react */
    typedef enum DK_JobEvents {
        DK_JOB_EVENT_UNIT_ADDED,
        DK_JOB_EVENT_BLOCK_SELECTION_CHANGED,
        DK_JOB_EVENT_BLOCK_DESTROYED,
        DK_JOB_EVENT_BLOCK_CONVERTED,

        DK_JOB_EVENT_COUNT
    } DK_JobEvents;

    /** Player IDs for possible players in a game */
    typedef enum {
        DK_PLAYER_NONE,
        DK_PLAYER_ONE,
        DK_PLAYER_TWO,
        DK_PLAYER_THREE,
        DK_PLAYER_FOUR,
        DK_PLAYER_FIVE,
        DK_PLAYER_COUNT
    } DK_Player;

#ifdef	__cplusplus
}
#endif

#endif	/* TYPES_H */

