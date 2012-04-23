/* 
 * File:   render.h
 * Author: fnuecke
 *
 * Created on April 23, 2012, 2:11 PM
 */

#ifndef RENDER_H
#define	RENDER_H

#ifdef	__cplusplus
extern "C" {
#endif

/** (Re)initialize openGL */
void DK_init_gl(void);

/** Render the game to the screen */
void DK_render(void);

#ifdef	__cplusplus
}
#endif

#endif	/* RENDER_H */

