#include <math.h>
#include <GL/glew.h>

#include "config.h"
#include "graphics.h"
#include "vmath.h"

/** Matrices we use for resolving vertex positions to screen positions */
static struct {
    /** The current projection transform */
    mat4 projection;

    /** The current view transform */
    mat4 view;

    /** The current model transform */
    mat4 model;

    /** Precomputed model-view transform */
    mat4 mv;

    /** The current model-view-projection transform */
    mat4 mvp;

    /** Precomputed surface normal transform (transpose of inverse of current model transform) */
    mat3 normal;
} matrix;

const mat4* DK_GetModelMatrix(void) {
    return &matrix.model;
}

const mat4* DK_GetViewMatrix(void) {
    return &matrix.view;
}

const mat4* DK_GetProjectionMatrix(void) {
    return &matrix.projection;
}

const mat4* DK_GetModelViewMatrix(void) {
    return &matrix.mv;
}

const mat4* DK_GetModelViewProjectionMatrix(void) {
    return &matrix.mvp;
}

void DK_SetModelMatrix(const mat4* m) {
    matrix.model = *m;

    // Pre-compute model-view transform.
    mmulm(&matrix.mv, &matrix.view, &matrix.model);

    // Pre-compute model-view-projection transform.
    mmulm(&matrix.mvp, &matrix.projection, &matrix.mv);
}

///////////////////////////////////////////////////////////////////////////////
// Projection and view matrix generation
///////////////////////////////////////////////////////////////////////////////

static const float PI = 3.14159265358979323846f;

void DK_SetPerspective(float fov, float ratio, float near, float far) {
    mat4* m = &matrix.projection;
    // range = tan(deg2rad(fov / 2)) * near
    // => near / range = 1 / tan(deg2rad(fov / 2))
    const float f = 1.0f / tanf(fov * (PI / 360.0f));

    *m = IDENTITY_MATRIX4;

    m->m[0] = f / ratio;
    m->m[1 * 4 + 1] = f;
    m->m[2 * 4 + 2] = (far + near) / (near - far);
    m->m[3 * 4 + 2] = (2.0f * far * near) / (near - far);
    m->m[2 * 4 + 3] = -1.0f;
    m->m[3 * 4 + 3] = 0.0f;

    // Pre-compute model-view-projection transform.
    mmulm(&matrix.mvp, &matrix.projection, &matrix.mv);
}

void DK_SetOrthogonal(float left, float right, float bottom, float top, float near, float far) {
    mat4* m = &matrix.projection;
    mat4 ortho;
#define M(row,col) ortho.m[col * 4 + row]
    M(0, 0) = 2.0F / (right - left);
    M(0, 1) = 0.0F;
    M(0, 2) = 0.0F;
    M(0, 3) = -(right + left) / (right - left);

    M(1, 0) = 0.0F;
    M(1, 1) = 2.0F / (top - bottom);
    M(1, 2) = 0.0F;
    M(1, 3) = -(top + bottom) / (top - bottom);

    M(2, 0) = 0.0F;
    M(2, 1) = 0.0F;
    M(2, 2) = -2.0F / (far - near);
    M(2, 3) = -(far + near) / (far - near);

    M(3, 0) = 0.0F;
    M(3, 1) = 0.0F;
    M(3, 2) = 0.0F;
    M(3, 3) = 1.0F;
#undef M

    mimulm(m, &ortho);

    // Pre-compute model-view-projection transform.
    mmulm(&matrix.mvp, &matrix.projection, &matrix.mv);
}

void DK_SetLookAt(float eyex, float eyey, float eyez, float lookatx, float lookaty, float lookatz) {
    mat4* m = &matrix.view;
    vec4 up = {
        {0.0f, 0.0f, 1.0f, 1.0f}
    };
    vec4 direction, right;

    // Compute direction vectors and set up base matrix.
    direction.v[0] = (lookatx - eyex);
    direction.v[1] = (lookaty - eyey);
    direction.v[2] = (lookatz - eyez);
    vinormalize(&direction);

    vcross(&right, &direction, &up);
    vinormalize(&right);

    vcross(&up, &right, &direction);
    vinormalize(&up);

    // First row.
    m->m[0] = right.v[0];
    m->m[4] = right.v[1];
    m->m[8] = right.v[2];
    m->m[12] = 0.0f;

    // Second row.
    m->m[1] = up.v[0];
    m->m[5] = up.v[1];
    m->m[9] = up.v[2];
    m->m[13] = 0.0f;

    // Third row.
    m->m[2] = -direction.v[0];
    m->m[6] = -direction.v[1];
    m->m[10] = -direction.v[2];
    m->m[14] = 0.0f;

    // Fourth row.
    m->m[3] = 0.0f;
    m->m[7] = 0.0f;
    m->m[11] = 0.0f;
    m->m[15] = 1.0f;

    // Then add translation based on eye position.
    mitranslate(m, -eyex, -eyey, -eyez);

    // Pre-compute model-view transform.
    mmulm(&matrix.mv, &matrix.view, &matrix.model);

    // Pre-compute model-view-projection transform.
    mmulm(&matrix.mvp, &matrix.projection, &matrix.mv);
}

///////////////////////////////////////////////////////////////////////////////
// Projection
///////////////////////////////////////////////////////////////////////////////

int DK_Project(float objx, float objy, float objz,
        float *winx, float *winy, float *winz) {
    vec4 in;
    vec4 out;

    in.v[0] = objx;
    in.v[1] = objy;
    in.v[2] = objz;
    in.v[3] = 1.0f;

    //__gluMultMatrixVecd(modelMatrix, in, out);
    mmulv(&out, &in, &matrix.mv);
    //__gluMultMatrixVecd(projMatrix, out, in);
    mmulv(&in, &out, &matrix.projection);

    if (in.v[3] * in.v[3] < 1e-25) {
        return 0;
    }

    in.v[0] /= in.v[3];
    in.v[1] /= in.v[3];
    in.v[2] /= in.v[3];

    /* Map x, y and z to range 0-1 */
    in.v[0] = in.v[0] * 0.5f + 0.5f;
    in.v[1] = in.v[1] * 0.5f + 0.5f;
    in.v[2] = in.v[2] * 0.5f + 0.5f;

    /* Map x,y to viewport */
    in.v[0] = in.v[0] * DK_resolution_x;
    in.v[1] = in.v[1] * DK_resolution_y;

    *winx = in.v[0];
    *winy = in.v[1];
    *winz = in.v[2];

    return 1;
}

int DK_UnProject(float winx, float winy, float winz,
        float *objx, float *objy, float *objz) {
    mat4 mvp = matrix.mvp;
    vec4 in;
    vec4 out;

    if (!miinvert(&mvp)) {
        return 0;
    }

    in.v[0] = winx;
    in.v[1] = winy;
    in.v[2] = winz;
    in.v[3] = 1.0f;

    // Map x and y from window coordinates
    in.v[0] = in.v[0] / DK_resolution_x;
    in.v[1] = in.v[1] / DK_resolution_y;

    // Map to range -1 to 1
    in.v[0] = in.v[0] * 2.0f - 1.0f;
    in.v[1] = in.v[1] * 2.0f - 1.0f;
    in.v[2] = in.v[2] * 2.0f - 1.0f;

    //__gluMultMatrixVecd(finalMatrix, in, out);
    mmulv(&out, &in, &mvp);
    if (out.v[3] * out.v[3] < 1e-25) {
        return 0;
    }

    *objx = out.v[0] / out.v[3];
    *objy = out.v[1] / out.v[3];
    *objz = out.v[2] / out.v[3];

    return 1;
}
