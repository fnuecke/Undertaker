/* 
 * File:   jobs.h
 * Author: fnuecke
 *
 * Created on April 18, 2012, 4:04 PM
 */

#ifndef JOBS_H
#define	JOBS_H

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

    /** Data for a single job */
    struct MP_Job {
        /** Job type information */
        const MP_JobMeta* meta;

        /** The unit that is currently assigned to that job */
        MP_Unit* worker;

        /** The targeted block, if any */
        MP_Block* block;

        /** The targeted room, if any */
        MP_Room* room;

        /** The targeted unit, if any */
        MP_Unit* unit;

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
    MP_Job* MP_NewJob(MP_Player player, const MP_JobMeta* meta);

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
     * @param meta the type of job to delete.
     * @param block the targeted block to check for.
     */
    void MP_DeleteJobsTargetingBlock(MP_Player player, const MP_JobMeta* meta, const MP_Block* block);

    /**
     * Deletes all jobs targeting the specified room. Same effects as for the
     * normal DeleteJob method apply.
     * @param player the player for whom to delete the jobs.
     * @param meta the type of job to delete.
     * @param room the targeted room to check for.
     */
    void MP_DeleteJobsTargetingRoom(MP_Player player, const MP_JobMeta* meta, const MP_Room* room);

    /**
     * Deletes all jobs targeting the specified unit. Same effects as for the
     * normal DeleteJob method apply.
     * @param player the player for whom to delete the jobs.
     * @param meta the type of job to delete.
     * @param unit the targeted unit to check for.
     */
    void MP_DeleteJobsTargetingUnit(MP_Player player, const MP_JobMeta* meta, const MP_Unit* unit);

    /**
     * Get a list of all jobs of the specified type, as well as the size of that
     * list.
     * @param player the player to get the list for.
     * @param meta the job type to get the list for.
     * @param count used to return the length of the list.
     * @return the list of jobs.
     */
    MP_Job * const* MP_GetJobs(MP_Player player, const MP_JobMeta* meta, unsigned int* count);

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
    MP_Job* MP_FindJob(const MP_Unit* unit, const MP_JobMeta* type, float* distance);

    /**
     * Clear all job lists and free all additional memory.
     */
    void MP_ClearJobs(void);

    /**
     * Initialize job system.
     */
    void MP_InitJobs(void);

#ifdef	__cplusplus
}
#endif

#endif	/* JOBS_H */

