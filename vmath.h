/* 
 * Author: fnuecke
 *
 * Created on April 24, 2012, 10:11 PM
 */

#ifndef VMATH_H
#define	VMATH_H

#include "lua/lua.h"

#ifdef	__cplusplus
extern "C" {
#endif

    ///////////////////////////////////////////////////////////////////////////
    // Types
    ///////////////////////////////////////////////////////////////////////////

    typedef union vec2 {
        float v[2];

        struct {
            float x, y;
        } d;

        struct {
            float u, v;
        } t;
    } vec2;

    typedef union vec3 {
        float v[3];

        struct {
            float x, y, z;
        } d;

        struct {
            float r, g, b;
        } c;
    } vec3;

    typedef union vec4 {
        float v[4];

        struct {
            float x, y, z, w;
        } d;

        struct {
            float r, g, b, a;
        } c;
    } vec4;

    typedef union quat {
        float v[4];

        struct {
            float x, y, z, w;
        } d;
    } quat;

    typedef struct mat3 {
        float m[9];
    } mat3;

    typedef struct mat4 {
        float m[16];
    } mat4;

    static const vec2 ZERO_VEC2 = {
        { 0, 0}
    };

    static const vec3 ZERO_VEC3 = {
        { 0, 0, 0}
    };

    static const vec4 ZERO_VEC4 = {
        { 0, 0, 0, 1}
    };

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

    ///////////////////////////////////////////////////////////////////////////
    // Vector math (2d)
    ///////////////////////////////////////////////////////////////////////////

    void v2set(vec2* v, float x, float y);

    void v2copy(vec2* into, const vec2* v);

    void v2neg(vec2* into, const vec2* v);

    void v2ineg(vec2* v);

    void v2add(vec2* sum, const vec2* va, const vec2* vb);

    void v2iadd(vec2* va, const vec2* vb);

    void v2sub(vec2* difference, const vec2* va, const vec2* vb);

    void v2isub(vec2* va, const vec2* vb);

    void v2mul(vec2* product, const vec2* va, const vec2* vb);

    void v2imul(vec2* va, const vec2* vb);

    void v2muls(vec2* product, const vec2* v, float s);

    void v2imuls(vec2* v, float s);

    void v2div(vec2* quotient, const vec2* va, const vec2* vb);

    void v2idiv(vec2* va, const vec2* vb);

    void v2divs(vec2* quotient, const vec2* v, float s);

    void v2idivs(vec2* v, float s);

    float v2dot(const vec2* va, const vec2* vb);

    void v2cross(vec2* cross, const vec2* va, const vec2* vb);

    float v2norm(const vec2* v);

    float v2len(const vec2* v);

    float v2distance(const vec2* va, const vec2* vb);

    void v2normalize(vec2* normalized, const vec2* v);

    void v2inormalize(vec2* v);

    void v2lerp(vec2* v, const vec2* v1, const vec2* v2, float t);

    ///////////////////////////////////////////////////////////////////////////
    // Vector math (3d)
    ///////////////////////////////////////////////////////////////////////////

    void v3set(vec3* v, float x, float y, float z);

    void v3copy(vec3* into, const vec3* v);

    void v3neg(vec3* into, const vec3* v);

    void v3ineg(vec3* v);

    void v3add(vec3* sum, const vec3* va, const vec3* vb);

    void v3iadd(vec3* va, const vec3* vb);

    void v3sub(vec3* difference, const vec3* va, const vec3* vb);

    void v3isub(vec3* va, const vec3* vb);

    void v3mul(vec3* product, const vec3* va, const vec3* vb);

    void v3imul(vec3* va, const vec3* vb);

    void v3muls(vec3* product, const vec3* v, float s);

    void v3imuls(vec3* v, float s);

    void v3div(vec3* quotient, const vec3* va, const vec3* vb);

    void v3idiv(vec3* va, const vec3* vb);

    void v3divs(vec3* quotient, const vec3* v, float s);

    void v3idivs(vec3* v, float s);

    float v3dot(const vec3* va, const vec3* vb);

    void v3cross(vec3* cross, const vec3* va, const vec3* vb);

    float v3norm(const vec3* v);

    float v3len(const vec3* v);

    float v3distance(const vec3* va, const vec3* vb);

    void v3normalize(vec3* normalized, const vec3* v);

    void v3inormalize(vec3* v);

    void v3lerp(vec3* v, const vec3* v1, const vec3* v2, float t);

    ///////////////////////////////////////////////////////////////////////////
    // Vector math (4d)
    ///////////////////////////////////////////////////////////////////////////

    void v4set(vec4* v, float x, float y, float z, float w);

    void v4copy(vec4* into, const vec4* v);

    void v4neg(vec4* into, const vec4* v);

    void v4ineg(vec4* v);

    void v4add(vec4* sum, const vec4* va, const vec4* vb);

    void v4iadd(vec4* va, const vec4* vb);

    void v4sub(vec4* difference, const vec4* va, const vec4* vb);

    void v4isub(vec4* va, const vec4* vb);

    void v4mul(vec4* product, const vec4* va, const vec4* vb);

    void v4imul(vec4* va, const vec4* vb);

    void v4muls(vec4* product, const vec4* v, float s);

    void v4imuls(vec4* v, float s);

    void v4div(vec4* quotient, const vec4* va, const vec4* vb);

    void v4idiv(vec4* va, const vec4* vb);

    void v4divs(vec4* quotient, const vec4* v, float s);

    void v4idivs(vec4* v, float s);

    float v4dot(const vec4* va, const vec4* vb);

    void v4cross(vec4* cross, const vec4* va, const vec4* vb);

    float v4norm(const vec4* v);

    float v4len(const vec4* v);

    float v4distance(const vec4* va, const vec4* vb);

    void v4normalize(vec4* normalized, const vec4* v);

    void v4inormalize(vec4* v);

    void v4lerp(vec4* v, const vec4* v1, const vec4* v2, float t);

    ///////////////////////////////////////////////////////////////////////////
    // Quaternion math
    ///////////////////////////////////////////////////////////////////////////

    void qfromrot(quat* result, const mat4* m);

    void qslerp(quat* result, const quat* q1, const quat* q2, float t);

    ///////////////////////////////////////////////////////////////////////////
    // Matrix math
    ///////////////////////////////////////////////////////////////////////////

    void m4fromq(mat4* result, const quat* q);

    void m4mulm(mat4* result, const mat4* ma, const mat4* mb);

    void m4imulm(mat4 *ma, const mat4 *mb);

    void m4mulv(vec4* result, const vec4* v, const mat4* m);

    void m4imulv(vec4* v, const mat4* m);

    void m4transpose(mat4* to, const mat4* from);

    void m4itranspose(mat4* m);

    int m4invert(mat4* inverse, const mat4* m);

    int m4iinvert(mat4* m);

    void m4irotatex(mat4* m, float angle);

    void m4irotatey(mat4* m, float angle);

    void m4irotatez(mat4* m, float angle);

    void m4iscale(mat4* m, float x, float y, float z);

    void m4itranslate(mat4* m, float tx, float ty, float tz);

    void m4forward(vec3* result, const mat4* m);

    void m4backward(vec3* result, const mat4* m);

    void m4up(vec3* result, const mat4* m);

    void m4down(vec3* result, const mat4* m);

    void m4left(vec3* result, const mat4* m);

    void m4right(vec3* result, const mat4* m);

    void m4sforward(mat4* m, const vec3* v);

    void m4sbackward(mat4* m, const vec3* v);

    void m4sup(mat4* m, const vec3* v);

    void m4sdown(mat4* m, const vec3* v);

    void m4sleft(mat4* m, const vec3* v);

    void m4sright(mat4* m, const vec3* v);

#ifdef	__cplusplus
}
#endif

#endif
