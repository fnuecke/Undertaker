#include <math.h>

#include "vmath.h"

///////////////////////////////////////////////////////////////////////////////
// Vector math
///////////////////////////////////////////////////////////////////////////////

void vcopy(vec4* into, const vec4* v) {
    into->v[0] = v->v[0];
    into->v[1] = v->v[1];
    into->v[2] = v->v[2];
}

void vadd(vec4* sum, const vec4* va, const vec4* vb) {
    sum->v[0] = va->v[0] + vb->v[0];
    sum->v[1] = va->v[1] + vb->v[1];
    sum->v[2] = va->v[2] + vb->v[2];
}

void viadd(vec4* va, const vec4* vb) {
    va->v[0] += vb->v[0];
    va->v[1] += vb->v[1];
    va->v[2] += vb->v[2];
}

void vsub(vec4* difference, const vec4* va, const vec4* vb) {
    difference->v[0] = va->v[0] - vb->v[0];
    difference->v[1] = va->v[1] - vb->v[1];
    difference->v[2] = va->v[2] - vb->v[2];
}

void visub(vec4* va, const vec4* vb) {
    va->v[0] -= vb->v[0];
    va->v[1] -= vb->v[1];
    va->v[2] -= vb->v[2];
}

void vmul(vec4* product, const vec4* va, const vec4* vb) {
    product->v[0] = va->v[0] * vb->v[0];
    product->v[1] = va->v[1] * vb->v[1];
    product->v[2] = va->v[2] * vb->v[2];
}

void vimul(vec4* va, const vec4* vb) {
    va->v[0] *= vb->v[0];
    va->v[1] *= vb->v[1];
    va->v[2] *= vb->v[2];
}

void vmuls(vec4* product, const vec4* v, float s) {
    product->v[0] = v->v[0] * s;
    product->v[1] = v->v[1] * s;
    product->v[2] = v->v[2] * s;
}

void vimuls(vec4* v, float s) {
    v->v[0] *= s;
    v->v[1] *= s;
    v->v[2] *= s;
}

void vdiv(vec4* quotient, const vec4* va, const vec4* vb) {
    quotient->v[0] = va->v[0] / vb->v[0];
    quotient->v[1] = va->v[1] / vb->v[1];
    quotient->v[2] = va->v[2] / vb->v[2];
}

void vidiv(vec4* va, const vec4* vb) {
    va->v[0] /= vb->v[0];
    va->v[1] /= vb->v[1];
    va->v[2] /= vb->v[2];
}

void vdivs(vec4* quotient, const vec4* v, float s) {
    quotient->v[0] = v->v[0] / s;
    quotient->v[1] = v->v[1] / s;
    quotient->v[2] = v->v[2] / s;
}

void vidivs(vec4* v, float s) {
    v->v[0] /= s;
    v->v[1] /= s;
    v->v[2] /= s;
}

float vdot(const vec4* va, const vec4* vb) {
    return va->v[0] * vb->v[0] + va->v[1] * vb->v[1] + va->v[2] * vb->v[2];
}

void vcross(vec4* cross, const vec4* va, const vec4* vb) {
    cross->v[0] = va->v[1] * vb->v[2] - va->v[2] * vb->v[1];
    cross->v[1] = va->v[2] * vb->v[0] - va->v[0] * vb->v[2];
    cross->v[2] = va->v[0] * vb->v[1] - va->v[1] * vb->v[0];
}

float vnorm(const vec4* v) {
    return v->v[0] * v->v[0] + v->v[1] * v->v[1] + v->v[2] * v->v[2];
}

float vlen(const vec4* v) {
    return sqrtf(vnorm(v));
}

void vnormalize(vec4* normalized, const vec4* v) {
    const float len = vlen(v);
    normalized->v[0] = v->v[0] / len;
    normalized->v[1] = v->v[1] / len;
    normalized->v[2] = v->v[2] / len;
}

void vinormalize(vec4* v) {
    const float len = vlen(v);
    v->v[0] /= len;
    v->v[1] /= len;
    v->v[2] /= len;
}

///////////////////////////////////////////////////////////////////////////////
// Matrix math
///////////////////////////////////////////////////////////////////////////////

void mmulm(mat4* result, const mat4* ma, const mat4* mb) {
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            result->m[j * 4 + i] = 0.0f;
            for (int k = 0; k < 4; ++k) {
                result->m[j * 4 + i] += ma->m[k * 4 + i] * mb->m[j * 4 + k];
            }
        }
    }
}

void mimulm(mat4 *ma, const mat4 *mb) {
    mat4 result;
    mmulm(&result, ma, mb);
    *ma = result;
}

void mirotatex(mat4* m, float angle) {
    const float sine = sinf(angle);
    const float cosine = cosf(angle);
    mat4 rotation = IDENTITY_MATRIX4;

    rotation.m[5] = cosine;
    rotation.m[6] = -sine;
    rotation.m[9] = sine;
    rotation.m[10] = cosine;

    mimulm(m, &rotation);
}

void mirotatey(mat4* m, float angle) {
    const float sine = sinf(angle);
    const float cosine = cosf(angle);
    mat4 rotation = IDENTITY_MATRIX4;

    rotation.m[0] = cosine;
    rotation.m[8] = sine;
    rotation.m[2] = -sine;
    rotation.m[10] = cosine;

    mimulm(m, &rotation);
}

void mirotatez(mat4* m, float angle) {
    const float sine = sinf(angle);
    const float cosine = cosf(angle);
    mat4 rotation = IDENTITY_MATRIX4;

    rotation.m[0] = cosine;
    rotation.m[1] = -sine;
    rotation.m[4] = sine;
    rotation.m[5] = cosine;

    mimulm(m, &rotation);
}

void miscale(mat4* m, float x, float y, float z) {
    mat4 scale = IDENTITY_MATRIX4;

    scale.m[0] = x;
    scale.m[5] = y;
    scale.m[10] = z;

    mimulm(m, &scale);
}

void mitranslate(mat4* m, float tx, float ty, float tz) {
    mat4 translation = IDENTITY_MATRIX4;

    translation.m[12] += tx;
    translation.m[13] += ty;
    translation.m[14] += tz;

    mimulm(m, &translation);
}

///////////////////////////////////////////////////////////////////////////////
// Projection and view matrix generation
///////////////////////////////////////////////////////////////////////////////

static const float PI = 3.14159265358979323846f;

void perspective(mat4* m, float fov, float ratio, float near, float far) {
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
}

void orthogonal(mat4* m, float left, float right, float bottom, float top, float near, float far) {
    const float tx = -(right + left) / (right - left);
    const float ty = -(top + bottom) / (top - bottom);
    const float tz = -(far + near) / (far - near);

    *m = IDENTITY_MATRIX4;

    m->m[0] = 2.0f / (right - left);
    m->m[1 * 4 + 1] = 2.0f / (top - bottom);
    m->m[2 * 4 + 2] = -2.0f / (far - near);
    m->m[3 * 4 + 0] = tx;
    m->m[3 * 4 + 1] = ty;
    m->m[3 * 4 + 2] = tz;
}

void lookat(mat4* m, const vec4* position, const vec4* target) {
    vec4 up = {
        {0.0f, 0.0f, 1.0f, 1.0f}
    };
    vec4 direction, right;

    // Compute direction vectors and set up base matrix.
    direction.v[0] = (target->v[0] - position->v[0]);
    direction.v[1] = (target->v[1] - position->v[1]);
    direction.v[2] = (target->v[2] - position->v[2]);
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
    mitranslate(m, -position->v[0], -position->v[1], -position->v[2]);
}