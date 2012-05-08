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
        /** The job this state performs */
        MP_Job* job;

        /** The number of the job the state performs (in the unit's job list) */
        unsigned int jobNumber;

        /** Delay before re-evaluating the job's logic */
        unsigned int delay;

        /**
         * Whether the job should be canceled. Jobs can only be popped by
         * themselves, to avoid stack corruption regardless of the call chain,
         * and this is how we notify them they should do just that.
         */
        bool shouldCancel;
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
        /** AI info stack */
        AI_State stack[MP_AI_STACK_DEPTH];

        /** The current AI state (pointer into stack) */
        AI_State* current;

        /** Current pathing, used when moving */
        AI_Path pathing;
    };

    /**
     * Makes a unit discard its current movement and begin moving to the
     * specified location.
     * @param unit the unit that should move.
     * @param position the position it should move to.
     * @return whether the unit now moves to the specified position.
     */
    bool MP_MoveTo(const MP_Unit* unit, const vec2* position);

    /**
     * Update AI logic for the specified unit.
     * @param unit the unit for which to update the AI.
     */
    void MP_UpdateAI(MP_Unit* unit);

    /**
     * Lua interface for MP_MoveTo.
     */
    int MP_Lua_MoveTo(lua_State* L);

#ifdef	__cplusplus
}
#endif

#endif	/* UNITS_AI_H */

