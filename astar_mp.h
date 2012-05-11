/* 
 * File:   astar_mp.h
 * Author: fnuecke
 *
 * Created on May 12, 2012, 12:47 AM
 */

#ifndef ASTAR_MP_H
#define	ASTAR_MP_H

#include "types.h"
#include "vmath.h"

#ifdef	__cplusplus
extern "C" {
#endif

    /**
     * Performs an A* path search using JPS.
     * @param unit the unit to find a path for.
     * @param goal the target position as a fraction of map coordinates.
     * @param path used to return the found path.
     * @param depth the number of path nodes that can be returned via path.
     * @param length the length of the found path.
     * @return 1 if a path was found, 0 if there was no path to the target.
     */
    bool MP_AStar(const MP_Unit* unit, const vec2* goal,
            vec2* path, unsigned int* depth, float* length);

#ifdef	__cplusplus
}
#endif

#endif	/* ASTAR_MP_H */
