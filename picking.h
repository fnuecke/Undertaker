/* 
 * File:   picking.h
 * Author: fnuecke
 *
 * Created on April 18, 2012, 3:19 AM
 */

#ifndef PICKING_H
#define	PICKING_H

#ifdef	__cplusplus
extern "C" {
#endif

/** Pick the topmost object at the specified cursor position */
GLuint DK_pick(int x, int y, void(*render)(void));

#ifdef	__cplusplus
}
#endif

#endif	/* PICKING_H */

