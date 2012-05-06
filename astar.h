/* 
 * File:   astar.h
 * Author: fnuecke
 *
 * Created on April 18, 2012, 3:07 PM
 */

#ifndef ASTAR_H
#define	ASTAR_H

#include "units.h"

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
    int DK_AStar(const DK_Unit* unit, const vec2* goal, vec2* path, unsigned int* depth, float* length);

    /**
     * (Re)Initializes data structures after a map change.
     */
    void DK_InitAStar(void);

#ifdef	__cplusplus
}
#endif

#endif	/* ASTAR_H */
