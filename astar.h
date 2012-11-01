/* 
 * Author: fnuecke
 *
 * Created on April 18, 2012, 3:07 PM
 */

#ifndef ASTAR_H
#define	ASTAR_H

#include "vmath.h"

///////////////////////////////////////////////////////////////////////////////
// Defines
///////////////////////////////////////////////////////////////////////////////

/**
 * How fine the overlayed block grid for A* is (higher = finer).
 */
#ifndef ASTAR_GRANULARITY
#define ASTAR_GRANULARITY 2
#endif

/**
 * Use jumps point search, skipping distances where possible.
 * D. Harabor and A. Grastien. Online Graph Pruning for Pathfinding on Grid Maps.
 * In National Conference on Artificial Intelligence (AAAI), 2011.
 */
#ifndef ASTAR_JPS
#define ASTAR_JPS 1
#endif

#ifdef	__cplusplus
extern "C" {
#endif

    /**
     * Performs an A* path search using JPS.
     * @param start the starting position of the search.
     * @param goal the goal position of the search.
     * @param passable a method used to check if a cell is passable.
     * @param bounds determines world bounds, to know when to stop.
     * @param path used to return the found path, if not null.
     * @param depth the number of path nodes that can be returned via path, if not null.
     * @param length the length of the found path, if not null.
     * @return 1 if a path was found, 0 if there was no path to the target.
     */
    int AStar(const vec2* start, const vec2* goal,
            int(*passable)(float x, float y), unsigned int bounds,
            vec2* path, unsigned int* depth, float* length);

#ifdef	__cplusplus
}
#endif

#endif