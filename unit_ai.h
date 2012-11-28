/* 
 * Author: fnuecke
 *
 * Created on May 4, 2012, 4:37 PM
 */

#ifndef UNIT_AI_H
#define	UNIT_AI_H

#include "astar.h"
#include "config.h"
#include "types.h"

#ifdef	__cplusplus
extern "C" {
#endif

    /** A single entry in a unit's AI stack */
    typedef struct {
        /** Updates to wait before performing the next job search */
        unsigned int jobSearchDelay;

        /** Delay before re-evaluating the job's logic */
        unsigned int jobRunDelay;

        /** The actual job (workplace) we are active at */
        MP_Job* job;

        /** Whether the job is active (saturation rises or sinks) */
        bool active;
    } AI_State;

    /** Pathing information for traveling along a path */
    typedef struct AI_Path {
        /** The path the unit currently follows (if moving) */
        vec2 nodes[MP_AI_PATH_DEPTH + 2];

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
    struct MP_AI_Info {
        /** Whether the unit is currently in its owners hand */
        bool isInHand;

        /** The current AI state */
        AI_State state;

        /** Current pathing, used when moving */
        AI_Path pathing;
    };

    /**
     * Makes a unit discard its current movement and begin moving to the
     * specified location.
     * @param unit the unit that should move.
     * @param position the position it should move to.
     * @return the estimated time in seconds it takes the unit to reach the
     * target position, or an unspecified negative value on failure to find a
     * path.
     */
    float MP_MoveTo(const MP_Unit* unit, const vec2* position);

    /**
     * Update AI logic for the specified unit.
     * @param unit the unit for which to update the AI.
     */
    void MP_UpdateAI(MP_Unit* unit);

    /**
     * Render pathing information for units (debug only).
     */
    void MP_RenderPathing(const MP_Unit* unit);

#ifdef	__cplusplus
}
#endif

#endif
