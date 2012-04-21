/* 
 * File:   cursor.h
 * Author: fnuecke
 *
 * Created on April 21, 2012, 3:05 PM
 */

#ifndef CURSOR_H
#define	CURSOR_H

#ifdef	__cplusplus
extern "C" {
#endif

/** Update the cursor coordinate (i.e. where it is on the X/Y plane) */
void DK_update_cursor();

/** Get the current cursor coordinate */
float DK_cursor(float* x, float* y);

#ifdef	__cplusplus
}
#endif

#endif	/* CURSOR_H */

