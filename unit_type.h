/* 
 * Author: fnuecke
 *
 * Created on May 5, 2012, 5:18 PM
 */

#ifndef UNIT_TYPE_H
#define	UNIT_TYPE_H

#include "type.h"
#include "types.h"

/** Default values for a unit's ability */
struct MP_UnitAbilityType {
    /** The type information of the ability */
    const MP_AbilityType* type;

    /** Reference to Lua table with default values for properties */
    int properties;
};

/** Information about a single job a unit can perform */
struct MP_UnitJobType {
    /** The type information of the job */
    const MP_JobType* type;

    /** How strongly the unit wants to do this job */
    float preference;

    /** The initial job saturation value */
    float initialSaturation;

    /** The value at which a unit becomes seriously pissed */
    float angerThreshold;

    /** The value at which a unit is unsatisfied */
    float unsatisfiedThreshold;

    /** The value at which a unit is satisfied */
    float satisfiedThreshold;

    /** The value at which a unit becomes bored with this job */
    float boredThreshold;

    /** How does the saturation change while performing the job? */
    float performingDelta;

    /** How does the saturation change while *not* performing the job? */
    float notPerformingDelta;
};

/** Description of a single unit type */
struct MP_UnitType {
    /** The type information */
    MP_Type info;

    /** Passability types this unit supports */
    MP_Passability canPass;

    /** Base movement speed of the unit */
    float moveSpeed;

    /** Jobs the unit can perform */
    MP_UnitAbilityType* abilities;

    /** Number of abilities for this unit */
    unsigned int abilityCount;

    /** Jobs the unit can perform */
    MP_UnitJobType* jobs;

    /** Number of jobs for this unit */
    unsigned int jobCount;

    /** What the unit does when it gets angry */
    const MP_JobType* angerJob;
};

TYPE_HEADER(MP_UnitType, Unit);

#endif
