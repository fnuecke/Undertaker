/* 
 * File:   jobs_meta.h
 * Author: fnuecke
 *
 * Created on May 7, 2012, 4:38 PM
 */

#ifndef META_JOB_H
#define	META_JOB_H

#include "meta.h"
#include "types.h"

#ifdef	__cplusplus
extern "C" {
#endif

    struct MP_JobMeta {
        /** The ID of this job type */
        unsigned int id;

        /** The name of this job type */
        const char* name;

        /** The script used to handle job logic */
        lua_State* L;

        /** List to check which events are handled by the script */
        bool handlesEvent[MP_JOB_EVENT_COUNT];

        /** Whether the script should be queried for a current saturation */
        bool hasDynamicPreference;

        /** Whether the script has a run method (and thus: can be active) */
        bool hasRunMethod;
    };

    META_header(MP_JobMeta, Job);

    /**
     * Utility method to disable a single event callback for a specific job meta.
     * @param meta the meta to modify.
     */
    void MP_DisableJobEvent(const MP_JobMeta* meta, MP_JobEvent event);

    /**
     * Utility method to disable the dynamic preference for a specific job meta.
     * @param meta the meta to modify.
     */
    void MP_DisableDynamicPreference(const MP_JobMeta* meta);

    /**
     * Utility method to disable the run method for a specific job meta.
     * @param meta the meta to modify.
     */
    void MP_DisableRunMethod(const MP_JobMeta* meta);

#ifdef	__cplusplus
}
#endif

#endif	/* META_JOB_H */
