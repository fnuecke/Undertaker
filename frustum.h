/* 
 * Author: fnuecke
 *
 * Created on November 26, 2012, 4:11 PM
 */

#ifndef FRUSTUM_H
#define	FRUSTUM_H

#include "types.h"
#include "vmath.h"

#ifdef	__cplusplus
extern "C" {
#endif

    /** Holds data for a single frustum. */
    typedef struct plane {
        /** Distance of the plane from the origin projected on the normal */
        float d;

        /** The surface normal of the plane */
        vec3 n;
    } plane;

    typedef enum {
        FRUSTUM_SIDE_NEAR,
        FRUSTUM_SIDE_FAR,
        FRUSTUM_SIDE_LEFT,
        FRUSTUM_SIDE_RIGHT,
        FRUSTUM_SIDE_TOP,
        FRUSTUM_SIDE_BOTTOM
    } frustum_side;

    typedef enum {
        FRUSTUM_CORNER_NEAR_TOP_LEFT,
        FRUSTUM_CORNER_NEAR_TOP_RIGHT,
        FRUSTUM_CORNER_NEAR_BOTTOM_RIGHT,
        FRUSTUM_CORNER_NEAR_BOTTOM_LEFT,
        FRUSTUM_CORNER_FAR_TOP_LEFT,
        FRUSTUM_CORNER_FAR_TOP_RIGHT,
        FRUSTUM_CORNER_FAR_BOTTOM_RIGHT,
        FRUSTUM_CORNER_FAR_BOTTOM_LEFT,
    } frustum_corner;

    typedef struct frustum {
        plane sides[6];
        vec3 corners[8];
    } frustum;

    /**
     * Initializes the specified frustum to the specified model-view matrix.
     * @param frustum the frustum to initialize.
     * @param m the model view projection matrix.
     */
    void MP_FrustumFromMatrix(frustum* f, const mat4* m);

    /**
     * Generates a frustum that represents a sub-segment of the specified
     * frustum. The sub-frustum is generated by cutting from the left/right and
     * top/bottom ends -- i.e. the specified region is mapped to the near and
     * far plane and the frustum between these two regions is returned.
     * @param result the sub-frustum.
     * @param f the original frustum.
     * @param u0 the relative x starting offset.
     * @param v0 the relative y starting offset.
     * @param u1 the relative x ending offset.
     * @param v1 the relative y ending offset.
     */
    void MP_FrustumSegment(frustum* result, const frustum* f, float u0, float v0, float u1, float v1);
    
    /**
     * Tests whether the specified point is contained within the specified frustum.
     * @param f the frustum to test against.
     * @param p the point to check.
     * @return true if contained, else false.
     */
    bool MP_IsPointInFrustum(const frustum* f, const vec3* p);

    /**
     * Tests whether a sphere intersects with the specified frustum.
     * @param f the frustum to test against.
     * @param center the center of the sphere.
     * @param radius the radius of the sphere.
     * @return true if intersecting, else false.
     */
    bool MP_IsSphereInFrustum(const frustum* f, const vec3* center, float radius);

#ifdef	__cplusplus
}
#endif

#endif

