#include "frustum.h"

inline static void setPlaneCoefficients(plane* p, float a, float b, float c, float d) {
    v3set(&p->n, a, b, c);
    {
        const float len = v3len(&p->n);
        v3idivs(&p->n, len);
        p->d = d / len;
    }
}

static void computeIntersectionLine(vec3* o, vec3* d, const plane* p1, const plane* p2) {
    float dot;
    vec3 v1, v2;
    v3muls(&v1, &p1->n, p2->d);
    v3muls(&v2, &p2->n, -p1->d);
    v3iadd(&v1, &v2);
    v3cross(d, &p1->n, &p2->n);
    dot = v3norm(d);
    v3cross(o, &v1, d);
    v3idivs(o, dot);
}

static void computeIntersectionPoint(vec3* result, const plane* p, const vec3* o, const vec3* d) {
    v3muls(result, d, (-p->d - v3dot(&p->n, o)) / v3dot(&p->n, d));
    v3iadd(result, o);
}

void MP_FrustumFromMatrix(frustum* f, const mat4* m) {
    vec3 o, d;
#define M(row, col) m->m[col * 4 + row]
    setPlaneCoefficients(&f->sides[FRUSTUM_SIDE_NEAR],
                         -M(3, 0) - M(2, 0),
                         -M(3, 1) - M(2, 1),
                         -M(3, 2) - M(2, 2),
                         -M(3, 3) - M(2, 3));
    setPlaneCoefficients(&f->sides[FRUSTUM_SIDE_FAR],
                         -M(3, 0) + M(2, 0),
                         -M(3, 1) + M(2, 1),
                         -M(3, 2) + M(2, 2),
                         -M(3, 3) + M(2, 3));
    setPlaneCoefficients(&f->sides[FRUSTUM_SIDE_LEFT],
                         -M(3, 0) - M(0, 0),
                         -M(3, 1) - M(0, 1),
                         -M(3, 2) - M(0, 2),
                         -M(3, 3) - M(0, 3));
    setPlaneCoefficients(&f->sides[FRUSTUM_SIDE_RIGHT],
                         -M(3, 0) + M(0, 0),
                         -M(3, 1) + M(0, 1),
                         -M(3, 2) + M(0, 2),
                         -M(3, 3) + M(0, 3));
    setPlaneCoefficients(&f->sides[FRUSTUM_SIDE_TOP],
                         -M(3, 0) + M(1, 0),
                         -M(3, 1) + M(1, 1),
                         -M(3, 2) + M(1, 2),
                         -M(3, 3) + M(1, 3));
    setPlaneCoefficients(&f->sides[FRUSTUM_SIDE_BOTTOM],
                         -M(3, 0) - M(1, 0),
                         -M(3, 1) - M(1, 1),
                         -M(3, 2) - M(1, 2),
                         -M(3, 3) - M(1, 3));
#undef M
    computeIntersectionLine(&o, &d, &f->sides[FRUSTUM_SIDE_NEAR], &f->sides[FRUSTUM_SIDE_LEFT]);
    computeIntersectionPoint(&f->corners[FRUSTUM_CORNER_NEAR_TOP_LEFT], &f->sides[FRUSTUM_SIDE_TOP], &o, &d);
    computeIntersectionPoint(&f->corners[FRUSTUM_CORNER_NEAR_BOTTOM_LEFT], &f->sides[FRUSTUM_SIDE_BOTTOM], &o, &d);
    computeIntersectionLine(&o, &d, &f->sides[FRUSTUM_SIDE_RIGHT], &f->sides[FRUSTUM_SIDE_NEAR]);
    computeIntersectionPoint(&f->corners[FRUSTUM_CORNER_NEAR_TOP_RIGHT], &f->sides[FRUSTUM_SIDE_TOP], &o, &d);
    computeIntersectionPoint(&f->corners[FRUSTUM_CORNER_NEAR_BOTTOM_RIGHT], &f->sides[FRUSTUM_SIDE_BOTTOM], &o, &d);
    computeIntersectionLine(&o, &d, &f->sides[FRUSTUM_SIDE_LEFT], &f->sides[FRUSTUM_SIDE_FAR]);
    computeIntersectionPoint(&f->corners[FRUSTUM_CORNER_FAR_TOP_LEFT], &f->sides[FRUSTUM_SIDE_TOP], &o, &d);
    computeIntersectionPoint(&f->corners[FRUSTUM_CORNER_FAR_BOTTOM_LEFT], &f->sides[FRUSTUM_SIDE_BOTTOM], &o, &d);
    computeIntersectionLine(&o, &d, &f->sides[FRUSTUM_SIDE_FAR], &f->sides[FRUSTUM_SIDE_RIGHT]);
    computeIntersectionPoint(&f->corners[FRUSTUM_CORNER_FAR_TOP_RIGHT], &f->sides[FRUSTUM_SIDE_TOP], &o, &d);
    computeIntersectionPoint(&f->corners[FRUSTUM_CORNER_FAR_BOTTOM_RIGHT], &f->sides[FRUSTUM_SIDE_BOTTOM], &o, &d);
}

