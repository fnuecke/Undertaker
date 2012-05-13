#include "graphics.h"

#include <math.h>
#include <GL/glew.h>

#include "callbacks.h"
#include "config.h"
#include "events.h"
#include "vmath.h"

///////////////////////////////////////////////////////////////////////////////
// Globals
///////////////////////////////////////////////////////////////////////////////

/** It's pie. Yummy! */
static const float PI = 3.14159265358979323846f;

/** Number of pushes we support for each matrix type */
#define MATRIX_STACK_SIZE 8

/** Matrices we use for resolving vertex positions to screen positions */
static struct {
    /** The current projection transform */
    mat4 projection[MATRIX_STACK_SIZE];

    /** The current view transform */
    mat4 view[MATRIX_STACK_SIZE];

    /** The current model transform */
    mat4 model[MATRIX_STACK_SIZE];

    /** Precomputed model-view transform */
    mat4 mv;

    /** The current model-view-projection transform */
    mat4 mvp;

    /** Precomputed surface normal transform (transpose of inverse of current model transform) */
    mat3 normal;
} matrix;

/** Stack depths for the three matrix stacks */
static struct {
    /** Projection transform stack depth */
    unsigned int projection;

    /** View transform stack depth */
    unsigned int view;

    /** Model transform stack depth */
    unsigned int model;
} stack;

/** Methods to call when the model matrix changes */
static Callbacks* gModelMatrixChangedCallbacks = 0;

///////////////////////////////////////////////////////////////////////////////
// Utility methods
///////////////////////////////////////////////////////////////////////////////

static void updateMatrices(int modelViewChanged) {
    if (modelViewChanged) {
        // Pre-compute model-view transform, as either model or view changed.
        mmulm(&matrix.mv, MP_GetViewMatrix(), MP_GetModelMatrix());

        // And update OpenGLs model-view matrix.
        glMatrixMode(GL_MODELVIEW);
        glLoadMatrixf(matrix.mv.m);
    } else {
        // Model and view didn't change, so it was the projection matrix.
        glMatrixMode(GL_PROJECTION);
        glLoadMatrixf(MP_GetProjectionMatrix()->m);
    }

    // Pre-compute model-view-projection transform.
    mmulm(&matrix.mvp, MP_GetProjectionMatrix(), &matrix.mv);

    if (modelViewChanged) {
        CB_Call(gModelMatrixChangedCallbacks);
    }
}

static int pushProjection(void) {
    if (stack.projection > 0) {
        --stack.projection;
        matrix.projection[stack.projection] = IDENTITY_MATRIX4;
        updateMatrices(0);
        return 1;
    }
    return 0;
}

static int popProjection(void) {
    if (stack.projection < MATRIX_STACK_SIZE - 1) {
        ++stack.projection;
        updateMatrices(0);
        return 1;
    }
    return 0;
}

static int pushView(void) {
    if (stack.view > 0) {
        --stack.view;
        matrix.view[stack.view] = IDENTITY_MATRIX4;
        updateMatrices(1);
        return 1;
    }
    return 0;
}

static int popView(void) {
    if (stack.view < MATRIX_STACK_SIZE - 1) {
        ++stack.view;
        updateMatrices(1);
        return 1;
    }
    return 0;
}

static int pushModel(void) {
    if (stack.model > 0) {
        --stack.model;
        matrix.model[stack.model] = matrix.model[stack.model + 1];
        updateMatrices(1);
        return 1;
    }
    return 0;
}

static int popModel(void) {
    if (stack.model < MATRIX_STACK_SIZE - 1) {
        ++stack.model;
        updateMatrices(1);
        return 1;
    }
    return 0;
}

///////////////////////////////////////////////////////////////////////////////
// Accessors
///////////////////////////////////////////////////////////////////////////////

const mat4* MP_GetModelMatrix(void) {
    return &matrix.model[stack.model];
}

const mat4* MP_GetViewMatrix(void) {
    return &matrix.view[stack.view];
}

const mat4* MP_GetProjectionMatrix(void) {
    return &matrix.projection[stack.projection];
}

const mat4* MP_GetModelViewMatrix(void) {
    return &matrix.mv;
}

const mat4* MP_GetModelViewProjectionMatrix(void) {
    return &matrix.mvp;
}

