/* 
 * Author: fnuecke
 *
 * Created on May 17, 2012, 3:14 AM
 */

#ifndef HAND_H
#define	HAND_H

#include "types.h"
#include "vmath.h"

#ifdef	__cplusplus
extern "C" {
#endif

    void MP_PickUpUnit(MP_Player player, MP_Unit* unit);

    void MP_PickUpGold(MP_Player player, unsigned int amount);

    void MP_DropTopHandEntry(MP_Player player, const vec2* position);

    unsigned int MP_ObjectsInHandCount(MP_Player player);

#ifdef	__cplusplus
}
#endif

#endif
