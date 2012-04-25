/* 
 * File:   graphics.h
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

    const mat4* DK_GetModelMatrix(void);

    const mat4* DK_GetViewMatrix(void);

    const mat4* DK_GetProjectionMatrix(void);

    const mat4* DK_GetModelViewMatrix(void);

    const mat4* DK_GetModelViewProjectionMatrix(void);

    void DK_SetModelMatrix(const mat4* m);
    
    ///////////////////////////////////////////////////////////////////////////
    // Projection and view matrix generation
    ///////////////////////////////////////////////////////////////////////////

    void DK_SetPerspective(float fov, float ratio, float near, float far);

    void DK_SetOrthogonal(float left, float right, float bottom, float top, float near, float far);

    void DK_SetLookAt(float eyex, float eyey, float eyez, float lookatx, float lookaty, float lookatz);

    ///////////////////////////////////////////////////////////////////////////
    // Projection
    ///////////////////////////////////////////////////////////////////////////

    int DK_Project(float objx, float objy, float objz,
            float *winx, float *winy, float *winz);

    int DK_UnProject(float winx, float winy, float winz,
            float *objx, float *objy, float *objz);

#ifdef	__cplusplus
}
#endif

#endif	/* GRAPHICS_H */

