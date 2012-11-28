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

#include "vmath.h"

#include <math.h>

///////////////////////////////////////////////////////////////////////////////
// Vector math (2d)
///////////////////////////////////////////////////////////////////////////////

void v2set(vec2* v, float x, float y) {
    v->d.x = x;
    v->d.y = y;
}

void v2copy(vec2* into, const vec2* v) {
    into->d.x = v->d.x;
    into->d.y = v->d.y;
}

void v2neg(vec2* into, const vec2* v) {
    into->d.x = -v->d.x;
    into->d.y = -v->d.y;
}

void v2ineg(vec2* v) {
    v->d.x = -v->d.x;
    v->d.y = -v->d.y;
}

void v2add(vec2* sum, const vec2* va, const vec2* vb) {
    sum->d.x = va->d.x + vb->d.x;
    sum->d.y = va->d.y + vb->d.y;
}

void v2iadd(vec2* va, const vec2* vb) {
    va->d.x += vb->d.x;
    va->d.y += vb->d.y;
}

void v2sub(vec2* difference, const vec2* va, const vec2* vb) {
    difference->d.x = va->d.x - vb->d.x;
    difference->d.y = va->d.y - vb->d.y;
}

void v2isub(vec2* va, const vec2* vb) {
    va->d.x -= vb->d.x;
    va->d.y -= vb->d.y;
}

void v2mul(vec2* product, const vec2* va, const vec2* vb) {
    product->d.x = va->d.x * vb->d.x;
    product->d.y = va->d.y * vb->d.y;
}

void v2imul(vec2* va, const vec2* vb) {
    va->d.x *= vb->d.x;
    va->d.y *= vb->d.y;
}

void v2muls(vec2* product, const vec2* v, float s) {
    product->d.x = v->d.x * s;
    product->d.y = v->d.y * s;
}

void v2imuls(vec2* v, float s) {
    v->d.x *= s;
    v->d.y *= s;
}

void v2div(vec2* quotient, const vec2* va, const vec2* vb) {
    quotient->d.x = va->d.x / vb->d.x;
    quotient->d.y = va->d.y / vb->d.y;
}

void v2idiv(vec2* va, const vec2* vb) {
    va->d.x /= vb->d.x;
    va->d.y /= vb->d.y;
}

void v2divs(vec2* quotient, const vec2* v, float s) {
    quotient->d.x = v->d.x / s;
    quotient->d.y = v->d.y / s;
}

void v2idivs(vec2* v, float s) {
    v->d.x /= s;
    v->d.y /= s;
}

float v2dot(const vec2* va, const vec2* vb) {
    return va->d.x * vb->d.x + va->d.y * vb->d.y;
}

void v2cross(vec2* cross, const vec2* va, const vec2* vb) {
    cross->d.x = va->d.y * vb->d.x - va->d.x * vb->d.y;
    cross->d.y = va->d.x * vb->d.y - va->d.y * vb->d.x;
}

float v2norm(const vec2* v) {
    return v->d.x * v->d.x + v->d.y * v->d.y;
}

float v2len(const vec2* v) {
    return sqrtf(v2norm(v));
}

float v2distance(const vec2* va, const vec2* vb) {
    vec2 tmp;
    v2sub(&tmp, va, vb);
    return v2len(&tmp);
}

void v2normalize(vec2* normalized, const vec2* v) {
    const float len = v2len(v);
    normalized->d.x = v->d.x / len;
    normalized->d.y = v->d.y / len;
}

void v2inormalize(vec2* v) {
    const float len = v2len(v);
    v->d.x /= len;
    v->d.y /= len;
}

void v2lerp(vec2* v, const vec2* v1, const vec2* v2, float t) {
    const float ti = 1 - t;
    v->d.x = v1->d.x * ti + v2->d.x * t;
    v->d.y = v1->d.y * ti + v2->d.y * t;
}

///////////////////////////////////////////////////////////////////////////////
// Vector math (3d)
///////////////////////////////////////////////////////////////////////////////

