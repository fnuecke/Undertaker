/* 
 * File:   units.h
 * Author: fnuecke
 *
 * Created on April 16, 2012, 7:18 PM
 */

#ifndef UNITS_H
#define	UNITS_H

#include "players.h"

typedef enum {
    DK_UNIT_IMP,
    DK_UNIT_WIZARD
} DK_UnitType;

typedef struct DK_Unit DK_Unit;

#ifdef	__cplusplus
extern "C" {
#endif

/** (Re)Initialize unit logic after a map change */
void DK_init_units(void);

/** Update unit logic, i.e. run ai and update positions */
void DK_update_units(void);

/** Render units to screen */
void DK_render_units(void);

/** Add a unit for a player at the specified block coordinates */
unsigned int DK_add_unit(DK_Player player, DK_UnitType type, unsigned short x, unsigned short y);

/** Make a unit (imp) cancel it's current job */
void DK_unit_cancel_job(DK_Unit* unit);

/** Get the current position of the specified unit; returns 1 on success, 0 on failure (null pointer) */
int DK_unit_position(const DK_Unit* unit, float* x, float* y);

/** Test whether the specified unit is immune to lava */
int DK_unit_immune_to_lava(const DK_Unit* unit);

/** Get the owner of the specified unit */
DK_Player DK_unit_owner(const DK_Unit* unit);

#ifdef	__cplusplus
}
#endif

#endif	/* UNITS_H */

