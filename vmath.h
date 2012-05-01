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
    // Vector math (3d)
    ///////////////////////////////////////////////////////////////////////////////

    void v3copy(vec3* into, const vec3* v);

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

    ///////////////////////////////////////////////////////////////////////////////
    // Vector math (4d)
    ///////////////////////////////////////////////////////////////////////////////

    void v4copy(vec4* into, const vec4* v);

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

    ///////////////////////////////////////////////////////////////////////////////
    // Matrix math
    ///////////////////////////////////////////////////////////////////////////////

    void mmulm(mat4* result, const mat4* ma, const mat4* mb);

    void mimulm(mat4 *ma, const mat4 *mb);

    void mmulv(vec4* result, const vec4* v, const mat4* m);

    void mimulv(vec4* v, const mat4* m);

    void mtranspose(mat4* to, const mat4* from);

    void mitranspose(mat4* m);

    int minvert(mat4* inverse, const mat4* m);

    int miinvert(mat4* m);

    void mirotatex(mat4* m, float angle);

    void mirotatey(mat4* m, float angle);

    void mirotatez(mat4* m, float angle);

    void miscale(mat4* m, float x, float y, float z);

    void mitranslate(mat4* m, float tx, float ty, float tz);

#ifdef	__cplusplus
}
#endif

#endif	/* VMATH_H */