int MP_PushModelMatrix(void) {
    return pushModel();
}

int MP_PopModelMatrix(void) {
    return popModel();
}

void MP_SetModelMatrix(const mat4* m) {
    matrix.model[stack.model] = *m;
    updateMatrices(1);
}

void MP_ScaleModelMatrix(float sx, float sy, float sz) {
    miscale(&matrix.model[stack.model], sx, sy, sz);
    updateMatrices(1);
}

void MP_TranslateModelMatrix(float tx, float ty, float tz) {
    mitranslate(&matrix.model[stack.model], tx, ty, tz);
    updateMatrices(1);
}

///////////////////////////////////////////////////////////////////////////////
// Projection and view matrix generation
///////////////////////////////////////////////////////////////////////////////

int MP_BeginPerspective(void) {
    const float f = 1.0f / tanf(MP_field_of_view * (PI / 360.0f));
    float* m;

    if (!pushProjection()) {
        return 0;
    }

    m = matrix.projection[stack.projection].m;
#define M(row, col) m[col * 4 + row]
    M(0, 0) = f / MP_ASPECT_RATIO;
    M(1, 1) = f;
    M(2, 2) = (MP_CLIP_FAR + MP_CLIP_NEAR) / (MP_CLIP_NEAR - MP_CLIP_FAR);
    M(2, 3) = (2.0f * MP_CLIP_FAR * MP_CLIP_NEAR) / (MP_CLIP_NEAR - MP_CLIP_FAR);
    M(3, 2) = -1.0f;
    M(3, 3) = 0.0f;
#undef M

    updateMatrices(0);

    return 1;
}

int MP_EndPerspective(void) {
    return popProjection();
}

int MP_BeginOrthogonal(void) {
    float* m;

    if (!pushProjection()) {
        return 0;
    }

    m = matrix.projection[stack.projection].m;
#define M(row, col) m[col * 4 + row]
    M(0, 0) = 2.0f / MP_resolution_x;
    M(0, 1) = 0.0f;
    M(0, 2) = 0.0f;
    M(0, 3) = -1.0f;

    M(1, 0) = 0.0f;
    M(1, 1) = 2.0f / MP_resolution_y;
    M(1, 2) = 0.0f;
    M(1, 3) = -1.0f;

    M(2, 0) = 0.0f;
    M(2, 1) = 0.0f;
    M(2, 2) = -2.0f / (MP_CLIP_FAR - MP_CLIP_NEAR);
    M(2, 3) = -(MP_CLIP_FAR + MP_CLIP_NEAR) / (MP_CLIP_FAR - MP_CLIP_NEAR);

    M(3, 0) = 0.0f;
    M(3, 1) = 0.0f;
    M(3, 2) = 0.0f;
    M(3, 3) = 1.0f;
#undef M

    updateMatrices(0);

    return 1;
}

int MP_EndOrthogonal(void) {
    return popProjection();
}

int MP_BeginPerspectiveForPicking(float x, float y) {
    if (MP_BeginPerspective()) {
        mat4 look = IDENTITY_MATRIX4;
        mitranslate(&look, MP_resolution_x - 2 * x, MP_resolution_y - 2 * y, 0);
        miscale(&look, MP_resolution_x, MP_resolution_y, 1.0);

        mimulm(&look, MP_GetProjectionMatrix());
        matrix.projection[stack.projection] = look;

        updateMatrices(0);

        return 1;
    }
    return 0;
}

