/* 
 * File:   ability.h
 * Author: fnuecke
 *
 * Created on May 3, 2012, 6:00 PM
 */

#ifndef ABILITY_H
#define	ABILITY_H

#include "types.h"

#ifdef	__cplusplus
extern "C" {
#endif

    struct DK_AbilityMeta {
        /** The ID of the ability type */
        unsigned int id;

        /** The name of the ability type */
        char name[32];

        /** The time it takes before the ability can trigger again */
        float cooldown;
    };

    struct DK_Ability {
        /** Info on the ability type */
        const DK_AbilityMeta* meta;

        /** The remaining cooldown time before the ability can trigger again */
        unsigned int cooldown;
    };

#ifdef	__cplusplus
}
#endif

#endif	/* ABILITY_H */

