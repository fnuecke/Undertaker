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

/**
 * Get the current cursor coordinate.
 * @return the current cursor coordinate on the x/y plane.
 */
const vec2* DK_GetCursor(void);

/**
 * Update the cursor coordinate. This should be called each frame.
 */
void DK_UpdateCursor(void);

#ifdef	__cplusplus
}
#endif

#endif	/* CURSOR_H */

