/* 
 * File:   cursor.h
 * Author: fnuecke
 *
 * Created on April 21, 2012, 3:05 PM
 */

#ifndef CURSOR_H
#define	CURSOR_H

#include "vmath.h"

#ifdef	__cplusplus
extern "C" {
#endif

    /** Describes possible x/y plane heights on which to get cursor position */
    typedef enum {
        /** Position on floor plane, i.e. ground level */
        DK_CURSOR_LEVEL_FLOOR,

        /** Position on top plane, i.e. on top of blocks */
        DK_CURSOR_LEVEL_TOP,

        /** Reserved for array initialization */
        DK_CURSOR_LEVEL_COUNT
    } DK_CursorLevel;

    /**
     * Get the current cursor coordinate.
     * @param level the height on which to get the cursor.
     * @return the current cursor coordinate on the x/y plane.
     */
    const vec2* DK_GetCursor(DK_CursorLevel level);

    /**
     * Initialize cursor bindings.
     */
    void DK_InitCursor(void);

#ifdef	__cplusplus
}
#endif

#endif	/* CURSOR_H */

