/* 
 * File:   jobs_meta.h
 * Author: fnuecke
 *
 * Created on May 7, 2012, 4:38 PM
 */

#ifndef JOBS_META_H
#define	JOBS_META_H

#include "meta.h"
#include "types.h"

#ifdef	__cplusplus
extern "C" {
#endif

    struct DK_JobMeta {
        /** The ID of this job type */
        unsigned int id;

        /** The name of this job type */
        const char* name;

        /** The script used to handle job logic */
        lua_State* L;

        /** List to check which events are handled by the script */
        bool handledEvents[DK_JOB_EVENT_COUNT];

        /** Whether the script has a run method (and thus: can be active) */
        bool hasRunMethod;
    };

    META_header(DK_JobMeta, Job)

#ifdef	__cplusplus
}
#endif

#endif	/* JOBS_META_H */