void v3set(vec3* v, float x, float y, float z) {
    v->d.x = x;
    v->d.y = y;
    v->d.z = z;
}

void v3copy(vec3* into, const vec3* v) {
    into->d.x = v->d.x;
    into->d.y = v->d.y;
    into->d.z = v->d.z;
}

void v3neg(vec3* into, const vec3* v) {
    into->d.x = -v->d.x;
    into->d.y = -v->d.y;
    into->d.z = -v->d.z;
}

void v3ineg(vec3* v) {
    v->d.x = -v->d.x;
    v->d.y = -v->d.y;
    v->d.z = -v->d.z;
}

void v3add(vec3* sum, const vec3* va, const vec3* vb) {
    sum->d.x = va->d.x + vb->d.x;
    sum->d.y = va->d.y + vb->d.y;
    sum->d.z = va->d.z + vb->d.z;
}

void v3iadd(vec3* va, const vec3* vb) {
    va->d.x += vb->d.x;
    va->d.y += vb->d.y;
    va->d.z += vb->d.z;
}

void v3sub(vec3* difference, const vec3* va, const vec3* vb) {
    difference->d.x = va->d.x - vb->d.x;
    difference->d.y = va->d.y - vb->d.y;
    difference->d.z = va->d.z - vb->d.z;
}

void v3isub(vec3* va, const vec3* vb) {
    va->d.x -= vb->d.x;
    va->d.y -= vb->d.y;
    va->d.z -= vb->d.z;
}

void v3mul(vec3* product, const vec3* va, const vec3* vb) {
    product->d.x = va->d.x * vb->d.x;
    product->d.y = va->d.y * vb->d.y;
    product->d.z = va->d.z * vb->d.z;
}

void v3imul(vec3* va, const vec3* vb) {
    va->d.x *= vb->d.x;
    va->d.y *= vb->d.y;
    va->d.z *= vb->d.z;
}

void v3muls(vec3* product, const vec3* v, float s) {
    product->d.x = v->d.x * s;
    product->d.y = v->d.y * s;
    product->d.z = v->d.z * s;
}

void v3imuls(vec3* v, float s) {
    v->d.x *= s;
    v->d.y *= s;
    v->d.z *= s;
}

void v3div(vec3* quotient, const vec3* va, const vec3* vb) {
    quotient->d.x = va->d.x / vb->d.x;
    quotient->d.y = va->d.y / vb->d.y;
    quotient->d.z = va->d.z / vb->d.z;
}

void v3idiv(vec3* va, const vec3* vb) {
    va->d.x /= vb->d.x;
    va->d.y /= vb->d.y;
    va->d.z /= vb->d.z;
}

void v3divs(vec3* quotient, const vec3* v, float s) {
    quotient->d.x = v->d.x / s;
    quotient->d.y = v->d.y / s;
    quotient->d.z = v->d.z / s;
}

void v3idivs(vec3* v, float s) {
    v->d.x /= s;
    v->d.y /= s;
    v->d.z /= s;
}

float v3dot(const vec3* va, const vec3* vb) {
    return va->d.x * vb->d.x + va->d.y * vb->d.y + va->d.z * vb->d.z;
}

void v3cross(vec3* cross, const vec3* va, const vec3* vb) {
    cross->d.x = va->d.y * vb->d.z - va->d.z * vb->d.y;
    cross->d.y = va->d.z * vb->d.x - va->d.x * vb->d.z;
    cross->d.z = va->d.x * vb->d.y - va->d.y * vb->d.x;
}

float v3norm(const vec3* v) {
    return v->d.x * v->d.x + v->d.y * v->d.y + v->d.z * v->d.z;
}

float v3len(const vec3* v) {
    return sqrtf(v3norm(v));
}

float v3distance(const vec3* va, const vec3* vb) {
    vec3 tmp;
    v3sub(&tmp, va, vb);
    return v3len(&tmp);
}

