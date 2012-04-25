/* 
 * File:   jobs.h
 * Author: fnuecke
 *
 * Created on April 18, 2012, 4:04 PM
 */

#ifndef JOBS_H
#define	JOBS_H

#include "players.h"
#include "units.h"
#include "map.h"

#ifdef	__cplusplus
extern "C" {
#endif

/**
 * Possible jobs.
 */
typedef enum {
    /**
     * Digging job, targets a block to be destroyed.
     */
    DK_JOB_DIG,

    /**
     * Conversion job, targets a block to be converted (claimed).
     */
    DK_JOB_CONVERT
} DK_JobType;

/**
 * Data for a single job.
 */
typedef struct {
    /**
     * The type of the job.
     */
    DK_JobType type;

    /**
     * The unit that is currently assigned to that job.
     */
    DK_Unit* worker;

    /**
     * The targeted block, if any.
     */
    DK_Block* block;

    /**
     * Coordinates of the workplace in map space.
     */
    float x, y;
} DK_Job;

/**
 * (Re)Initialize data structures on map change.
 */
void DK_InitJobs(void);

/**
 * DEBUGGING FEATURE
 * Display hints for job slots.
 */
void DK_RenderJobs(void);

/**
 * Find and make available jobs at and surrounding the block at the specified
 * coordinate.
 * @param player the player for whom to check.
 * @param the x coordinate of the block in map space.
 * @param the y coordinate of the block in map space.
 */
void DK_FindJobs(DK_Player player, unsigned short x, unsigned short y);

/**
 * Get a list of all jobs for the specified player. This is a list of pointers
 * to the actual job data.
 * @param player the player to get all jobs for.
 * @param count the number of jobs.
 * @return a list of jobs for the specified player.
 */
DK_Job** DK_GetJobs(DK_Player player, unsigned int* count);

#ifdef	__cplusplus
}
#endif

#endif	/* JOBS_H */

