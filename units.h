/* 
 * File:   units.h
 * Author: fnuecke
 *
 * Created on April 16, 2012, 7:18 PM
 */

#ifndef UNITS_H
#define	UNITS_H

#include "units_meta.h"
#include "vmath.h"

#ifdef	__cplusplus
extern "C" {
#endif

    ///////////////////////////////////////////////////////////////////////////
    // Types
    ///////////////////////////////////////////////////////////////////////////

    /** Holds information on unit job desire saturation */
    struct MP_UnitSatisfaction {
        /** How satisfied the unit is with each job */
        float* jobSaturation;
    };

    /** Contains data on a single unit instance */
    struct MP_Unit {
        /** Info on the unit type */
        const MP_UnitMeta* meta;

        /** The player this unit belongs to */
        MP_Player owner;

        /** Current position of the unit */
        vec2 position;

        /** Unit desire saturation information */
        MP_UnitSatisfaction satisfaction;

        /** Internal AI state of the unit */
        MP_AI_Info* ai;
    };

    ///////////////////////////////////////////////////////////////////////////
    // Accessors
    ///////////////////////////////////////////////////////////////////////////

    /**
     * Tells if a unit is currently moving.
     * @param unit the unit to check for.
     * @return whether the unit is currently moving (1) or not (0).
     */
    bool MP_IsUnitMoving(const MP_Unit* unit);

    ///////////////////////////////////////////////////////////////////////////
    // Modifiers
    ///////////////////////////////////////////////////////////////////////////

    /**
     * Add a unit for a player at the specified coordinates.
     * @param player the player for whom to create the unit.
     * @param type the unit-type to create.
     * @param position the position at which to spawn the unit, in map space.
     * @return whether adding was successful (1) or not(0).
     */
    int MP_AddUnit(MP_Player player, const MP_UnitMeta* type, const vec2* position);

    /**
     * Make a unit working on the specified job cancel it (if one is working on
     * it at all). This will unwind the unit's AI stack to the entry associated
     * with the job and mark it for cancellation.
     * @param job the job to stop.
     */
    void MP_StopJob(MP_Job* job);

    ///////////////////////////////////////////////////////////////////////////
    // Initialization
    ///////////////////////////////////////////////////////////////////////////

    /** Initialize unit logic */
    void MP_InitUnits(void);

    ///////////////////////////////////////////////////////////////////////////
    // Scripting
    ///////////////////////////////////////////////////////////////////////////

    void MP_Lua_RegisterUnit(lua_State* L);

    void MP_Lua_pushunit(lua_State* L, MP_Unit* unit);

    MP_Unit* MP_Lua_checkunit(lua_State* L, int narg);

#ifdef	__cplusplus
}
#endif

#endif	/* UNITS_H */

