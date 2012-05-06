/* 
 * File:   graphics.h
 * Author: fnuecke
 *
 * Created on April 25, 2012, 5:41 PM
 */

#ifndef GRAPHICS_H
#define	GRAPHICS_H

#include "callbacks.h"
#include "vmath.h"

#ifdef	__cplusplus
extern "C" {
#endif

    ///////////////////////////////////////////////////////////////////////////
    // Matrix access
    ///////////////////////////////////////////////////////////////////////////

    const mat4* DK_GetModelMatrix(void);

    const mat4* DK_GetViewMatrix(void);

    const mat4* DK_GetProjectionMatrix(void);

    const mat4* DK_GetModelViewMatrix(void);

    const mat4* DK_GetModelViewProjectionMatrix(void);

    int DK_PushModelMatrix(void);

    int DK_PopModelMatrix(void);

    void DK_SetModelMatrix(const mat4* m);

    void DK_ScaleModelMatrix(float sx, float sy, float sz);

    void DK_TranslateModelMatrix(float tx, float ty, float tz);

    ///////////////////////////////////////////////////////////////////////////
    // Projection and view matrix generation
    ///////////////////////////////////////////////////////////////////////////

    int DK_BeginPerspective(void);

    int DK_EndPerspective(void);

    int DK_BeginOrthogonal(void);

    int DK_EndOrthogonal(void);

    int DK_BeginPerspectiveForPicking(float x, float y);

    int DK_BeginLookAt(float eyex, float eyey, float eyez, float lookatx, float lookaty, float lookatz);

    int DK_EndLookAt(void);

    ///////////////////////////////////////////////////////////////////////////
    // Projection
    ///////////////////////////////////////////////////////////////////////////

    int DK_Project(float objx, float objy, float objz,
            float *winx, float *winy, float *winz);

    int DK_UnProject(float winx, float winy, float winz,
            float *objx, float *objy, float *objz);

    ///////////////////////////////////////////////////////////////////////////
    // Events
    ///////////////////////////////////////////////////////////////////////////

    /**
     * Initializes the graphics environment.
     */
    void DK_InitGraphics(void);

    /**
     * Register a method to be notified whenever the model matrix changes.
     * @param method the method to call when the model matrix changes.
     */
    void DK_OnModelMatrixChanged(callback method);

#ifdef	__cplusplus
}
#endif

#endif	/* GRAPHICS_H */

