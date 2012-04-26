#include <math.h>
#include <GL/glew.h>

#include "config.h"
#include "graphics.h"
#include "vmath.h"

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
} stack = {MATRIX_STACK_SIZE - 1, MATRIX_STACK_SIZE - 1, MATRIX_STACK_SIZE - 1};

static void updateMatrices(int modelViewChanged) {
    if (modelViewChanged) {
        // Pre-compute model-view transform, as either model or view changed.
        mmulm(&matrix.mv, DK_GetViewMatrix(), DK_GetModelMatrix());

        // And update OpenGLs model-view matrix.
        glMatrixMode(GL_MODELVIEW);
        glLoadMatrixf(matrix.mv.m);
    } else {
        // Model and view didn't change, so it was the projection matrix.
        glMatrixMode(GL_PROJECTION);
        glLoadMatrixf(DK_GetProjectionMatrix()->m);
    }

    // Pre-compute model-view-projection transform.
    mmulm(&matrix.mvp, DK_GetProjectionMatrix(), &matrix.mv);
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
        matrix.model[stack.model] = IDENTITY_MATRIX4;
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

const mat4* DK_GetModelMatrix(void) {
    return &matrix.model[stack.model];
}

const mat4* DK_GetViewMatrix(void) {
    return &matrix.view[stack.view];
}

const mat4* DK_GetProjectionMatrix(void) {
    return &matrix.projection[stack.projection];
}

const mat4* DK_GetModelViewMatrix(void) {
    return &matrix.mv;
}

const mat4* DK_GetModelViewProjectionMatrix(void) {
    return &matrix.mvp;
}

int DK_PushModelMatrix(void) {
    return pushModel();
}

int DK_PopModelMatrix(void) {
    return popModel();
}

void DK_SetModelMatrix(const mat4* m) {
    matrix.model[stack.model] = *m;

    updateMatrices(1);
}

///////////////////////////////////////////////////////////////////////////////
// Projection and view matrix generation
///////////////////////////////////////////////////////////////////////////////

int DK_BeginPerspective(void) {
    const float f = 1.0f / tanf(DK_field_of_view * (PI / 360.0f));
    float* m = matrix.projection[stack.projection].m;

    if (!pushProjection()) {
        return 0;
    }

#define M(row, col) m[col * 4 + row]
    M(0, 0) = f / DK_ASPECT_RATIO;
    M(1, 1) = f;
    M(2, 2) = (DK_CLIP_FAR + DK_CLIP_NEAR) / (DK_CLIP_NEAR - DK_CLIP_FAR);
    M(2, 3) = (2.0f * DK_CLIP_FAR * DK_CLIP_NEAR) / (DK_CLIP_NEAR - DK_CLIP_FAR);
    M(3, 2) = -1.0f;
    M(3, 3) = 0.0f;
#undef M

    updateMatrices(0);

    return 1;
}

int DK_EndPerspective(void) {
    return popProjection();
}

int DK_BeginOrthogonal(void) {
    float* m = matrix.projection[stack.projection].m;

    if (!pushProjection()) {
        return 0;
    }

#define M(row, col) m[col * 4 + row]
    M(0, 0) = 2.0f / DK_resolution_x;
    M(0, 1) = 0.0f;
    M(0, 2) = 0.0f;
    M(0, 3) = -1.0f;

    M(1, 0) = 0.0f;
    M(1, 1) = 2.0f / DK_resolution_y;
    M(1, 2) = 0.0f;
    M(1, 3) = -1.0f;

    M(2, 0) = 0.0f;
    M(2, 1) = 0.0f;
    M(2, 2) = -2.0f / (DK_CLIP_FAR - DK_CLIP_NEAR);
    M(2, 3) = -(DK_CLIP_FAR + DK_CLIP_NEAR) / (DK_CLIP_FAR - DK_CLIP_NEAR);

    M(3, 0) = 0.0f;
    M(3, 1) = 0.0f;
    M(3, 2) = 0.0f;
    M(3, 3) = 1.0f;
#undef M

    updateMatrices(0);

    return 1;
}

int DK_EndOrthogonal(void) {
    return popProjection();
}

int DK_BeginPerspectiveForPicking(float x, float y) {
    if (DK_BeginPerspective()) {
        mat4 look = IDENTITY_MATRIX4;
        mitranslate(&look, DK_resolution_x - 2 * x, DK_resolution_y - 2 * y, 0);
        miscale(&look, DK_resolution_x, DK_resolution_y, 1.0);

        mimulm(&look, DK_GetProjectionMatrix());
        matrix.projection[stack.projection] = look;

        updateMatrices(0);

        return 1;
    }
    return 0;
}

int DK_BeginLookAt(float eyex, float eyey, float eyez, float lookatx, float lookaty, float lookatz) {
    mat4* m = &matrix.view[stack.view];
    vec4 up = {
        {0.0f, 0.0f, 1.0f, 1.0f}
    };
    vec4 direction, right;

    if (!pushView()) {
        return 0;
    }

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

    updateMatrices(1);

    return 1;
}

int DK_EndLookAt(void) {
    return popView();
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
    mmulv(&in, &out, DK_GetProjectionMatrix());

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
