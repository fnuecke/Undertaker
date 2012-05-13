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
     * Get the preference of a unit for the specified job. This will return the
     * fixed value if the meta has no dynamic callback, else get the value from
     * the callback.
     * @param unit the unit to get the preference for.
     * @param meta the job type to get the preference for.
     * @return the preference for the job.
     */
    float MP_GetJobPreference(const MP_Unit* unit, const MP_JobMeta* meta);

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

