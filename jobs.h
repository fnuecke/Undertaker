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
    struct DK_Job {
        /** Job type information */
        const DK_JobMeta* meta;

        /** The unit that is currently assigned to that job */
        DK_Unit* worker;

        /** The targeted block, if any */
        DK_Block* block;

        /** The targeted room, if any */
        DK_Room* room;

        /** The targeted unit, if any */
        DK_Unit* unit;

        /** Offset to the target position; absolute, if there is no target */
        vec2 offset;
    };

    /**
     * Find the job of the specified type closest to the specified unit.
     * @param unit the unit to find the job for.
     * @param type the type of job we're looking for.
     * @param the distance to the found job, if any.
     * @return the closest job of that type to the unit. May be null.
     */
    DK_Job* DK_FindJob(const DK_Unit* unit, const DK_JobMeta* type, float* distance);

    /**
     * Run a job's script for the specified unit.
     */
    void DK_RunJob(DK_Unit* unit, const DK_JobMeta* job);

    /**
     * Clear all job lists and free all additional memory.
     */
    void DK_ClearJobs(void);

    /**
     * Initialize job system.
     */
    void DK_InitJobs(void);

#ifdef	__cplusplus
}
#endif

#endif	/* JOBS_H */