static void computePlaneSide(frustum* f, unsigned int side, unsigned int a, unsigned int b, unsigned int c) {
    vec3 ab, ac;
    v3sub(&ab, &f->corners[b], &f->corners[a]);
    v3sub(&ac, &f->corners[c], &f->corners[a]);
    v3cross(&f->sides[side].n, &ab, &ac);
    v3inormalize(&f->sides[side].n);
    f->sides[side].d = -v3dot(&f->sides[side].n, &f->corners[a]);
}

void MP_FrustumSegment(frustum* result, const frustum* f, float u0, float v0, float u1, float v1) {
    // For storing results of first axis interpolation.
    frustum tmp;
    // Interpolate first axis.
    v3lerp(&tmp.corners[FRUSTUM_CORNER_NEAR_TOP_LEFT],
           &f->corners[FRUSTUM_CORNER_NEAR_TOP_LEFT],
           &f->corners[FRUSTUM_CORNER_NEAR_TOP_RIGHT], u0);
    v3lerp(&tmp.corners[FRUSTUM_CORNER_NEAR_TOP_RIGHT],
           &f->corners[FRUSTUM_CORNER_NEAR_TOP_LEFT],
           &f->corners[FRUSTUM_CORNER_NEAR_TOP_RIGHT], u1);

    v3lerp(&tmp.corners[FRUSTUM_CORNER_NEAR_BOTTOM_RIGHT],
           &f->corners[FRUSTUM_CORNER_NEAR_BOTTOM_LEFT],
           &f->corners[FRUSTUM_CORNER_NEAR_BOTTOM_RIGHT], u1);
    v3lerp(&tmp.corners[FRUSTUM_CORNER_NEAR_BOTTOM_LEFT],
           &f->corners[FRUSTUM_CORNER_NEAR_BOTTOM_LEFT],
           &f->corners[FRUSTUM_CORNER_NEAR_BOTTOM_RIGHT], u0);

    v3lerp(&tmp.corners[FRUSTUM_CORNER_FAR_TOP_LEFT],
           &f->corners[FRUSTUM_CORNER_FAR_TOP_LEFT],
           &f->corners[FRUSTUM_CORNER_FAR_TOP_RIGHT], u0);
    v3lerp(&tmp.corners[FRUSTUM_CORNER_FAR_TOP_RIGHT],
           &f->corners[FRUSTUM_CORNER_FAR_TOP_LEFT],
           &f->corners[FRUSTUM_CORNER_FAR_TOP_RIGHT], u1);

    v3lerp(&tmp.corners[FRUSTUM_CORNER_FAR_BOTTOM_RIGHT],
           &f->corners[FRUSTUM_CORNER_FAR_BOTTOM_LEFT],
           &f->corners[FRUSTUM_CORNER_FAR_BOTTOM_RIGHT], u1);
    v3lerp(&tmp.corners[FRUSTUM_CORNER_FAR_BOTTOM_LEFT],
           &f->corners[FRUSTUM_CORNER_FAR_BOTTOM_LEFT],
           &f->corners[FRUSTUM_CORNER_FAR_BOTTOM_RIGHT], u0);

    // Interpolate second axis.
    v3lerp(&result->corners[FRUSTUM_CORNER_NEAR_TOP_LEFT],
           &tmp.corners[FRUSTUM_CORNER_NEAR_TOP_LEFT],
           &tmp.corners[FRUSTUM_CORNER_NEAR_BOTTOM_LEFT], v0);
    v3lerp(&result->corners[FRUSTUM_CORNER_NEAR_TOP_RIGHT],
           &tmp.corners[FRUSTUM_CORNER_NEAR_TOP_RIGHT],
           &tmp.corners[FRUSTUM_CORNER_NEAR_BOTTOM_RIGHT], v0);

    v3lerp(&result->corners[FRUSTUM_CORNER_NEAR_BOTTOM_LEFT],
           &tmp.corners[FRUSTUM_CORNER_NEAR_TOP_LEFT],
           &tmp.corners[FRUSTUM_CORNER_NEAR_BOTTOM_LEFT], v1);
    v3lerp(&result->corners[FRUSTUM_CORNER_NEAR_BOTTOM_RIGHT],
           &tmp.corners[FRUSTUM_CORNER_NEAR_TOP_RIGHT],
           &tmp.corners[FRUSTUM_CORNER_NEAR_BOTTOM_RIGHT], v1);

    v3lerp(&result->corners[FRUSTUM_CORNER_FAR_TOP_LEFT],
           &tmp.corners[FRUSTUM_CORNER_FAR_TOP_LEFT],
           &tmp.corners[FRUSTUM_CORNER_FAR_BOTTOM_LEFT], v0);
    v3lerp(&result->corners[FRUSTUM_CORNER_FAR_TOP_RIGHT],
           &tmp.corners[FRUSTUM_CORNER_FAR_TOP_RIGHT],
           &tmp.corners[FRUSTUM_CORNER_FAR_BOTTOM_RIGHT], v0);

    v3lerp(&result->corners[FRUSTUM_CORNER_FAR_BOTTOM_LEFT],
           &tmp.corners[FRUSTUM_CORNER_FAR_TOP_LEFT],
           &tmp.corners[FRUSTUM_CORNER_FAR_BOTTOM_LEFT], v1);
    v3lerp(&result->corners[FRUSTUM_CORNER_FAR_BOTTOM_RIGHT],
           &tmp.corners[FRUSTUM_CORNER_FAR_TOP_RIGHT],
           &tmp.corners[FRUSTUM_CORNER_FAR_BOTTOM_RIGHT], v1);

    // Compute plane equations.
    computePlaneSide(result, FRUSTUM_SIDE_NEAR,
                     FRUSTUM_CORNER_NEAR_TOP_LEFT,
                     FRUSTUM_CORNER_NEAR_BOTTOM_LEFT,
                     FRUSTUM_CORNER_NEAR_TOP_RIGHT);
    computePlaneSide(result, FRUSTUM_SIDE_FAR,
                     FRUSTUM_CORNER_FAR_TOP_LEFT,
                     FRUSTUM_CORNER_FAR_TOP_RIGHT,
                     FRUSTUM_CORNER_FAR_BOTTOM_LEFT);
    computePlaneSide(result, FRUSTUM_SIDE_LEFT,
                     FRUSTUM_CORNER_NEAR_TOP_LEFT,
                     FRUSTUM_CORNER_FAR_TOP_LEFT,
                     FRUSTUM_CORNER_NEAR_BOTTOM_LEFT);
    computePlaneSide(result, FRUSTUM_SIDE_RIGHT,
                     FRUSTUM_CORNER_NEAR_TOP_RIGHT,
                     FRUSTUM_CORNER_NEAR_BOTTOM_RIGHT,
                     FRUSTUM_CORNER_FAR_TOP_RIGHT);
    computePlaneSide(result, FRUSTUM_SIDE_TOP,
                     FRUSTUM_CORNER_NEAR_TOP_LEFT,
                     FRUSTUM_CORNER_NEAR_TOP_RIGHT,
                     FRUSTUM_CORNER_FAR_TOP_LEFT);
    computePlaneSide(result, FRUSTUM_SIDE_BOTTOM,
                     FRUSTUM_CORNER_NEAR_BOTTOM_LEFT,
                     FRUSTUM_CORNER_FAR_BOTTOM_LEFT,
                     FRUSTUM_CORNER_NEAR_BOTTOM_RIGHT);
}

static float planeDistance(const plane* p, const vec3* v) {
    return v3dot(&p->n, v) + p->d;
}

bool MP_IsPointInFrustum(const frustum* f, const vec3* p) {
    for (int i = 0; i < 6; i++) {
        if (planeDistance(&f->sides[i], p) > 0) {
            return false;
        }
    }
    return true;
}

bool MP_IsSphereInFrustum(const frustum* f, const vec3* center, float radius) {
    for (int i = 0; i < 6; i++) {
        if (planeDistance(&f->sides[i], center) > radius) {
            return false;
        }
    }
    return true;
}

/*
bool MP_IsBoxInFrustum(const frustum* f, AABox &b) {
    for (int i = 0; i < 6; i++) {
        if (planeDistance(&f->sides[i], b.getVertexP(f->sides[i].normal)) < 0)
            return false;
    }
    return true;
}
 */
