/* 
 * File:   job_events.h
 * Author: fnuecke
 *
 * Created on May 8, 2012, 5:17 PM
 */

#ifndef JOB_EVENTS_H
#define	JOB_EVENTS_H

#ifdef	__cplusplus
extern "C" {
#endif

    void MP_Lua_FireUnitAdded(MP_Unit* unit);

    void MP_Lua_FireBlockSelectionChanged(MP_Player player, MP_Block* block, unsigned short x, unsigned short y);

    void MP_Lua_FireBlockDestroyed(MP_Block* block, unsigned short x, unsigned short y);

    void MP_Lua_FireBlockConverted(MP_Block* block, unsigned short x, unsigned short y);

#ifdef	__cplusplus
}
#endif

#endif	/* JOB_EVENTS_H */