int MP_BeginLookAt(float eyex, float eyey, float eyez, float lookatx, float lookaty, float lookatz) {
    mat4* m;
    vec4 up = {
        {0.0f, 0.0f, 1.0f, 1.0f}
    };
    vec4 direction, right;

    if (!pushView()) {
        return 0;
    }

    m = &matrix.view[stack.view];

    // Compute direction vectors and set up base matrix.
    direction.d.x = (lookatx - eyex);
    direction.d.y = (lookaty - eyey);
    direction.d.z = (lookatz - eyez);
    v4inormalize(&direction);

    v4cross(&right, &direction, &up);
    v4inormalize(&right);

    v4cross(&up, &right, &direction);
    v4inormalize(&up);

    // First row.
    m->m[0] = right.d.x;
    m->m[4] = right.d.y;
    m->m[8] = right.d.z;
    m->m[12] = 0.0f;

    // Second row.
    m->m[1] = up.d.x;
    m->m[5] = up.d.y;
    m->m[9] = up.d.z;
    m->m[13] = 0.0f;

    // Third row.
    m->m[2] = -direction.d.x;
    m->m[6] = -direction.d.y;
    m->m[10] = -direction.d.z;
    m->m[14] = 0.0f;

    // Fourth row.
    m->m[3] = 0.0f;
    m->m[7] = 0.0f;
    m->m[11] = 0.0f;
    m->m[15] = 1.0f;

    // Then add translation based on eye position.
    mitranslate(m, -eyex, -eyey, -eyez);

    updateMatrices(1);

    return 1;
}

int MP_EndLookAt(void) {
    return popView();
}

///////////////////////////////////////////////////////////////////////////////
// Projection
///////////////////////////////////////////////////////////////////////////////

int MP_Project(float objx, float objy, float objz,
               float *winx, float *winy, float *winz) {
    vec4 in;
    vec4 out;

    in.d.x = objx;
    in.d.y = objy;
    in.d.z = objz;
    in.d.w = 1.0f;

    mmulv(&out, &in, MP_GetModelViewMatrix());
    mmulv(&in, &out, MP_GetProjectionMatrix());

    if (in.d.w * in.d.w < 1e-25) {
        return 0;
    }

    in.d.x /= in.d.w;
    in.d.y /= in.d.w;
    in.d.z /= in.d.w;

    /* Map x, y and z to range 0-1 */
    in.d.x = in.d.x * 0.5f + 0.5f;
    in.d.y = in.d.y * 0.5f + 0.5f;
    in.d.z = in.d.z * 0.5f + 0.5f;

    /* Map x,y to viewport */
    in.d.x = in.d.x * MP_resolution_x;
    in.d.y = in.d.y * MP_resolution_y;

    *winx = in.d.x;
    *winy = in.d.y;
    *winz = in.d.z;

    return 1;
}

int MP_UnProject(float winx, float winy, float winz,
                 float *objx, float *objy, float *objz) {
    mat4 mvp = matrix.mvp;
    vec4 in;
    vec4 out;

    if (!miinvert(&mvp)) {
        return 0;
    }

    in.d.x = winx;
    in.d.y = winy;
    in.d.z = winz;
    in.d.w = 1.0f;

    // Map x and y from window coordinates
    in.d.x = in.d.x / MP_resolution_x;
    in.d.y = in.d.y / MP_resolution_y;

    // Map to range -1 to 1
    in.d.x = in.d.x * 2.0f - 1.0f;
    in.d.y = in.d.y * 2.0f - 1.0f;
    in.d.z = in.d.z * 2.0f - 1.0f;

    //__gluMultMatrixVecd(finalMatrix, in, out);
    mmulv(&out, &in, &mvp);
    if (out.d.w * out.d.w < 1e-25) {
        return 0;
    }

    *objx = out.d.x / out.d.w;
    *objy = out.d.y / out.d.w;
    *objz = out.d.z / out.d.w;

    return 1;
}

///////////////////////////////////////////////////////////////////////////////
// Init / Events
///////////////////////////////////////////////////////////////////////////////

void MP_InitGraphics(void) {
    for (unsigned int i = 0; i < MATRIX_STACK_SIZE; ++i) {
        matrix.model[i] = IDENTITY_MATRIX4;
        matrix.view[i] = IDENTITY_MATRIX4;
        matrix.projection[i] = IDENTITY_MATRIX4;
    }

    matrix.mv = IDENTITY_MATRIX4;
    matrix.mvp = IDENTITY_MATRIX4;
    matrix.normal = IDENTITY_MATRIX3;

    stack.model = MATRIX_STACK_SIZE - 1;
    stack.projection = MATRIX_STACK_SIZE - 1;
    stack.view = MATRIX_STACK_SIZE - 1;
}

void MP_OnModelMatrixChanged(callback method) {
    if (!gModelMatrixChangedCallbacks) {
        gModelMatrixChangedCallbacks = CB_New();
    }
    CB_Add(gModelMatrixChangedCallbacks, method);
}
