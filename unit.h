/* 
 * File:   units.h
 * Author: fnuecke
 *
 * Created on April 16, 2012, 7:18 PM
 */

#ifndef UNITS_H
#define	UNITS_H

#include "meta_unit.h"
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

    /**
     * Get the unit currently hovered by the cursor, if any.
     */
    MP_Unit* MP_GetUnitUnderCursor(void);

    /**
     * Get the distance of the unit to the camera in an interval of [0, 1].
     */
    float MP_GetUnitDepthUnderCursor(void);

    ///////////////////////////////////////////////////////////////////////////
    // Modifiers
    ///////////////////////////////////////////////////////////////////////////

    /**
     * Add a unit for a player at the specified coordinates.
     * @param player the player for whom to create the unit.
     * @param type the unit-type to create.
     * @param position the position at which to spawn the unit, in map space.
     * @return the added unit, or NULL on failure.
     */
    MP_Unit* MP_AddUnit(MP_Player player, const MP_UnitMeta* type, const vec2* position);

    /**
     * Make a unit working on the specified job cancel it (if one is working on
     * it at all). This will unwind the unit's AI stack to the entry associated
     * with the job and mark it for cancellation.
     * @param job the job to stop.
     */
    void MP_StopJob(MP_Job* job);

    ///////////////////////////////////////////////////////////////////////////
    // Initialization / Cleanup
    ///////////////////////////////////////////////////////////////////////////

    /** Clear unit lists, freeing all dynamically allocated memory */
    void MP_ClearUnits(void);

    /** Initialize unit logic */
    void MP_InitUnits(void);

#ifdef	__cplusplus
}
#endif

#endif	/* UNITS_H */

