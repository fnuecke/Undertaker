/* 
 * File:   units_ai.h
 * Author: fnuecke
 *
 * Created on May 4, 2012, 4:37 PM
 */

#ifndef UNITS_AI_H
#define	UNITS_AI_H

#include "astar.h"
#include "config.h"
#include "types.h"

#ifdef	__cplusplus
extern "C" {
#endif

    /** A single entry in a unit's AI stack */
    typedef struct {
        /** The job the unit currently performs */
        DK_JobType jobType;

        /** The job information on the job the unit currently performs */
        DK_Job* jobInfo;

        /** Delay before re-evaluating the job's logic */
        unsigned int delay;

        /**
         * Whether the job should be canceled. Jobs can only be popped by
         * themselves, to avoid stack corruption regardless of the call chain, and
         * this is how we notify them they should do just that.
         */
        char shouldCancel;
    } AI_State;

    /** Pathing information for traveling along a path */
    typedef struct AI_Path {
        /** The path the unit currently follows (if moving) */
        DK_AStarWaypoint nodes[DK_AI_PATH_DEPTH + 2];

        /** The total depth of the path (number of nodes) */
        unsigned int depth;

        /** The current node of the path */
        unsigned int index;

        /** Distance to next node in the path */
        float distance;

        /** Distance already traveled to the next node */
        float traveled;
    } AI_Path;

    /**
     * AI information struct. Each unit has one. Unit AI works as a stack/state
     * machine. The job on top of the stack is executed and may push new jobs, or
     * pop itself when complete.
     */
    struct DK_AI_Info {
        /** AI info stack */
        AI_State stack[DK_AI_STACK_DEPTH];

        /** The current AI state (pointer into stack) */
        AI_State* current;

        /** Current pathing, used when moving */
        AI_Path pathing;
    };

    void DK_UpdateAI(DK_Unit* unit);

#ifdef	__cplusplus
}
#endif

#endif	/* UNITS_AI_H */

