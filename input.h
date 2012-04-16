/* 
 * File:   input.h
 * Author: fnuecke
 *
 * Created on April 16, 2012, 1:25 PM
 */

#ifndef INPUT_H
#define	INPUT_H

#ifdef	__cplusplus
extern "C" {
#endif

/** Handle key press */
void DK_key_down(const SDL_Event* e);

/** Handle key release */
void DK_key_up(const SDL_Event* e);
    
/** Handle mouse down */
void DK_mouse_down(const SDL_Event* e);

/** Handle mouse up */
void DK_mouse_up(const SDL_Event* e);

#ifdef	__cplusplus
}
#endif

#endif	/* INPUT_H */

