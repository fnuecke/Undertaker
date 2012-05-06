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
     * Describes a unit's "satisfaction" with a certain job. This controls how
     * distances are weighted when looking for jobs. The satisfaction state can
     * be seen in multiple segments: unsatisfied, undecided, satisfied, bored.
     * 
     * This is per job per unit.
     * 
     * When a unit is bored with a job it just doesn't want to do it. It'll
     * rather wander around the dungeon.
     * 
     * When a unit is satisfied, it doesn't really have to do this job. The job
     * will not receive any weighting when looking for a job to perform. The
     * unit will still continue to perform the job if there's nothing else to do
     * though.
     * 
     * When a unit is undecided the job receives a base weighing that ranges
     * from 0 to the value of the unit's preference for this particular job
     * linearly in the satisfaction range of satisfiedThreshold to
     * unsatisfiedThreshold, respectively.
     * 
     * When a unit is unsatisfied, in addition to the weighing (which is still
     * used in case there are multiple unsatisfied desires), all other jobs that
     * the unit is at least undecided with are ignored when looking for a job.
     * 
     * When a unit is angry it's essentially beyond repair. The unit will switch
     * to its "anger job". This can range from leaving the dungeon to randomly
     * attacking other minions.
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
     * Note that satisfaction is always bounded to a range of [0, 1], so the
     * thresholds should lay in this interval.
     */
    struct DK_UnitJobSatisfactionMeta {
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

        /** How does the satisfaction change while performing the job? */
        float performingDelta;

        /** How does the satisfaction change while *not* performing the job? */
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
        DK_UnitJobSatisfactionMeta jobs[DK_JOB_TYPE_COUNT];

        /** How satisfaction changes when being slapped */
        float slapDelta;

        /** How satisfaction changes while being held in the hand */
        float inHandDelta;

        /** The value at which a unit becomes seriously pissed */
        float angerThreshold;

        /** What the unit does when it gets angry */
        DK_JobType angerJob;
    };

    /** Description of a single unit type */
    struct DK_UnitMeta {
        /** The ID of this unit type */
        unsigned int id;

        /** The name of this unit type */
        const char* name;

        /** Passability types this unit supports */
        DK_Passability passability;

        /** Base movement speed of the unit */
        float moveSpeed;

        /** Jobs the unit can perform */
        bool jobs[DK_JOB_TYPE_COUNT];

        /** Information on unit satisfaction */
        DK_UnitSatisfactionMeta satisfaction;

        /** Abilities of the unit */
        //DK_Ability* abilities;
    };

    META_header(DK_UnitMeta, Unit)

#ifdef	__cplusplus
}
#endif

#endif	/* UNITS_META_H */