void v3normalize(vec3* normalized, const vec3* v) {
    const float len = v3len(v);
    normalized->d.x = v->d.x / len;
    normalized->d.y = v->d.y / len;
    normalized->d.z = v->d.z / len;
}

void v3inormalize(vec3* v) {
    const float len = v3len(v);
    v->d.x /= len;
    v->d.y /= len;
    v->d.z /= len;
}

void v3lerp(vec3* v, const vec3* v1, const vec3* v2, float t) {
    const float ti = 1 - t;
    v->d.x = v1->d.x * ti + v2->d.x * t;
    v->d.y = v1->d.y * ti + v2->d.y * t;
    v->d.z = v1->d.z * ti + v2->d.z * t;
}

///////////////////////////////////////////////////////////////////////////////
// Vector math (4d)
///////////////////////////////////////////////////////////////////////////////

void v4set(vec4* v, float x, float y, float z, float w) {
    v->d.x = x;
    v->d.y = y;
    v->d.z = z;
    v->d.w = w;
}

void v4copy(vec4* into, const vec4* v) {
    into->d.x = v->d.x;
    into->d.y = v->d.y;
    into->d.z = v->d.z;
    into->d.w = v->d.w;
}

void v4neg(vec4* into, const vec4* v) {
    into->d.x = -v->d.x;
    into->d.y = -v->d.y;
    into->d.z = -v->d.z;
    into->d.w = v->d.w;
}

