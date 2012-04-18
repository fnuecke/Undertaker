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

typedef enum {
    DK_JOB_DIG,
    DK_JOB_CONVERT
} DK_JobType;

/** A slot for imp work (digging, converting) */
typedef struct {
    /** The targeted block */
    DK_Block* block;

    /** Coordinates of the workplace in A* coordinates */
    float x, y;

    /** The type of the job */
    DK_JobType type;

    /** The imp that wants to work here */
    struct DK_Unit* worker;
} DK_Job;

#ifdef	__cplusplus
extern "C" {
#endif

/** (Re)Initialize data structures on map change */
void DK_init_jobs();

/** Display hints for job slots (e.g. debugging) */
void DK_render_jobs();

/**
 * Create job(s) for the block at the specified coordinates.
 * If the block is minable adjacent free slots will be marked, else, if the
 * block is adjacent to an owned block, it will be marked for conversion. For
 * open tiles this will create one job (at the center of the block), for
 * occupied ones (must be ensured to be unselected dirt only!) this will
 * generate 3 * [open walls] jobs.
 */
void DK_jobs_create(DK_Player player, unsigned short x, unsigned short y);

/**
 * Invert operation of create, removes all adjacent jobs that target the block
 * at the specified coordinates.
 */
void DK_jobs_destroy(DK_Player player, unsigned short x, unsigned short y);

/** Get a list of all jobs for the specified player */
DK_Job** DK_jobs(DK_Player player, unsigned int* count);

#ifdef	__cplusplus
}
#endif

#endif	/* JOBS_H */

