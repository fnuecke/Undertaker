/* 
 * File:   picking.h
 * Author: fnuecke
 *
 * Created on April 18, 2012, 3:19 AM
 */

#ifndef PICKING_H
#define	PICKING_H

#include <GL/glew.h>

#ifdef	__cplusplus
extern "C" {
#endif

    /**
     * Pick the topmost object at the specified cursor position.
     * @param x the x coordinate of the cursor.
     * @param y the y coordinate of the cursor.
     * @param render the method to use for rendering objects while in select mode.
     * @return the name (identifier) of the object that was picked.
     */
    GLuint DK_Pick(int x, int y, void(*render)(void));

#ifdef	__cplusplus
}
#endif

#endif	/* PICKING_H */
