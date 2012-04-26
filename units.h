/* 
 * File:   units.h
 * Author: fnuecke
 *
 * Created on April 16, 2012, 7:18 PM
 */

#ifndef UNITS_H
#define	UNITS_H

#include "players.h"
#include "vmath.h"

#ifdef	__cplusplus
extern "C" {
#endif

    ///////////////////////////////////////////////////////////////////////////
    // Types
    ///////////////////////////////////////////////////////////////////////////

    /**
     * Possible unit types.
     */
    typedef enum {
        DK_UNIT_IMP,
        DK_UNIT_WIZARD
    } DK_UnitType;

    typedef struct DK_Unit DK_Unit;

    ///////////////////////////////////////////////////////////////////////////
    // Accessors
    ///////////////////////////////////////////////////////////////////////////

    /**
     * Get the current position of the specified unit, in map space.
     * @param unit the unit to get the position for.
     */
    const vec2* DK_GetUnitPosition(const DK_Unit* unit);

    /**
     * Get the owner of the specified unit.
     * @param unit the unit to get the owner of.
     * @return the owner of that unit.
     */
    DK_Player DK_GetUnitOwner(const DK_Unit* unit);

    /**
     * Test whether the specified unit is immune to lava.
     */
    int DK_IsUnitImmuneToLava(const DK_Unit* unit);

    ///////////////////////////////////////////////////////////////////////////
    // Modifiers
    ///////////////////////////////////////////////////////////////////////////

    /**
     * Add a unit for a player at the specified block coordinates.
     */
    int DK_AddUnit(DK_Player player, DK_UnitType type, unsigned short x, unsigned short y);

    /**
     * Make a unit (imp) cancel it's current job.
     */
    void DK_CancelJob(DK_Unit* unit);

    ///////////////////////////////////////////////////////////////////////////
    // Initialization
    ///////////////////////////////////////////////////////////////////////////

    /** Initialize unit logic */
    void DK_InitUnits(void);

#ifdef	__cplusplus
}
#endif

#endif	/* UNITS_H */

