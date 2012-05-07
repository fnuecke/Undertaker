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

    /** Holds information on unit satisfaction */
    struct DK_UnitSatisfaction {
        /** How satisfied the unit is with each job */
        float* jobSatisfaction;

        /** How satisfied the unit is from (not) being slapped */
        float slapDelta;

        /** How satisfied the unit is from (not) being held in the hand */
        float inHandDelta;
    };

    /** Contains data on a single unit instance */
    struct DK_Unit {
        /** Info on the unit type */
        const DK_UnitMeta* meta;

        /** The player this unit belongs to */
        DK_Player owner;

        /** Current position of the unit */
        vec2 position;

        /** Unit satisfaction information */
        DK_UnitSatisfaction satisfaction;

        /** Internal AI state of the unit */
        DK_AI_Info* ai;
    };

    ///////////////////////////////////////////////////////////////////////////
    // Accessors
    ///////////////////////////////////////////////////////////////////////////

    /**
     * Tells if a unit is currently moving.
     * @param unit the unit to check for.
     * @return whether the unit is currently moving (1) or not (0).
     */
    bool DK_IsUnitMoving(const DK_Unit* unit);

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
    int DK_AddUnit(DK_Player player, const DK_UnitMeta* type, const vec2* position);

    /**
     * Make a unit working on the specified job cancel it (if one is working on
     * it at all). This will unwind the unit's AI stack to the entry associated
     * with the job and mark it for cancellation.
     * @param job the job to stop.
     */
    void DK_StopJob(DK_Job* job);

    ///////////////////////////////////////////////////////////////////////////
    // Initialization
    ///////////////////////////////////////////////////////////////////////////

    /** Initialize unit logic */
    void DK_InitUnits(void);

#ifdef	__cplusplus
}
#endif

#endif	/* UNITS_H */

