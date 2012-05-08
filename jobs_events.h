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

    void MP_FireUnitAdded(MP_Unit* unit);

    void MP_FireBlockSelectionChanged(MP_Block* block, unsigned short x, unsigned short y, bool selected);

    void MP_FireBlockDestroyed(MP_Block* block, unsigned short x, unsigned short y);

    void MP_FireBlockConverted(MP_Block* block, unsigned short x, unsigned short y);

#ifdef	__cplusplus
}
#endif

#endif	/* JOB_EVENTS_H */

