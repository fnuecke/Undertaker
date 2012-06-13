/* 
 * File:   meta_ability.h
 * Author: fnuecke
 *
 * Created on June 8, 2012, 7:21 PM
 */

#ifndef META_ABILITY_H
#define	META_ABILITY_H

#include "meta.h"

#ifdef	__cplusplus
extern "C" {
#endif

    struct MP_AbilityMeta {
        /** The ID of the ability type */
        unsigned int id;

        /** The name of the ability type */
        char* name;

        /** The time it takes before the ability can trigger again */
        float cooldown;

        /** Whether the script has a run method */
        bool hasRunMethod;
    };

    META_header(MP_AbilityMeta, Ability);

#ifdef	__cplusplus
}
#endif

#endif	/* META_ABILITY_H */

