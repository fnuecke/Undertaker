/* 
 * File:   units_meta.h
 * Author: fnuecke
 *
 * Created on May 5, 2012, 5:18 PM
 */

#ifndef UNITS_META_H
#define	UNITS_META_H

#include "meta.h"
#include "types.h"

#ifdef	__cplusplus
extern "C" {
#endif

    /**
     * Describes a unit's "desire saturation" for a certain job. This controls
     * how distances are weighted when looking for jobs. The saturation is
     * divided into multiple segments: unsatisfied, neutral, satisfied, bored.
     * 
     * This is per job per unit.
     * 
     * When a unit is bored with a job it just doesn't want to do it. It'll
     * rather idle.
     * 
     * When a unit is satisfied, it doesn't really have to do this job. The job
     * will not receive any weighting when looking for a job to perform. The
     * unit will still continue to perform the job if there's nothing else to do
     * though.
     * 
     * When a unit is neutral, the job receives a base weighing based on how
     * satisfied the unit is.
     * 
     * When a unit is unsatisfied, in addition to the weighing (which is still
     * used in case there are multiple unsatisfied desires), all other jobs that
     * the unit is at least undecided with are ignored when looking for a job.
     * 
     * When a unit is angry it will switch to its "anger job". This can range
     * from leaving the dungeon to randomly attacking other minions.
     * 
     * 
     * The job preference that is used for weighting is applied to the distance
     * to a job, when looking for one. It is essentially mapped to the number
     * of tiles that the job may be away further from the unit, so that it's
     * still considered equal to another.
     * For example, if job A is at a distance of 10 (distances are in map space,
     * so this would be 10 blocks), and job B is at a distance of 5. If the
     * preference of job A is 0 and that of job B is 6, then job B would be
     * preferred (as it's 10 - 0 > 15 - 6 -> 10 > 9).
     * 
     * 
     * Note that saturation is always bounded to a range of [0, 1], so the
     * thresholds should lie in this interval.
     */
    struct DK_UnitJobSaturationMeta {
        /** The initial value */
        float initialValue;

        /** How strongly the unit wants to do this job */
        float preference;

        /** The value at which a unit is unsatisfied */
        float unsatisfiedThreshold;

        /** The value at which a unit is satisfied */
        float satisfiedThreshold;

        /** The value at which a unit becomes bored with this job */
        float boredThreshold;

        /** How does the saturation change while performing the job? */
        float performingDelta;

        /** How does the saturation change while *not* performing the job? */
        float notPerformingDelta;
    };

    /**
     * General satisfaction information for a unit. Current overall satisfaction
     * is computed by averaging satisfaction values over all jobs, weighted by
     * their preference, and world influences (which should be normalized in a
     * range of 0 to 1, where 0 means unsatisfied and 1 means satisfied).
     */
    struct DK_UnitSatisfactionMeta {
        /** Job related satisfaction data */
        DK_UnitJobSaturationMeta* jobSaturation;

        /** How satisfaction changes when being slapped */
        float slapDelta;

        /** How satisfaction changes while being held in the hand */
        float inHandDelta;

        /** The value at which a unit becomes seriously pissed */
        float angerThreshold;

        /** What the unit does when it gets angry */
        const DK_JobMeta* angerJob;
    };

    /** Description of a single unit type */
    struct DK_UnitMeta {
        /** The ID of this unit type */
        unsigned int id;

        /** The name of this unit type */
        const char* name;

        /** Passability types this unit supports */
        DK_Passability canPass;

        /** Base movement speed of the unit */
        float moveSpeed;

        /** Jobs the unit can perform */
        const DK_JobMeta** jobs;

        /** Number of jobs for this unit */
        unsigned int jobCount;

        /** Information on unit satisfaction */
        DK_UnitSatisfactionMeta satisfaction;
    };

    META_header(DK_UnitMeta, Unit)

#ifdef	__cplusplus
}
#endif

#endif	/* UNITS_META_H */

