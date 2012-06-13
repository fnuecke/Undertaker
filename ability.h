/* 
 * File:   ability.h
 * Author: fnuecke
 *
 * Created on May 3, 2012, 6:00 PM
 */

#ifndef ABILITY_H
#define	ABILITY_H

#include "meta_ability.h"

#ifdef	__cplusplus
extern "C" {
#endif

    struct MP_Ability {
        /** Info on the ability type */
        const MP_AbilityMeta* meta;

        /** The remaining cooldown time before the ability can trigger again */
        unsigned int cooldown;
    };

#ifdef	__cplusplus
}
#endif

#endif	/* ABILITY_H */

