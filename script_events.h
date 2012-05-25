/* 
 * File:   job_names.h
 * Author: fnuecke
 *
 * Created on May 12, 2012, 1:28 AM
 */

#ifndef JOB_NAMES_H
#define	JOB_NAMES_H

#ifdef	__cplusplus
extern "C" {
#endif

    static const char* JOB_EVENT_NAME[MP_JOB_EVENT_COUNT] = {
        [MP_JOB_EVENT_UNIT_ADDED] = "onUnitAdded",
        [MP_JOB_EVENT_BLOCK_SELECTION_CHANGED] = "onBlockSelectionChanged",
        [MP_JOB_EVENT_BLOCK_META_CHANGED] = "onBlockMetaChanged",
        [MP_JOB_EVENT_BLOCK_OWNER_CHANGED] = "onBlockOwnerChanged"
    };

#ifdef	__cplusplus
}
#endif

#endif	/* JOB_NAMES_H */

