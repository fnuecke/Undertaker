/* 
 * Author: fnuecke
 *
 * Created on April 18, 2012, 4:04 PM
 */

#ifndef JOB_H
#define	JOB_H

#include "types.h"
#include "vmath.h"

#ifdef	__cplusplus
extern "C" {
#endif

    /**
     * Jobs are possible ways for a unit to spend its time, and generally is
     * directly related to a single AI state. Jobs come in various basic types:
     * - Jobs related to blocks (digging, converting)
     * - Jobs related to units (attack, guard, ...)
     * - Jobs related to rooms (train, research, craft, torture, pray, ...)
     * 
     * Block, unit and room related jobs are tracked globally. I.e. the
     * 'potential' of working these jobs is held in memory, so that when a unit
     * looks for a job of a certain type it is a simple list lookup (search for
     * the closest one, usually).
     * 
     * For example, unclaimed blocks will always have a 'convert' job opening,
     * selected ones a 'dig' opening, enemy units will always have an 'attack'
     * opening, and so on.
     * 
     * Most jobs (essentially all except unit related ones) are restricted to
     * one active worker. This worker registers itself with the job opening, so
     * that other units will know not to try and take it, if they're further
     * away than the active worker.
     * 
     * Aside from the distance to a job, a units desires and needs also control
     * the actual choice in job a unit decides on.
     */

    /** Defines possible target types for a job */
    typedef enum {
        /** Job does not target anything in particular */
        MP_JOB_TARGET_NONE,

        /** Job targets a block */
        MP_JOB_TARGET_BLOCK,

        /** Job targets a room */
        MP_JOB_TARGET_ROOM,

        /** Job targets a unit */
        MP_JOB_TARGET_UNIT
    } MP_JobTargetType;

    /** Data for a single job instance */
    struct MP_Job {
        /** Job type information */
        const MP_JobType* type;

        /** The unit that is currently assigned to that job */
        MP_Unit* worker;

        /** The type of the targeted object (defines type of the pointer) */
        MP_JobTargetType targetType;

        /** The targeted object, if any */
        void* target;

        /** Offset to the target position; absolute, if there is no target */
        vec2 offset;
    };

    /**
     * Allocates a new job that will be tracked and can be found via the
     * FindJob method.
     * @param player the player for whom to create the job.
     * @param meta the type of the job to create.
     * @return the newly created job.
     */
    MP_Job* MP_NewJob(MP_Player player, const MP_JobType* type);

    /**
     * Deletes a job. This frees the memory the job occupies, so all pointers to
     * it will be invalid after calling this. It will also tell any units
     * working on this job to stop doing so.
     * @param player the player to whom the job belongs.
     * @param job the job to delete.
     */
    void MP_DeleteJob(MP_Player player, MP_Job* job);

    /**
     * Deletes all jobs targeting the specified block. Same effects as for the
     * normal DeleteJob method apply.
     * @param player the player for whom to delete the jobs.
     * @param type the type of job to delete.
     * @param block the targeted block to check for.
     */
    void MP_DeleteJobsTargetingBlock(MP_Player player, const MP_JobType* type, const MP_Block* block);

    /**
     * Deletes all jobs targeting the specified room. Same effects as for the
     * normal DeleteJob method apply.
     * @param player the player for whom to delete the jobs.
     * @param type the type of job to delete.
     * @param room the targeted room to check for.
     */
    void MP_DeleteJobsTargetingRoom(MP_Player player, const MP_JobType* type, const MP_Room* room);

    /**
     * Deletes all jobs targeting the specified unit. Same effects as for the
     * normal DeleteJob method apply.
     * @param player the player for whom to delete the jobs.
     * @param type the type of job to delete.
     * @param unit the targeted unit to check for.
     */
    void MP_DeleteJobsTargetingUnit(MP_Player player, const MP_JobType* type, const MP_Unit* unit);

    /**
     * Get a list of all jobs of the specified type, as well as the size of that
     * list.
     * @param player the player to get the list for.
     * @param meta the job type to get the list for.
     * @param count used to return the length of the list.
     * @return the list of jobs.
     */
    MP_Job* const* MP_GetJobs(MP_Player player, const MP_JobType* meta, unsigned int* count);

    /**
     * Get the actual position of a job, i.e. that of its target including the
     * set offset.
     * @param position the position of the job.
     * @param job the job to check for.
     */
    void MP_GetJobPosition(vec2* position, const MP_Job* job);

    /**
     * Find the job of the specified type closest to the specified unit.
     * @param unit the unit to find the job for.
     * @param type the type of job we're looking for.
     * @param the distance to the found job, if any.
     * @return the closest job of that type to the unit. May be null.
     */
    MP_Job* MP_FindJob(const MP_Unit* unit, const MP_JobType* type, float* distance);

    /**
     * Clear all job lists and free all additional memory.
     */
    void MP_ClearJobs(void);

    /**
     * Runs a script associated with the specified job, for the specified unit.
     * If delay is not null, a delay may be returned this way, after which the
     * job should be executed again, instead of instantly.
     * @param unit the unit to run the job for.
     * @param job the job to run.
     * @param delay if not null, used to return a delay to wait.
     * @return whether the job is active or not (for saturation changes).
     */
    bool MP_RunJob(MP_Unit* unit, MP_Job* job, unsigned int* delay);

    /**
     * Gets a dynamic preference of a unit for a job. This allows units having
     * a job preference based on dynamic parameters (e.g. number of enemies
     * nearby, to trigger a "flee" behavior).
     * @param unit the unit to get the preference for.
     * @param type the job type.
     * @return the weighted preference for that job.
     */
    float MP_GetDynamicPreference(MP_Unit* unit, const MP_JobType* type);

    /**
     * Initialize job system.
     */
    void MP_InitJobs(void);
    void MP_Debug_InitJobs(void);

#ifdef	__cplusplus
}
#endif

#endif
