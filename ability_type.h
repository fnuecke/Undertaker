/* 
 * Author: fnuecke
 *
 * Created on June 8, 2012, 7:21 PM
 */

#ifndef ABILITY_TYPE_H
#define	ABILITY_TYPE_H

#include "type.h"

#ifdef	__cplusplus
extern "C" {
#endif

    struct MP_AbilityType {
        /** The type information */
        MP_Type info;

        /** Reference to the Lua function that is this scripts run method */
        int runMethod;
    };

    TYPE_HEADER(MP_AbilityType, Ability);

    void MP_DisableAbilityRunMethod(const MP_AbilityType* type);

#ifdef	__cplusplus
}
#endif

#endif
