/* 
 * Author: fnuecke
 *
 * Created on May 3, 2012, 6:00 PM
 */

#ifndef ABILITY_H
#define	ABILITY_H

#include "ability_type.h"

#ifdef	__cplusplus
extern "C" {
#endif

    struct MP_Ability {
        /** Info on the ability type */
        const MP_AbilityType* type;

        /** The unit this ability belongs to */
        MP_Unit* unit;

        /** Reference to the Lua table containing property values */
        int properties;

        /** The remaining cooldown time before the ability can trigger again */
        unsigned int cooldown;
    };

    /** Activates the specified ability, if it is not on cooldown. */
    float MP_UseAbility(MP_Ability* ability);

#ifdef	__cplusplus
}
#endif

#endif
