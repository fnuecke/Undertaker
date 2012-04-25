/* 
 * File:   vmath.h
 * Author: fnuecke
 *
 * Created on April 24, 2012, 10:11 PM
 */

#ifndef VMATH_H
#define	VMATH_H

#ifdef	__cplusplus
extern "C" {
#endif

///////////////////////////////////////////////////////////////////////////////
// Types
///////////////////////////////////////////////////////////////////////////////

typedef struct vec2 {
    float v[2];
} vec2;

typedef struct vec3 {
    float v[3];
} vec3;

typedef struct vec4 {
    float v[4];
} vec4;

typedef struct mat3 {
    float m[9];
} mat3;

typedef struct mat4 {
    float m[16];
} mat4;

static const mat3 IDENTITY_MATRIX3 = {
    {
        1, 0, 0,
        0, 1, 0,
        0, 0, 1
    }
};

static const mat4 IDENTITY_MATRIX4 = {
    {
        1, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, 1, 0,
        0, 0, 0, 1
    }
};

///////////////////////////////////////////////////////////////////////////////
// Vector math
///////////////////////////////////////////////////////////////////////////////

void vcopy(vec4* into, const vec4* v);

void vadd(vec4* sum, const vec4* va, const vec4* vb);

void viadd(vec4* va, const vec4* vb);

void vsub(vec4* difference, const vec4* va, const vec4* vb);

void visub(vec4* va, const vec4* vb);

void vmul(vec4* product, const vec4* va, const vec4* vb);

void vimul(vec4* va, const vec4* vb);

void vmuls(vec4* product, const vec4* v, float s);

void vimuls(vec4* v, float s);

void vdiv(vec4* quotient, const vec4* va, const vec4* vb);

void vidiv(vec4* va, const vec4* vb);

void vdivs(vec4* quotient, const vec4* v, float s);

void vidivs(vec4* v, float s);

float vdot(const vec4* va, const vec4* vb);

void vcross(vec4* cross, const vec4* va, const vec4* vb);

float vnorm(const vec4* v);

float vlen(const vec4* v);

void vnormalize(vec4* normalized, const vec4* v);

void vinormalize(vec4* v);

///////////////////////////////////////////////////////////////////////////////
// Matrix math
///////////////////////////////////////////////////////////////////////////////

void mmulm(mat4* result, const mat4* ma, const mat4* mb);

void mimulm(mat4 *ma, const mat4 *mb);

void mirotatex(mat4* m, float angle);

void mirotatey(mat4* m, float angle);

void mirotatez(mat4* m, float angle);

void miscale(mat4* m, float x, float y, float z);

void mitranslate(mat4* m, float tx, float ty, float tz);

///////////////////////////////////////////////////////////////////////////////
// Projection and view matrix generation
///////////////////////////////////////////////////////////////////////////////

void perspective(mat4* m, float fov, float ratio, float near, float far);

void orthogonal(mat4* m, float left, float right, float bottom, float top, float near, float far);

void lookat(mat4* m, const vec4* position, const vec4* target);

#ifdef	__cplusplus
}
#endif

#endif	/* VMATH_H */

