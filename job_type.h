/* 
 * Author: fnuecke
 *
 * Created on May 7, 2012, 4:38 PM
 */

#ifndef JOB_TYPE_H
#define	JOB_TYPE_H

#include "type.h"
#include "types.h"

#ifdef	__cplusplus
extern "C" {
#endif

    /** Information common to all instances of this job type */
    struct MP_JobType {
        /** The type information */
        MP_Type info;

        /** Reference to the script's run method */
        int runMethod;

        /** Reference to the function to be queried for a current saturation */
        int dynamicPreference;
    };

    TYPE_HEADER(MP_JobType, Job);

    /**
     * Get the preference of a unit for the specified job. This will return the
     * fixed value if the meta has no dynamic callback, else get the value from
     * the callback.
     * @param unit the unit to get the preference for.
     * @param meta the job type to get the preference for.
     * @return the preference for the job.
     */
    float MP_GetJobPreference(const MP_Unit* unit, const MP_JobType* meta);

    /**
     * Utility method to disable the run method for a specific job meta.
     * @param meta the meta to modify.
     */
    void MP_DisableJobRunMethod(const MP_JobType* meta);

    /**
     * Utility method to disable the dynamic preference for a specific job meta.
     * @param meta the meta to modify.
     */
    void MP_DisableDynamicPreference(const MP_JobType* meta);

#ifdef	__cplusplus
}
#endif

#endif
