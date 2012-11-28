/* 
 * Author: fnuecke
 *
 * Created on April 18, 2012, 3:19 AM
 */

#ifndef PICKING_H
#define	PICKING_H

#include <GL/glew.h>

#include "types.h"

#ifdef	__cplusplus
extern "C" {
#endif

    /**
     * Pick the topmost object at the specified cursor position.
     * @param x the x coordinate of the cursor.
     * @param y the y coordinate of the cursor.
     * @param render the method to use for rendering objects while in select mode.
     * @param name the name (identifier) of the object that was picked.
     * @param depth the depth of the object in an interval of [0,1].
     * @return whether an object was picked or not.
     */
    bool MP_Pick(int x, int y, void(*render)(void), GLuint* name, float* depth);

#ifdef	__cplusplus
}
#endif

#endif