void v4ineg(vec4* v) {
    v->d.x = -v->d.x;
    v->d.y = -v->d.y;
    v->d.z = -v->d.z;
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

float v4distance(const vec4* va, const vec4* vb) {
    vec4 tmp;
    v4sub(&tmp, va, vb);
    return v4len(&tmp);
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

void v4lerp(vec4* v, const vec4* v1, const vec4* v2, float t) {
    const float ti = 1 - t;
    v->d.x = v1->d.x * ti + v2->d.x * t;
    v->d.y = v1->d.y * ti + v2->d.y * t;
    v->d.z = v1->d.z * ti + v2->d.z * t;
}

///////////////////////////////////////////////////////////////////////////
// Quaternion math
///////////////////////////////////////////////////////////////////////////

void qfromrot(quat* result, const mat4* m) {
#define M(row, col) m->m[col * 4 + row]
    float det = M(1, 1) + M(2, 2) + M(3, 3);
    if (det > 0.0f) {
        float a = sqrtf(det + 1.0f);
        result->d.w = a * 0.5f;
        a = 0.5f / a;
        result->d.x = (M(2, 3) - M(3, 2)) * a;
        result->d.y = (M(3, 1) - M(1, 3)) * a;
        result->d.z = (M(1, 2) - M(2, 1)) * a;
    } else if (M(1, 1) >= M(2, 2) && M(1, 1) >= M(3, 3)) {
        float a = sqrtf(1.0f + M(1, 1) - M(2, 2) - M(3, 3));
        float b = 0.5f / a;
        result->d.x = 0.5f * a;
        result->d.y = (M(1, 2) + M(2, 1)) * b;
        result->d.z = (M(1, 3) + M(3, 1)) * b;
        result->d.w = (M(2, 3) - M(3, 2)) * b;
    } else if (M(2, 2) > M(3, 3)) {
        float a = sqrtf(1.0f + M(2, 2) - M(1, 1) - M(3, 3));
        float b = 0.5f / a;
        result->d.x = (M(2, 1) + M(1, 2)) * b;
        result->d.y = 0.5f * a;
        result->d.z = (M(3, 2) + M(2, 3)) * b;
        result->d.w = (M(3, 1) - M(1, 3)) * b;
    } else {
        float a = sqrtf(1.0f + M(3, 3) - M(1, 1) - M(2, 2));
        float b = 0.5f / a;
        result->d.x = (M(3, 1) + M(1, 3)) * b;
        result->d.y = (M(3, 2) + M(2, 3)) * b;
        result->d.z = 0.5f * a;
        result->d.w = (M(1, 2) - M(2, 1)) * b;
    }
#undef M
}

void qslerp(quat* result, const quat* q1, const quat* q2, float t) {
    float ratioA, ratioB, halfTheta, sinHalfTheta;
    float cosHalfTheta = q1->d.x * q2->d.x + q1->d.y * q2->d.y + q1->d.z * q2->d.z + q1->d.w * q2->d.w;
    if (fabsf(cosHalfTheta) >= 1.0) {
        result->d.x = q1->d.x;
        result->d.y = q1->d.y;
        result->d.z = q1->d.z;
        result->d.w = q1->d.w;
        return;
    }
    halfTheta = acosf(cosHalfTheta);
    sinHalfTheta = sqrtf(1.0f - cosHalfTheta * cosHalfTheta);
    if (fabsf(sinHalfTheta) < 0.001) {
        result->d.w = (q1->d.w * 0.5f + q2->d.w * 0.5f);
        result->d.x = (q1->d.x * 0.5f + q2->d.x * 0.5f);
        result->d.y = (q1->d.y * 0.5f + q2->d.y * 0.5f);
        result->d.z = (q1->d.z * 0.5f + q2->d.z * 0.5f);
        return;
    }
    ratioA = sin((1.0f - t) * halfTheta) / sinHalfTheta;
    ratioB = sin(t * halfTheta) / sinHalfTheta;
    result->d.x = ratioA * q1->d.x + ratioB * q2->d.x;
    result->d.y = ratioA * q1->d.y + ratioB * q2->d.y;
    result->d.z = ratioA * q1->d.z + ratioB * q2->d.z;
    result->d.w = ratioA * q1->d.w + ratioB * q2->d.w;
}

///////////////////////////////////////////////////////////////////////////////
// Matrix math
///////////////////////////////////////////////////////////////////////////////

void m4fromq(mat4* result, const quat* q) {
#define M(row, col) result->m[col * 4 + row]
    float a = q->d.x * q->d.x;
    float b = q->d.y * q->d.y;
    float c = q->d.z * q->d.z;
    float d = q->d.x * q->d.y;
    float e = q->d.z * q->d.w;
    float f = q->d.z * q->d.x;
    float g = q->d.y * q->d.w;
    float h = q->d.y * q->d.z;
    float i = q->d.x * q->d.w;
    M(1, 1) = 1.0f - (2.0f * (b + c));
    M(1, 2) = 2.0f * (d + e);
    M(1, 3) = 2.0f * (f - g);
    M(1, 4) = 0.0f;
    M(2, 1) = 2.0f * (d - e);
    M(2, 2) = 1.0f - (2.0f * (c + a));
    M(2, 3) = 2.0f * (h + i);
    M(2, 4) = 0.0f;
    M(3, 1) = 2.0f * (f + g);
    M(3, 2) = 2.0f * (h - i);
    M(3, 3) = 1.0f - (2.0f * (b + a));
    M(3, 4) = 0.0f;
    M(4, 1) = 0.0f;
    M(4, 2) = 0.0f;
    M(4, 3) = 0.0f;
    M(4, 4) = 1.0f;
#undef M
}

void m4mulm(mat4* result, const mat4* ma, const mat4* mb) {
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

void m4imulm(mat4 *ma, const mat4 *mb) {
    mat4 result;
    m4mulm(&result, ma, mb);
    *ma = result;
}

void m4mulv(vec4* result, const vec4* v, const mat4* m) {
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

void m4imulv(vec4* v, const mat4* m) {
    vec4 t;
    m4mulv(&t, v, m);
    *v = t;
}

void m4transpose(mat4* to, const mat4* from) {
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

void m4itranspose(mat4* m) {
    mat4 t;
    m4transpose(&t, m);
    *m = t;
}

int m4invert(mat4* inverse, const mat4* m) {
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

int m4iinvert(mat4* m) {
    return m4invert(m, m);
}

void m4irotatex(mat4* m, float angle) {
    const float sine = sinf(angle);
    const float cosine = cosf(angle);
    mat4 rotation = IDENTITY_MATRIX4;

    rotation.m[5] = cosine;
    rotation.m[6] = -sine;
    rotation.m[9] = sine;
    rotation.m[10] = cosine;

    m4imulm(m, &rotation);
}

void m4irotatey(mat4* m, float angle) {
    const float sine = sinf(angle);
    const float cosine = cosf(angle);
    mat4 rotation = IDENTITY_MATRIX4;

    rotation.m[0] = cosine;
    rotation.m[8] = sine;
    rotation.m[2] = -sine;
    rotation.m[10] = cosine;

    m4imulm(m, &rotation);
}

void m4irotatez(mat4* m, float angle) {
    const float sine = sinf(angle);
    const float cosine = cosf(angle);
    mat4 rotation = IDENTITY_MATRIX4;

    rotation.m[0] = cosine;
    rotation.m[1] = -sine;
    rotation.m[4] = sine;
    rotation.m[5] = cosine;

    m4imulm(m, &rotation);
}

void m4iscale(mat4* m, float x, float y, float z) {
    mat4 scale = IDENTITY_MATRIX4;

    scale.m[0] = x;
    scale.m[5] = y;
    scale.m[10] = z;

    m4imulm(m, &scale);
}

void m4itranslate(mat4* mat, float tx, float ty, float tz) {
    float* m = mat->m;
    m[12] = m[0] * tx + m[4] * ty + m[8] * tz + m[12];
    m[13] = m[1] * tx + m[5] * ty + m[9] * tz + m[13];
    m[14] = m[2] * tx + m[6] * ty + m[10] * tz + m[14];
    m[15] = m[3] * tx + m[7] * ty + m[11] * tz + m[15];
}

#define M(row, col) m->m[col * 4 + row]

void m4forward(vec3* result, const mat4* m) {
    result->d.x = -M(3, 1);
    result->d.y = -M(3, 2);
    result->d.z = -M(3, 3);
}

void m4backward(vec3* result, const mat4* m) {
    result->d.x = M(3, 1);
    result->d.y = M(3, 2);
    result->d.z = M(3, 3);
}

void m4up(vec3* result, const mat4* m) {
    result->d.x = M(2, 1);
    result->d.y = M(2, 2);
    result->d.z = M(2, 3);
}

void m4down(vec3* result, const mat4* m) {
    result->d.x = -M(2, 1);
    result->d.y = -M(2, 2);
    result->d.z = -M(2, 3);
}

void m4left(vec3* result, const mat4* m) {
    result->d.x = -M(1, 1);
    result->d.y = -M(1, 2);
    result->d.z = -M(1, 3);
}

void m4right(vec3* result, const mat4* m) {
    result->d.x = M(1, 1);
    result->d.y = M(1, 2);
    result->d.z = M(1, 3);
}

void m4sforward(mat4* m, const vec3* v) {
    M(3, 1) = -v->d.x;
    M(3, 2) = -v->d.y;
    M(3, 3) = -v->d.z;
}

void m4sbackward(mat4* m, const vec3* v) {
    M(3, 1) = v->d.x;
    M(3, 2) = v->d.y;
    M(3, 3) = v->d.z;
}

void m4sup(mat4* m, const vec3* v) {
    M(2, 1) = v->d.x;
    M(2, 2) = v->d.y;
    M(2, 3) = v->d.z;
}

void m4sdown(mat4* m, const vec3* v) {
    M(2, 1) = -v->d.x;
    M(2, 2) = -v->d.y;
    M(2, 3) = -v->d.z;
}

void m4sleft(mat4* m, const vec3* v) {
    M(1, 1) = -v->d.x;
    M(1, 2) = -v->d.y;
    M(1, 3) = -v->d.z;
}

void m4sright(mat4* m, const vec3* v) {
    M(1, 1) = v->d.x;
    M(1, 2) = v->d.y;
    M(1, 3) = v->d.z;
}

#undef M