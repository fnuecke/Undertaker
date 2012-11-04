/* 
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

    typedef struct MP_Type MP_Type;

    typedef struct MP_Ability MP_Ability;
    typedef struct MP_AbilityType MP_AbilityType;

    typedef struct MP_Block MP_Block;
    typedef struct MP_BlockType MP_BlockType;

    typedef struct MP_Job MP_Job;
    typedef MP_Job* const* MP_JobList;
    typedef struct MP_JobType MP_JobType;

    typedef struct MP_Room MP_Room;
    typedef struct MP_RoomType MP_RoomType;

    typedef struct MP_Unit MP_Unit;
    typedef struct MP_UnitType MP_UnitType;
    typedef struct MP_UnitAbilityType MP_UnitAbilityType;
    typedef struct MP_UnitJobType MP_UnitJobType;
    typedef struct MP_AI_Info MP_AI_Info;

    ///////////////////////////////////////////////////////////////////////////
    // value typedefs
    ///////////////////////////////////////////////////////////////////////////

    /**
     * Possible passability types. These are defined by single bits, so they can
     * be combined as a bit field.
     */
    typedef unsigned int MP_Passability;

#define MP_PASSABILITY_NONE 0

    /** ID for textures that are loaded via the LoadTexture facilities */
    typedef unsigned int MP_TextureID;

    /** Boolean type for readability */
    typedef unsigned char bool;

#define true ((bool)1)
#define false ((bool)0)

    ///////////////////////////////////////////////////////////////////////////
    // Enums
    ///////////////////////////////////////////////////////////////////////////

    /** Possible block levels */
    typedef enum MP_BlockLevel {
        MP_BLOCK_LEVEL_PIT,
        MP_BLOCK_LEVEL_LOWERED,
        MP_BLOCK_LEVEL_NORMAL,
        MP_BLOCK_LEVEL_HIGH,

        MP_BLOCK_LEVEL_COUNT
    } MP_BlockLevel;

    typedef enum MP_BlockTextureTop {
        MP_BLOCK_TEXTURE_TOP,
        MP_BLOCK_TEXTURE_TOP_N,
        MP_BLOCK_TEXTURE_TOP_NE,
        MP_BLOCK_TEXTURE_TOP_NS,
        MP_BLOCK_TEXTURE_TOP_NES,
        MP_BLOCK_TEXTURE_TOP_NESW,
        MP_BLOCK_TEXTURE_TOP_NE_CORNER,
        MP_BLOCK_TEXTURE_TOP_NES_CORNER,
        MP_BLOCK_TEXTURE_TOP_NESW_CORNER,
        MP_BLOCK_TEXTURE_TOP_NESWN_CORNER,
        MP_BLOCK_TEXTURE_TOP_OWNED_OVERLAY,
        MP_BLOCK_TEXTURE_TOP_COUNT
    } MP_BlockTextureTop;

    typedef enum MP_BlockTextureSide {
        MP_BLOCK_TEXTURE_SIDE,
        MP_BLOCK_TEXTURE_SIDE_OWNED_OVERLAY,
        MP_BLOCK_TEXTURE_SIDE_COUNT
    } MP_BlockTextureSide;

    /** Possible events to which a job may react */
    typedef enum MP_JobEvent {
        MP_JOB_EVENT_UNIT_ADDED,
        MP_JOB_EVENT_BLOCK_SELECTION_CHANGED,
        MP_JOB_EVENT_BLOCK_META_CHANGED,
        MP_JOB_EVENT_BLOCK_OWNER_CHANGED,

        MP_JOB_EVENT_COUNT
    } MP_JobEvent;

    /** Player IDs for possible players in a game */
    typedef enum MP_Player {
        MP_PLAYER_NONE,
        MP_PLAYER_ONE,
        MP_PLAYER_TWO,
        MP_PLAYER_THREE,
        MP_PLAYER_FOUR,
        MP_PLAYER_FIVE,
        MP_PLAYER_COUNT
    } MP_Player;

#ifdef	__cplusplus
}
#endif

#endif
