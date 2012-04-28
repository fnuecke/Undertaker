/*
 * Parts taken from:
 * 
 * Mesa 3-D graphics library
 * Version:  6.3
 *
 * Copyright (C) 1999-2005  Brian Paul   All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * BRIAN PAUL BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <math.h>

#include "vmath.h"

///////////////////////////////////////////////////////////////////////////////
// Vector math (3d)
///////////////////////////////////////////////////////////////////////////////

void v3copy(vec3* into, const vec3* v) {
    into->d.x = v->d.x;
    into->d.y = v->d.y;
    into->d.z = v->d.z;
}

void v3add(vec3* sum, const vec3* va, const vec3* vb) {
    v4add((vec4*) sum, (const vec4*) va, (const vec4*) vb);
}

void v3iadd(vec3* va, const vec3* vb) {
    v4iadd((vec4*) va, (const vec4*) vb);
}

void v3sub(vec3* difference, const vec3* va, const vec3* vb) {
    v4sub((vec4*) difference, (const vec4*) va, (const vec4*) vb);
}

void v3isub(vec3* va, const vec3* vb) {
    v4isub((vec4*) va, (const vec4*) vb);
}

void v3mul(vec3* product, const vec3* va, const vec3* vb) {
    v4mul((vec4*) product, (const vec4*) va, (const vec4*) vb);
}

void v3imul(vec3* va, const vec3* vb) {
    v4imul((vec4*) va, (const vec4*) vb);
}

void v3muls(vec3* product, const vec3* v, float s) {
    v4muls((vec4*) product, (const vec4*) v, s);
}

void v3imuls(vec3* v, float s) {
    v4imuls((vec4*) v, s);
}

void v3div(vec3* quotient, const vec3* va, const vec3* vb) {
    v4div((vec4*) quotient, (const vec4*) va, (const vec4*) vb);
}

void v3idiv(vec3* va, const vec3* vb) {
    v4idiv((vec4*) va, (const vec4*) vb);
}

void v3divs(vec3* quotient, const vec3* v, float s) {
    v4divs((vec4*) quotient, (const vec4*) v, s);
}

void v3idivs(vec3* v, float s) {
    v4idivs((vec4*) v, s);
}

float v3dot(const vec3* va, const vec3* vb) {
    return v4dot((const vec4*) va, (const vec4*) vb);
}

void v3cross(vec3* cross, const vec3* va, const vec3* vb) {
    v4cross((vec4*) cross, (const vec4*) va, (const vec4*) vb);
}

float v3norm(const vec3* v) {
    return v4norm((const vec4*) v);
}

float v3len(const vec3* v) {
    return v4len((const vec4*) v);
}

void v3normalize(vec3* normalized, const vec3* v) {
    v4normalize((vec4*) normalized, (const vec4*) v);
}

void v3inormalize(vec3* v) {
    v4inormalize((vec4*) v);
}

///////////////////////////////////////////////////////////////////////////////
// Vector math (4d)
///////////////////////////////////////////////////////////////////////////////

void v4copy(vec4* into, const vec4* v) {
    into->d.x = v->d.x;
    into->d.y = v->d.y;
    into->d.z = v->d.z;
    into->d.w = v->d.w;
}

void v4add(vec4* sum, const vec4* va, const vec4* vb) {
    sum->d.x = va->d.x + vb->d.x;
    sum->d.y = va->d.y + vb->d.y;
    sum->d.z = va->d.z + vb->d.z;
}

void v4iadd(vec4* va, const vec4* vb) {
    va->d.x += vb->d.x;
    va->d.y += vb->d.y;
    va->d.z += vb->d.z;
}

void v4sub(vec4* difference, const vec4* va, const vec4* vb) {
    difference->d.x = va->d.x - vb->d.x;
    difference->d.y = va->d.y - vb->d.y;
    difference->d.z = va->d.z - vb->d.z;
}

void v4isub(vec4* va, const vec4* vb) {
    va->d.x -= vb->d.x;
    va->d.y -= vb->d.y;
    va->d.z -= vb->d.z;
}

void v4mul(vec4* product, const vec4* va, const vec4* vb) {
    product->d.x = va->d.x * vb->d.x;
    product->d.y = va->d.y * vb->d.y;
    product->d.z = va->d.z * vb->d.z;
}

void v4imul(vec4* va, const vec4* vb) {
    va->d.x *= vb->d.x;
    va->d.y *= vb->d.y;
    va->d.z *= vb->d.z;
}

void v4muls(vec4* product, const vec4* v, float s) {
    product->d.x = v->d.x * s;
    product->d.y = v->d.y * s;
    product->d.z = v->d.z * s;
}

void v4imuls(vec4* v, float s) {
    v->d.x *= s;
    v->d.y *= s;
    v->d.z *= s;
}

void v4div(vec4* quotient, const vec4* va, const vec4* vb) {
    quotient->d.x = va->d.x / vb->d.x;
    quotient->d.y = va->d.y / vb->d.y;
    quotient->d.z = va->d.z / vb->d.z;
}

void v4idiv(vec4* va, const vec4* vb) {
    va->d.x /= vb->d.x;
    va->d.y /= vb->d.y;
    va->d.z /= vb->d.z;
}

void v4divs(vec4* quotient, const vec4* v, float s) {
    quotient->d.x = v->d.x / s;
    quotient->d.y = v->d.y / s;
    quotient->d.z = v->d.z / s;
}

void v4idivs(vec4* v, float s) {
    v->d.x /= s;
    v->d.y /= s;
    v->d.z /= s;
}

float v4dot(const vec4* va, const vec4* vb) {
    return va->d.x * vb->d.x + va->d.y * vb->d.y + va->d.z * vb->d.z;
}

void v4cross(vec4* cross, const vec4* va, const vec4* vb) {
    cross->d.x = va->d.y * vb->d.z - va->d.z * vb->d.y;
    cross->d.y = va->d.z * vb->d.x - va->d.x * vb->d.z;
    cross->d.z = va->d.x * vb->d.y - va->d.y * vb->d.x;
}

float v4norm(const vec4* v) {
    return v->d.x * v->d.x + v->d.y * v->d.y + v->d.z * v->d.z;
}

float v4len(const vec4* v) {
    return sqrtf(v4norm(v));
}

void v4normalize(vec4* normalized, const vec4* v) {
    const float len = v4len(v);
    normalized->d.x = v->d.x / len;
    normalized->d.y = v->d.y / len;
    normalized->d.z = v->d.z / len;
}

void v4inormalize(vec4* v) {
    const float len = v4len(v);
    v->d.x /= len;
    v->d.y /= len;
    v->d.z /= len;
}

///////////////////////////////////////////////////////////////////////////////
// Matrix math
///////////////////////////////////////////////////////////////////////////////

void mmulm(mat4* result, const mat4* ma, const mat4* mb) {
#define A(row, col) ma->m[(col << 2) + row]
#define B(row, col) mb->m[(col << 2) + row]
#define P(row, col) result->m[(col << 2) + row]
    for (int i = 0; i < 4; i++) {
        const float ai0 = A(i, 0), ai1 = A(i, 1), ai2 = A(i, 2), ai3 = A(i, 3);
        P(i, 0) = ai0 * B(0, 0) + ai1 * B(1, 0) + ai2 * B(2, 0) + ai3 * B(3, 0);
        P(i, 1) = ai0 * B(0, 1) + ai1 * B(1, 1) + ai2 * B(2, 1) + ai3 * B(3, 1);
        P(i, 2) = ai0 * B(0, 2) + ai1 * B(1, 2) + ai2 * B(2, 2) + ai3 * B(3, 2);
        P(i, 3) = ai0 * B(0, 3) + ai1 * B(1, 3) + ai2 * B(2, 3) + ai3 * B(3, 3);
    }
#undef A
#undef B
#undef P
}

void mimulm(mat4 *ma, const mat4 *mb) {
    mat4 result;
    mmulm(&result, ma, mb);
    *ma = result;
}

void mmulv(vec4* result, const vec4* v, const mat4* m) {
#define V(i) v->v[i]
    const float v0 = V(0), v1 = V(1), v2 = V(2), v3 = V(3);
#undef V
#define U(i) result->v[i]
#define M(row, col) m->m[col * 4 + row]
    U(0) = v0 * M(0, 0) + v1 * M(0, 1) + v2 * M(0, 2) + v3 * M(0, 3);
    U(1) = v0 * M(1, 0) + v1 * M(1, 1) + v2 * M(1, 2) + v3 * M(1, 3);
    U(2) = v0 * M(2, 0) + v1 * M(2, 1) + v2 * M(2, 2) + v3 * M(2, 3);
    U(3) = v0 * M(3, 0) + v1 * M(3, 1) + v2 * M(3, 2) + v3 * M(3, 3);
#undef M
}

void mimulv(vec4* v, const mat4* m) {
    vec4 t;
    mmulv(&t, v, m);
    *v = t;
}

void mtranspose(mat4* to, const mat4* from) {
#define T(i) to->m[i]
#define F(i) from->m[i]
    T(0) = F(0);
    T(1) = F(4);
    T(2) = F(8);
    T(3) = F(12);
    T(4) = F(1);
    T(5) = F(5);
    T(6) = F(9);
    T(7) = F(13);
    T(8) = F(2);
    T(9) = F(6);
    T(10) = F(10);
    T(11) = F(14);
    T(12) = F(3);
    T(13) = F(7);
    T(14) = F(11);
    T(15) = F(15);
#undef T
#undef F
}

void mitranspose(mat4* m) {
    mat4 t;
    mtranspose(&t, m);
    *m = t;
}

int minvert(mat4* inverse, const mat4* m) {
    mat4 tmp;
    float det;

#define I(i) tmp.m[i]
#define M(i) m->m[i]
    I(0) = M(5) * M(10) * M(15) -
            M(5) * M(11) * M(14) -
            M(9) * M(6) * M(15) +
            M(9) * M(7) * M(14) +
            M(13) * M(6) * M(11) -
            M(13) * M(7) * M(10);

    I(4) = -M(4) * M(10) * M(15) +
            M(4) * M(11) * M(14) +
            M(8) * M(6) * M(15) -
            M(8) * M(7) * M(14) -
            M(12) * M(6) * M(11) +
            M(12) * M(7) * M(10);

    I(8) = M(4) * M(9) * M(15) -
            M(4) * M(11) * M(13) -
            M(8) * M(5) * M(15) +
            M(8) * M(7) * M(13) +
            M(12) * M(5) * M(11) -
            M(12) * M(7) * M(9);

    I(12) = -M(4) * M(9) * M(14) +
            M(4) * M(10) * M(13) +
            M(8) * M(5) * M(14) -
            M(8) * M(6) * M(13) -
            M(12) * M(5) * M(10) +
            M(12) * M(6) * M(9);

    I(1) = -M(1) * M(10) * M(15) +
            M(1) * M(11) * M(14) +
            M(9) * M(2) * M(15) -
            M(9) * M(3) * M(14) -
            M(13) * M(2) * M(11) +
            M(13) * M(3) * M(10);

    I(5) = M(0) * M(10) * M(15) -
            M(0) * M(11) * M(14) -
            M(8) * M(2) * M(15) +
            M(8) * M(3) * M(14) +
            M(12) * M(2) * M(11) -
            M(12) * M(3) * M(10);

    I(9) = -M(0) * M(9) * M(15) +
            M(0) * M(11) * M(13) +
            M(8) * M(1) * M(15) -
            M(8) * M(3) * M(13) -
            M(12) * M(1) * M(11) +
            M(12) * M(3) * M(9);

    I(13) = M(0) * M(9) * M(14) -
            M(0) * M(10) * M(13) -
            M(8) * M(1) * M(14) +
            M(8) * M(2) * M(13) +
            M(12) * M(1) * M(10) -
            M(12) * M(2) * M(9);

    I(2) = M(1) * M(6) * M(15) -
            M(1) * M(7) * M(14) -
            M(5) * M(2) * M(15) +
            M(5) * M(3) * M(14) +
            M(13) * M(2) * M(7) -
            M(13) * M(3) * M(6);

    I(6) = -M(0) * M(6) * M(15) +
            M(0) * M(7) * M(14) +
            M(4) * M(2) * M(15) -
            M(4) * M(3) * M(14) -
            M(12) * M(2) * M(7) +
            M(12) * M(3) * M(6);

    I(10) = M(0) * M(5) * M(15) -
            M(0) * M(7) * M(13) -
            M(4) * M(1) * M(15) +
            M(4) * M(3) * M(13) +
            M(12) * M(1) * M(7) -
            M(12) * M(3) * M(5);

    I(14) = -M(0) * M(5) * M(14) +
            M(0) * M(6) * M(13) +
            M(4) * M(1) * M(14) -
            M(4) * M(2) * M(13) -
            M(12) * M(1) * M(6) +
            M(12) * M(2) * M(5);

    I(3) = -M(1) * M(6) * M(11) +
            M(1) * M(7) * M(10) +
            M(5) * M(2) * M(11) -
            M(5) * M(3) * M(10) -
            M(9) * M(2) * M(7) +
            M(9) * M(3) * M(6);

    I(7) = M(0) * M(6) * M(11) -
            M(0) * M(7) * M(10) -
            M(4) * M(2) * M(11) +
            M(4) * M(3) * M(10) +
            M(8) * M(2) * M(7) -
            M(8) * M(3) * M(6);

    I(11) = -M(0) * M(5) * M(11) +
            M(0) * M(7) * M(9) +
            M(4) * M(1) * M(11) -
            M(4) * M(3) * M(9) -
            M(8) * M(1) * M(7) +
            M(8) * M(3) * M(5);

    I(15) = M(0) * M(5) * M(10) -
            M(0) * M(6) * M(9) -
            M(4) * M(1) * M(10) +
            M(4) * M(2) * M(9) +
            M(8) * M(1) * M(6) -
            M(8) * M(2) * M(5);

    det = M(0) * I(0) + M(1) * I(4) + M(2) * I(8) + M(3) * I(12);
#undef M

    if (det * det < 1e-25) {
        return 0;
    }

    det = 1.0 / det;

    for (unsigned int i = 0; i < 16; i++) {
        inverse->m[i] = I(i) * det;
    }
#undef I
    return 1;
}

int miinvert(mat4* m) {
    return minvert(m, m);
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

void mitranslate(mat4* mat, float tx, float ty, float tz) {
    /*
        mat4 translation = IDENTITY_MATRIX4;

        translation.m[12] = tx;
        translation.m[13] = ty;
        translation.m[14] = tz;

        mimulm(m, &translation);
     */

    float* m = mat->m;
    m[12] = m[0] * tx + m[4] * ty + m[8] * tz + m[12];
    m[13] = m[1] * tx + m[5] * ty + m[9] * tz + m[13];
    m[14] = m[2] * tx + m[6] * ty + m[10] * tz + m[14];
    m[15] = m[3] * tx + m[7] * ty + m[11] * tz + m[15];
}
