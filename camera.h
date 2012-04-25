/* 
 * File:   camera.h
 * Author: fnuecke
 *
 * Created on April 15, 2012, 2:21 PM
 */

#ifndef CAMERA_H
#define	CAMERA_H

#include "vmath.h"

#ifdef	__cplusplus
extern "C" {
#endif

/** Possible camera movement directions for DK_camera_[un]set_direction() */
typedef enum {
    DK_CAMD_NONE = 0,
    DK_CAMD_NORTH = 1,
    DK_CAMD_EAST = 2,
    DK_CAMD_SOUTH = 4,
    DK_CAMD_WEST = 8
} DK_CameraDirection;

/** Make the camera move in the specified direction */
void DK_camera_set_direction(DK_CameraDirection direction);

/** Stop the camera move in the specified direction */
void DK_camera_unset_direction(DK_CameraDirection direction);

/** Zoom the camera in (closer to the map) */
void DK_camera_zoom_in(void);

/** Zoom the camera out (further away from the map) */
void DK_camera_zoom_out(void);

/** Updates camera position and speed based on set movement direction */
void DK_update_camera(void);

/** Get the current camera position as (x, y) */
const vec2* DK_camera_position(void);

/** Get the current camera zoom */
float DK_camera_zoom(void);

#ifdef	__cplusplus
}
#endif

#endif	/* CAMERA_H */

