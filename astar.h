/* 
 * File:   astar.h
 * Author: fnuecke
 *
 * Created on April 18, 2012, 3:07 PM
 */

#ifndef ASTAR_H
#define	ASTAR_H

#include "units.h"

/** Represents a single waypoint along a path found using A* search */
typedef struct {
    float x, y;
} AStar_Waypoint;

#ifdef	__cplusplus
extern "C" {
#endif

/** (Re)Initializes data structures after a map change */
void DK_init_a_star(void);

/**
 * Performs an A* path search using JPS.
 * @param sx the start x coordinate as a fraction of map coordinates.
 * @param sy the start y coordinate as a fraction of map coordinates.
 * @param tx the target x coordinate as a fraction of map coordinates.
 * @param ty the target y coordinate as a fraction of map coordinates.
 * @param path used to return the found path.
 * @param depth the number of path nodes that can be returned via path.
 * @param length the length of the found path.
 * @return 1 if a path was found, 0 if there was no path to the target.
 */
int DK_a_star(float sx, float sy, float tx, float ty, AStar_Waypoint* path, unsigned int* depth, float* length);

#ifdef	__cplusplus
}
#endif

#endif	/* ASTAR_H */

