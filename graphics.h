/* 
 * Author: fnuecke
 *
 * Created on April 25, 2012, 5:41 PM
 */

#ifndef GRAPHICS_H
#define	GRAPHICS_H

#include "vmath.h"

#ifdef	__cplusplus
extern "C" {
#endif

    ///////////////////////////////////////////////////////////////////////////
    // Matrix access
    ///////////////////////////////////////////////////////////////////////////

    const mat4* MP_GetModelMatrix(void);

    const mat4* MP_GetViewMatrix(void);

    const mat4* MP_GetProjectionMatrix(void);

    const mat4* MP_GetModelViewMatrix(void);

    const mat4* MP_GetModelViewProjectionMatrix(void);

    int MP_PushModelMatrix(void);

    int MP_PopModelMatrix(void);

    void MP_SetModelMatrix(const mat4* m);

    void MP_ScaleModelMatrix(float sx, float sy, float sz);

    void MP_TranslateModelMatrix(float tx, float ty, float tz);

    ///////////////////////////////////////////////////////////////////////////
    // Projection and view matrix generation
    ///////////////////////////////////////////////////////////////////////////

    int MP_BeginPerspective(void);

    int MP_EndPerspective(void);

    int MP_BeginOrthogonal(void);

    int MP_EndOrthogonal(void);

    int MP_BeginPerspectiveForPicking(float x, float y);

    int MP_BeginLookAt(float eyex, float eyey, float eyez, float lookatx, float lookaty, float lookatz);

    int MP_EndLookAt(void);

    ///////////////////////////////////////////////////////////////////////////
    // Projection
    ///////////////////////////////////////////////////////////////////////////

    int MP_Project(float objx, float objy, float objz,
            float *winx, float *winy, float *winz);

    int MP_UnProject(float winx, float winy, float winz,
            float *objx, float *objy, float *objz);

    ///////////////////////////////////////////////////////////////////////////
    // Events
    ///////////////////////////////////////////////////////////////////////////

    /**
     * Initializes the graphics environment.
     */
    void MP_InitGraphics(void);

#ifdef	__cplusplus
}
#endif

#endif
