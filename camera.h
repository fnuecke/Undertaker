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

    ///////////////////////////////////////////////////////////////////////////////
    // Types
    ///////////////////////////////////////////////////////////////////////////////

    /**
     * Possible camera movement directions for DK_Camera[Un]SetDirection().
     */
    typedef enum {
        /**
         * No direction.
         */
        DK_CAMERA_DIRECTION_NONE = 0,

        /**
         * Northward / upward direction.
         */
        DK_CAMERA_DIRECTION_NORTH = 1,

        /**
         * Eastward / rightward direction.
         */
        DK_CAMERA_DIRECTION_EAST = 2,

        /**
         * Southward / downward direction.
         */
        DK_CAMERA_DIRECTION_SOUTH = 4,

        /**
         * Westward / leftward direction.
         */
        DK_CAMERA_DIRECTION_WEST = 8
    } DK_CameraDirection;

    ///////////////////////////////////////////////////////////////////////////////
    // Accessors
    ///////////////////////////////////////////////////////////////////////////////

    /**
     * Get the current camera position as in world space.
     * @return the current camera position in world space.
     */
    const vec3* DK_GetCameraPosition(void);

    /**
     * Get the current camera target in world space.
     * @return the current camera target in world space.
     */
    const vec3* DK_GetCameraTarget(void);

    /**
     * Get the current camera zoom.
     * @return the current camera zoom, as a percentage.
     */
    float DK_GetCameraZoom(void);

    ///////////////////////////////////////////////////////////////////////////////
    // Manipulation
    ///////////////////////////////////////////////////////////////////////////////

    /**
     * Make the camera move in the specified direction.
     * @param direction the direction to start scrolling in.
     */
    void DK_CameraStartScrolling(DK_CameraDirection direction);

    /**
     * Stop the camera move in the specified direction.
     * @param direction the direction to stop scrolling in.
     */
    void DK_CameraStopScrolling(DK_CameraDirection direction);

    /**
     * Zoom the camera in (closer to the map).
     */
    void DK_CameraZoomIn(void);

    /**
     * Zoom the camera out (further away from the map).
     */
    void DK_CameraZoomOut(void);

    ///////////////////////////////////////////////////////////////////////////////
    // Updating
    ///////////////////////////////////////////////////////////////////////////////

    /**
     * Set up camera events and logic.
     */
    void DK_InitCamera(void);

#ifdef	__cplusplus
}
#endif

#endif	/* CAMERA_H */

