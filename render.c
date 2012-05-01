#include <assert.h>
#include <math.h>
#include <float.h>

#include <SDL/SDL.h>
#include <GL/glew.h>

#include "camera.h"
#include "config.h"
#include "cursor.h"
#include "graphics.h"
#include "jobs.h"
#include "map.h"
#include "render.h"
#include "shader.h"
#include "textures.h"
#include "units.h"
#include "vmath.h"

///////////////////////////////////////////////////////////////////////////////
// Deferred shading variables
///////////////////////////////////////////////////////////////////////////////

/** Holds information on our GBuffer */
static struct {
    /** ID of the frame buffer we use for offscreen rendering */
    GLuint frameBuffer;

    GLuint renderBuffer[3];
    GLuint depthBuffer;
    GLuint texture[3];
} gBuffer;

/** Holds information on our geometry shader */
static struct {
    /** ID of the shader program */
    GLuint program;

    /** Uniforms for the vertex shader */
    struct {
        /** The current model matrix */
        GLint ModelMatrix;

        /** The current model-view-projection matrix */
        GLint ModelViewProjectionMatrix;

        /** The current surface normal matrix */
        GLint NormalMatrix;
    } vs_uniforms;

    /** Attributes for the vertex shader */
    struct {
        /** The vertex position model space */
        GLint ModelVertex;

        /** The vertex normal in model space */
        GLint ModelNormal;

        /** The texture coordinate at the vertex */
        GLint TextureCoordinate;
    } vs_attributes;

    /** Uniforms for the fragment shader */
    struct {
        /** The textures to use */
        GLint Textures[4];

        /** The number of textures to use */
        GLint TextureCount;

        /** The diffuse base color of the material */
        GLint ColorDiffuse;

        /** The specular intensity of the material */
        GLint SpecularIntensity;

        /** The specular exponent of the material */
        GLint SpecularExponent;

        /** The unattenuated color amount to use */
        GLint Emissivity;
    } fs_uniforms;
} gGeometryShader;

static struct {
    /** ID of the shader program */
    GLint program;

    /** Uniforms for the vertex shader */
    struct {
        /** The current model-view-projection matrix */
        GLint ModelViewProjectionMatrix;
    } vs_uniforms;

    /** Attributes for the vertex shader */
    struct {
        /** The vertex position model space */
        GLint ModelVertex;

        /** The texture coordinate at the vertex */
        GLint TextureCoordinate;
    } vs_attributes;

    /** Uniforms for the fragment shader */
    struct {
        /** First GBuffer (diffuse albedo and emissivity) */
        GLint GBuffer0;

        /** Power of ambient light (which is presumed to be white) */
        GLint AmbientLightPower;
    } fs_uniforms;
} gAmbientShader;

static struct {
    /** ID of the shader program */
    GLint program;

    /** Uniforms for the vertex shader */
    struct {
        /** The current model-view-projection matrix */
        GLint ModelViewProjectionMatrix;
    } vs_uniforms;

    /** Attributes for the vertex shader */
    struct {
        /** The vertex position model space */
        GLint ModelVertex;

        /** The texture coordinate at the vertex */
        GLint TextureCoordinate;
    } vs_attributes;

    /** Uniforms for the fragment shader */
    struct {
        /** First GBuffer */
        GLint GBuffer0;

        /** Second GBuffer */
        GLint GBuffer1;

        /** Third GBuffer */
        GLint GBuffer2;

        /** The position of the camera */
        GLint CameraPosition;

        /** The position of the light, in world space */
        GLint LightPosition;

        /** The diffuse color of the light */
        GLint DiffuseLightColor;

        /** Power of the diffuse light */
        GLint DiffuseLightPower;

        /** The specular color of the light */
        GLint SpecularLightColor;

        /** Power of the specular light */
        GLint SpecularLightPower;
    } fs_uniforms;
} gLightShader;

/** Flag if we're currently in the geometry rendering stage */
static int gIsGeometryPass = 0;

/** Represents a single light in a scene */
static const DK_Light** gLights = 0;
static unsigned int gLightCapacity = 0;
static unsigned int gLightCount = 0;

/** Used for rendering light volumes */
static GLuint gSphereArrayID = 0;
static GLuint gSphereBufferID = 0;

///////////////////////////////////////////////////////////////////////////////
// Event callback
///////////////////////////////////////////////////////////////////////////////

/** Callbacks/hooks for different render stages */
static Callbacks* gPreRenderCallbacks = 0;
static Callbacks* gRenderCallbacks = 0;
static Callbacks* gPostRenderCallbacks = 0;

///////////////////////////////////////////////////////////////////////////////
// Sphere rendering (for light volumes)
///////////////////////////////////////////////////////////////////////////////

/** Represents a single face on a sphere */
typedef struct Face {
    vec3 p0, p1, p2;
} Face;

/* Six equidistant points lying on the unit sphere */
#define XPLUS {{  1,  0,  0 }}	/*  X */
#define XMIN  {{ -1,  0,  0 }}	/* -X */
#define YPLUS {{  0,  1,  0 }}	/*  Y */
#define YMIN  {{  0, -1,  0 }}	/* -Y */
#define ZPLUS {{  0,  0,  1 }}	/*  Z */
#define ZMIN  {{  0,  0, -1 }}	/* -Z */
static const Face OCTAHEDRON[] = {
    {XPLUS, YPLUS, ZPLUS},
    {YPLUS, XMIN, ZPLUS},
    {XMIN, YMIN, ZPLUS},
    {YMIN, XPLUS, ZPLUS},
    {XPLUS, ZMIN, YPLUS},
    {YPLUS, ZMIN, XMIN},
    {XMIN, ZMIN, YMIN},
    {YMIN, ZMIN, XPLUS}
};
#undef XPLUS
#undef XMIN
#undef YPLUS
#undef YMIN
#undef ZPLUS
#undef ZMIN

#define ITERATIONS 1
// Number of faces.
const unsigned int faceCount = (4 << (ITERATIONS - 1)) * 8;

static void midPoint(vec3* mid, const vec3* va, const vec3* vb) {
    mid->d.x = (va->d.x + vb->d.x) / 2.0f;
    mid->d.y = (va->d.y + vb->d.y) / 2.0f;
    mid->d.z = (va->d.z + vb->d.z) / 2.0f;
}

static void initSphere(void) {
    // The actual faces.
    Face faces[faceCount];

    // Counters for vertices.
    unsigned int numFaces = 8;

    // Face we're currently building.
    Face* newFace = &faces[numFaces];

    // Initialize from octahedron.
    for (unsigned int i = 0; i < 8; ++i) {
        faces[i] = OCTAHEDRON[i];
    }

    // Bisect each edge and move to the surface of a unit sphere.
    for (unsigned int iteration = 0; iteration < ITERATIONS; ++iteration) {
        for (unsigned int i = 0; i < numFaces; i++) {
            Face* oldFace = &faces[i];
            vec3 pa, pb, pc;

            midPoint(&pa, &oldFace->p0, &oldFace->p2);
            midPoint(&pb, &oldFace->p0, &oldFace->p1);
            midPoint(&pc, &oldFace->p1, &oldFace->p2);

            v3inormalize(&pa);
            v3inormalize(&pb);
            v3inormalize(&pc);

            newFace->p0 = oldFace->p0;
            newFace->p1 = pb;
            newFace->p2 = pa;
            ++newFace;

            newFace->p0 = pb;
            newFace->p1 = oldFace->p1;
            newFace->p2 = pc;
            ++newFace;

            newFace->p0 = pa;
            newFace->p1 = pc;
            newFace->p2 = oldFace->p2;
            ++newFace;

            oldFace->p0 = pa;
            oldFace->p1 = pb;
            oldFace->p2 = pc;
        }
        numFaces *= 4;
    }

    assert(numFaces == faceCount);

    glGenVertexArrays(1, &gSphereArrayID);
    glBindVertexArray(gSphereArrayID);
    glGenBuffers(1, &gSphereBufferID);
    glBindBuffer(GL_ARRAY_BUFFER, gSphereBufferID);
    glBufferData(GL_ARRAY_BUFFER, faceCount * sizeof (Face), faces, GL_STATIC_DRAW);
}

static void drawSphere(void) {
    // Bind our vertex buffer as the array we use for attribute lookups.
    glBindBuffer(GL_ARRAY_BUFFER, gSphereBufferID);

    // Position is at 0 in fixed function pipeline.
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

    glDrawArrays(GL_TRIANGLES, 0, faceCount * 3);

    // Done with the rendering, unbind attributes.
    glDisableVertexAttribArray(0);

    // Unbind vertex buffer.
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    EXIT_ON_OPENGL_ERROR();
}

#undef ITERATIONS

///////////////////////////////////////////////////////////////////////////////
// Shader and GBuffer setup
///////////////////////////////////////////////////////////////////////////////

static void initShaders(void) {
    // Load shader programs for deferred shading.
    const char* outGeometry[] = {"GBuffer0", "GBuffer1", "GBuffer2"};
    const char* outAmbientAndLight[1] = {"Color"};

    gGeometryShader.program = DK_LoadProgram("data/shaders/deferredGeometryPass.vert", "data/shaders/deferredGeometryPass.frag", outGeometry, 3);
    if (!gGeometryShader.program) {
        return;
    }
    // Get the uniform/attribute locations from the shader.
    gGeometryShader.vs_uniforms.ModelMatrix =
            glGetUniformLocation(gGeometryShader.program, "ModelMatrix");
    gGeometryShader.vs_uniforms.ModelViewProjectionMatrix =
            glGetUniformLocation(gGeometryShader.program, "ModelViewProjectionMatrix");
    gGeometryShader.vs_uniforms.NormalMatrix =
            glGetUniformLocation(gGeometryShader.program, "NormalMatrix");

    gGeometryShader.vs_attributes.ModelVertex =
            glGetAttribLocation(gGeometryShader.program, "ModelVertex");
    gGeometryShader.vs_attributes.ModelNormal =
            glGetAttribLocation(gGeometryShader.program, "ModelNormal");
    gGeometryShader.vs_attributes.TextureCoordinate =
            glGetAttribLocation(gGeometryShader.program, "TextureCoordinate");

    for (unsigned int i = 0; i < 4; ++i) {
        char name[16];
        sprintf(name, "Textures[%d]", i);
        gGeometryShader.fs_uniforms.Textures[i] =
                glGetUniformLocation(gGeometryShader.program, name);
    }
    gGeometryShader.fs_uniforms.TextureCount =
            glGetUniformLocation(gGeometryShader.program, "TextureCount");
    gGeometryShader.fs_uniforms.ColorDiffuse =
            glGetUniformLocation(gGeometryShader.program, "ColorDiffuse");
    gGeometryShader.fs_uniforms.SpecularIntensity =
            glGetUniformLocation(gGeometryShader.program, "SpecularIntensity");
    gGeometryShader.fs_uniforms.SpecularExponent =
            glGetUniformLocation(gGeometryShader.program, "SpecularExponent");
    gGeometryShader.fs_uniforms.Emissivity =
            glGetUniformLocation(gGeometryShader.program, "Emissivity");
    EXIT_ON_OPENGL_ERROR();

    gAmbientShader.program = DK_LoadProgram("data/shaders/deferredAmbientPass.vert", "data/shaders/deferredAmbientPass.frag", outAmbientAndLight, 1);
    if (!gAmbientShader.program) {
        return;
    }
    // Get the uniform/attribute locations from the shader.
    gAmbientShader.vs_uniforms.ModelViewProjectionMatrix =
            glGetUniformLocation(gAmbientShader.program, "ModelViewProjectionMatrix");

    gAmbientShader.vs_attributes.ModelVertex =
            glGetAttribLocation(gAmbientShader.program, "ModelVertex");
    gAmbientShader.vs_attributes.TextureCoordinate =
            glGetAttribLocation(gAmbientShader.program, "TextureCoordinate");

    gAmbientShader.fs_uniforms.GBuffer0 =
            glGetUniformLocation(gAmbientShader.program, "GBuffer0");

    gAmbientShader.fs_uniforms.AmbientLightPower =
            glGetUniformLocation(gAmbientShader.program, "AmbientLightPower");
    EXIT_ON_OPENGL_ERROR();

    gLightShader.program = DK_LoadProgram("data/shaders/deferredLightPass.vert", "data/shaders/deferredLightPass.frag", outAmbientAndLight, 1);
    if (!gLightShader.program) {
        return;
    }
    // Get the uniform/attribute locations from the shader.
    gLightShader.vs_uniforms.ModelViewProjectionMatrix =
            glGetUniformLocation(gLightShader.program, "ModelViewProjectionMatrix");

    gLightShader.vs_attributes.ModelVertex =
            glGetAttribLocation(gLightShader.program, "ModelVertex");
    gLightShader.vs_attributes.TextureCoordinate =
            glGetAttribLocation(gLightShader.program, "TextureCoordinate");

    gLightShader.fs_uniforms.GBuffer0 =
            glGetUniformLocation(gLightShader.program, "GBuffer0");
    gLightShader.fs_uniforms.GBuffer1 =
            glGetUniformLocation(gLightShader.program, "GBuffer1");
    gLightShader.fs_uniforms.GBuffer2 =
            glGetUniformLocation(gLightShader.program, "GBuffer2");

    gLightShader.fs_uniforms.CameraPosition =
            glGetUniformLocation(gLightShader.program, "CameraPosition");
    gLightShader.fs_uniforms.LightPosition =
            glGetUniformLocation(gLightShader.program, "LightPosition");
    gLightShader.fs_uniforms.DiffuseLightColor =
            glGetUniformLocation(gLightShader.program, "DiffuseLightColor");
    gLightShader.fs_uniforms.DiffuseLightPower =
            glGetUniformLocation(gLightShader.program, "DiffuseLightPower");
    gLightShader.fs_uniforms.SpecularLightColor =
            glGetUniformLocation(gLightShader.program, "SpecularLightColor");
    gLightShader.fs_uniforms.SpecularLightPower =
            glGetUniformLocation(gLightShader.program, "SpecularLightPower");
    EXIT_ON_OPENGL_ERROR();
}

static GLenum createRenderBuffer(GLenum internalformat, GLenum attachment) {
    // Generate and bind the render buffer.
    GLuint render_buffer;
    glGenRenderbuffers(1, &render_buffer);
    EXIT_ON_OPENGL_ERROR();

    glBindRenderbuffer(GL_RENDERBUFFER, render_buffer);
    EXIT_ON_OPENGL_ERROR();

    // Set it up.
    glRenderbufferStorage(GL_RENDERBUFFER, internalformat, DK_resolution_x, DK_resolution_y);
    EXIT_ON_OPENGL_ERROR();

    // Bind it to the frame buffer.
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, attachment, GL_RENDERBUFFER, render_buffer);
    EXIT_ON_OPENGL_ERROR();

    // And return it.
    return render_buffer;
}

static GLenum createTexture(GLenum internalformat, GLenum type, GLenum attachment) {
    // Generate and bind the texture.
    GLenum texture;
    glGenTextures(1, &texture);
    EXIT_ON_OPENGL_ERROR();

    glBindTexture(GL_TEXTURE_2D, texture);
    EXIT_ON_OPENGL_ERROR();

    // Allocate it.
    glTexImage2D(GL_TEXTURE_2D, 0, internalformat, DK_resolution_x, DK_resolution_y, 0, GL_RGBA, type, NULL);
    EXIT_ON_OPENGL_ERROR();

    // Set some more parameters.
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    EXIT_ON_OPENGL_ERROR();

    // Attach it to the frame buffer.
    glFramebufferTexture2D(GL_FRAMEBUFFER, attachment, GL_TEXTURE_2D, texture, 0);
    EXIT_ON_OPENGL_ERROR();

    // And return it.
    return texture;
}

static void initGBuffer(void) {
    // Generate and bind our frame buffer.
    glGenFramebuffers(1, &gBuffer.frameBuffer);
    EXIT_ON_OPENGL_ERROR();

    glBindFramebuffer(GL_FRAMEBUFFER, gBuffer.frameBuffer);
    EXIT_ON_OPENGL_ERROR();

    // Create our render buffers.
    gBuffer.renderBuffer[0] = createRenderBuffer(GL_RGBA16F, GL_COLOR_ATTACHMENT0);
    gBuffer.renderBuffer[1] = createRenderBuffer(GL_RGBA16F, GL_COLOR_ATTACHMENT1);
    gBuffer.renderBuffer[2] = createRenderBuffer(GL_RGBA16F, GL_COLOR_ATTACHMENT2);
    gBuffer.depthBuffer = createRenderBuffer(GL_DEPTH_COMPONENT24, GL_DEPTH_ATTACHMENT);

    // Create our textures.
    gBuffer.texture[0] = createTexture(GL_RGBA16F, GL_FLOAT, GL_COLOR_ATTACHMENT0);
    gBuffer.texture[1] = createTexture(GL_RGBA16F, GL_FLOAT, GL_COLOR_ATTACHMENT1);
    gBuffer.texture[2] = createTexture(GL_RGBA16F, GL_FLOAT, GL_COLOR_ATTACHMENT2);

    // Check if all worked fine and unbind the FBO
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        fprintf(DK_log_target, "ERROR: Can't initialize an FBO render texture. FBO initialization failed.");
        exit(EXIT_FAILURE);
    }

    // All done, unbind frame buffer.
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    EXIT_ON_OPENGL_ERROR();
}

///////////////////////////////////////////////////////////////////////////////
// Helper methods
///////////////////////////////////////////////////////////////////////////////

static void onModelMatrixChanged(void) {
    if (gIsGeometryPass) {
        // Set uniforms for geometry shader.
        glUniformMatrix4fv(gGeometryShader.vs_uniforms.ModelMatrix, 1, GL_FALSE, DK_GetModelMatrix()->m);
        glUniformMatrix4fv(gGeometryShader.vs_uniforms.ModelViewProjectionMatrix, 1, GL_FALSE, DK_GetModelViewProjectionMatrix()->m);
        // TODO Pre-compute the normal matrix.
        //glUniformMatrix4fv(geometry_shader.vs_uniforms.NormalMatrix, 1, GL_FALSE, DK_GetNormalMatrix());

        EXIT_ON_OPENGL_ERROR();
    }
}

///////////////////////////////////////////////////////////////////////////////
// Deferred lighting
///////////////////////////////////////////////////////////////////////////////

static void geometryPass(void) {
    static const GLenum buffers[] = {
        GL_COLOR_ATTACHMENT0,
        GL_COLOR_ATTACHMENT1,
        GL_COLOR_ATTACHMENT2
    };

    // Bind our frame buffer.
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, gBuffer.frameBuffer);

    // Clear the render targets.
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Start using the geometry shader.
    glUseProgram(gGeometryShader.program);

    // Use our three buffers.
    glDrawBuffers(3, buffers);

    EXIT_ON_OPENGL_ERROR();
}

static void renderQuad(float startX, float startY, float endX, float endY) {
    const float tx0 = startX / DK_resolution_x;
    const float tx1 = endX / DK_resolution_x;
    const float ty0 = startY / DK_resolution_y;
    const float ty1 = endY / DK_resolution_y;

    // Render the quad.
    glBegin(GL_QUADS);
    {
        glTexCoord2f(tx0, ty0);
        glVertex3f(0.0f, startX, startY);
        glTexCoord2f(tx1, ty0);
        glVertex3f(0.0f, endX, startY);
        glTexCoord2f(tx1, ty1);
        glVertex3f(0.0f, endX, endY);
        glTexCoord2f(tx0, ty1);
        glVertex3f(0.0f, startX, endY);
    }
    glEnd();

    EXIT_ON_OPENGL_ERROR();
}

static void ambientPass(void) {
    // Don't change the depth mask, don't care for it.
    glDepthMask(GL_FALSE);
    glDisable(GL_DEPTH_TEST);

    // Set projection matrix to orthogonal, because we want to draw a quad
    // filling the complete screen.
    DK_BeginOrthogonal();

    // Set view a bit back to avoid clipping.
    DK_BeginLookAt(1.0f + DK_CLIP_NEAR, 0, 0, 0, 0, 0);

    // No model transform for us.
    DK_PushModelMatrix();
    DK_SetModelMatrix(&IDENTITY_MATRIX4);

    // Use ambient shader program and set uniforms for it.
    glUseProgram(gAmbientShader.program);
    glUniformMatrix4fv(gAmbientShader.vs_uniforms.ModelViewProjectionMatrix, 1, GL_FALSE, DK_GetModelViewProjectionMatrix()->m);
    glUniform1i(gAmbientShader.fs_uniforms.GBuffer0, 0);
    glUniform1f(gAmbientShader.fs_uniforms.AmbientLightPower, 0.1f);

    // Render the quad.
    renderQuad(0, 0, DK_resolution_x, DK_resolution_y);

    // Done with our program.
    glUseProgram(0);

    // Reset matrices.
    DK_PopModelMatrix();
    DK_EndLookAt();
    DK_EndOrthogonal();

    // Restore state.
    glDepthMask(GL_TRUE);
    glEnable(GL_DEPTH_TEST);

    EXIT_ON_OPENGL_ERROR();
}

static void drawLight(const DK_Light* light) {
    // Get the radius of the light (i.e. how far from the center it has no
    // noticeable effect anymore).
    const float range = (light->diffusePower > light->specularPower ? light->diffusePower : light->specularPower);
    const float cameraToLight = v3distance(&light->position, DK_GetCameraPosition());
    const int cameraIsInLightVolume = cameraToLight <= range * 1.42f;

    // Translate to the light.
    DK_PushModelMatrix();
    DK_TranslateModelMatrix(light->position.d.x, light->position.d.y, light->position.d.z);
    DK_ScaleModelMatrix(range, range, range);

    // Disable changing the depth buffer and color output.
    glDepthMask(GL_FALSE);
    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);

    // Use stencil buffer to determine which pixels to shade.
    glEnable(GL_STENCIL_TEST);

    // Clear stencil buffer.
    glClearStencil(0);
    glClear(GL_STENCIL_BUFFER_BIT);

    // First pass: front faces, i.e. the light volume from the outside, but only
    // *if* we're on the outside. Otherwise the second pass will be the deciding
    // one.
    if (!cameraIsInLightVolume) {
        // Take everything we can get.
        glStencilFunc(GL_ALWAYS, 1, 1);
        glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);

        // Closer to light than existing geometry.
        glDepthFunc(GL_LESS);

        drawSphere();

        // For second pass, we un-set those pixels that are visible on the
        // inside, too, because those are "in thin air".
        glStencilFunc(GL_EQUAL, 1, 1);
        glStencilOp(GL_ZERO, GL_ZERO, GL_KEEP);
    } else {
        // Only second pass, use everything we get that's "behind" something of
        // the scene already rendered, meaning the light volume collided with it
        // (and thus that it is lit).
        glStencilFunc(GL_ALWAYS, 1, 1);
        glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
    }

    // Second pass: back faces, i.e. light volume from the inside. Only where
    // it's further away than existing geometry.
    glDepthFunc(GL_GEQUAL);

    // Draw back faces.
    glFrontFace(GL_CW);

    drawSphere();

    // Reset to front faces.
    glFrontFace(GL_CCW);

    // Translate back away from light.
    DK_PopModelMatrix();

    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glDepthFunc(GL_LEQUAL);

    glStencilFunc(GL_EQUAL, 1, 1);
    glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);

    // Blend lights additively.
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE);

    // No depth testing for orthogonal quad rendering.
    glDisable(GL_DEPTH_TEST);

    // Set projection matrix to orthogonal, because we want to draw a quad
    // filling the complete screen.
    DK_BeginOrthogonal();

    // Set view a bit back to avoid clipping.
    DK_BeginLookAt(1.0f + DK_CLIP_NEAR, 0, 0, 0, 0, 0);

    // No model transform for us.
    DK_PushModelMatrix();
    DK_SetModelMatrix(&IDENTITY_MATRIX4);

    if (DK_d_draw_light_volumes) {
        glColor3f(0.05f, 0.05f, 0.05f);
        // Render the quad.
        renderQuad(0, 0, DK_resolution_x, DK_resolution_y);
        glColor3f(1.0f, 1.0f, 1.0f);
    }

    // Use lighting shader program.
    glUseProgram(gLightShader.program);
    glUniformMatrix4fv(gLightShader.vs_uniforms.ModelViewProjectionMatrix, 1, GL_FALSE, DK_GetModelViewProjectionMatrix()->m);
    glUniform1i(gLightShader.fs_uniforms.GBuffer0, 0);
    glUniform1i(gLightShader.fs_uniforms.GBuffer1, 1);
    glUniform1i(gLightShader.fs_uniforms.GBuffer2, 2);
    glUniform3fv(gLightShader.fs_uniforms.CameraPosition, 1, DK_GetCameraPosition()->v);
    glUniform3fv(gLightShader.fs_uniforms.DiffuseLightColor, 1, light->diffuseColor.v);
    glUniform1f(gLightShader.fs_uniforms.DiffuseLightPower, light->diffusePower);
    glUniform3fv(gLightShader.fs_uniforms.SpecularLightColor, 1, light->specularColor.v);
    glUniform1f(gLightShader.fs_uniforms.SpecularLightPower, light->specularPower);
    glUniform3fv(gLightShader.fs_uniforms.LightPosition, 1, light->position.v);

    // Render the quad.
    renderQuad(0, 0, DK_resolution_x, DK_resolution_y);

    // Pop the shader.
    glUseProgram(0);

    // Pop orthogonal view state.
    DK_PopModelMatrix();
    DK_EndLookAt();
    DK_EndOrthogonal();

    // Restore old state.
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    glDisable(GL_STENCIL_TEST);
    glDepthMask(GL_TRUE);

    EXIT_ON_OPENGL_ERROR();
}

static void lightPass(void) {
    // Stop using geometry shader.
    glUseProgram(0);

    // Done with our frame buffer.
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

    // Copy over the depth buffer from our frame buffer, to preserve the depths
    // in post-rendering (e.g. for lighting and selection outline).
    glBindFramebuffer(GL_READ_FRAMEBUFFER, gBuffer.frameBuffer);
    glBlitFramebuffer(0, 0, DK_resolution_x, DK_resolution_y,
            0, 0, DK_resolution_x, DK_resolution_y,
            GL_DEPTH_BUFFER_BIT, GL_NEAREST);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);

    // Figure out what to draw.
    if (DK_d_draw_deferred == DK_D_DEFERRED_FINAL) {
        // Set our three g-buffer textures.
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, gBuffer.texture[0]);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, gBuffer.texture[1]);
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, gBuffer.texture[2]);

        // Do ambient lighting pass.
        ambientPass();

        // Set uniforms for our shader program.
        glUseProgram(gLightShader.program);
        glUniformMatrix4fv(gLightShader.vs_uniforms.ModelViewProjectionMatrix, 1, GL_FALSE, DK_GetModelViewProjectionMatrix()->m);
        glUniform1i(gLightShader.fs_uniforms.GBuffer0, 0);
        glUniform1i(gLightShader.fs_uniforms.GBuffer1, 1);
        glUniform1i(gLightShader.fs_uniforms.GBuffer2, 2);
        glUniform3fv(gLightShader.fs_uniforms.CameraPosition, 1, DK_GetCameraPosition()->v);
        glUseProgram(0);

        EXIT_ON_OPENGL_ERROR();

        // Draw lights.
        for (unsigned int i = 0; i < gLightCount; ++i) {
            drawLight(gLights[i]);
        }
    } else if (DK_d_draw_deferred == DK_D_DEPTH_BUFFER) {
        glBindFramebuffer(GL_READ_FRAMEBUFFER, gBuffer.frameBuffer);
        glBlitFramebuffer(0, 0, DK_resolution_x, DK_resolution_y,
                0, 0, DK_resolution_x, DK_resolution_y,
                GL_DEPTH_BUFFER_BIT, GL_NEAREST);
        glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);

        EXIT_ON_OPENGL_ERROR();
    } else {
        glActiveTexture(GL_TEXTURE0);
        switch (DK_d_draw_deferred) {
            case DK_D_DEFERRED_DIFFUSE:
                glBindTexture(GL_TEXTURE_2D, gBuffer.texture[0]);
                break;
            case DK_D_DEFERRED_POSITION:
                glBindTexture(GL_TEXTURE_2D, gBuffer.texture[1]);
                break;
            case DK_D_DEFERRED_NORMALS:
                glBindTexture(GL_TEXTURE_2D, gBuffer.texture[2]);
                break;
            default:
                break;
        }

        // Not using a shader, so enable textures in fixed function pipeline.
        glEnable(GL_TEXTURE_2D);

        // Set projection matrix to orthogonal, because we want to draw a quad
        // filling the complete screen.
        DK_BeginOrthogonal();

        // Set view a bit back to avoid clipping.
        DK_BeginLookAt(1.0f + DK_CLIP_NEAR, 0, 0, 0, 0, 0);

        // No model transform for us.
        DK_PushModelMatrix();
        DK_SetModelMatrix(&IDENTITY_MATRIX4);

        // Render the quad.
        renderQuad(0, 0, DK_resolution_x, DK_resolution_y);

        // Reset matrices.
        DK_PopModelMatrix();
        DK_EndLookAt();
        DK_EndOrthogonal();

        // Reset state.
        glDisable(GL_TEXTURE_2D);

        EXIT_ON_OPENGL_ERROR();
    }

    // Unbind the textures we used.
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, 0);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, 0);
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, 0);
    glActiveTexture(GL_TEXTURE4);
    glBindTexture(GL_TEXTURE_2D, 0);

    EXIT_ON_OPENGL_ERROR();
}

///////////////////////////////////////////////////////////////////////////////
// Initialization and rendering
///////////////////////////////////////////////////////////////////////////////

void DK_Render(void) {
    // Set camera position.
    const vec3* cameraPosition = DK_GetCameraPosition();
    const vec3* cameraTarget = DK_GetCameraTarget();
    DK_BeginLookAt(cameraPosition->d.x, cameraPosition->d.y, cameraPosition->d.z,
            cameraTarget->d.x, cameraTarget->d.y, cameraTarget->d.z);

    // Set projection matrix.
    if (DK_d_draw_picking_mode) {
        int x, y;
        SDL_GetMouseState(&x, &y);
        DK_BeginPerspectiveForPicking(x, DK_resolution_y - y);
    } else {
        DK_BeginPerspective();
    }

    // Reset model transform.
    DK_PushModelMatrix();
    DK_SetModelMatrix(&IDENTITY_MATRIX4);

    // Clear to black and set default vertex color to white.
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClearDepth(1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

    // Trigger pre render hooks.
    CB_Call(gPreRenderCallbacks);

    if (DK_d_draw_deferred_shader && gGeometryShader.program && gLightShader.program) {
        geometryPass();
        gIsGeometryPass = 1;
    }

    // Render game components.
    CB_Call(gRenderCallbacks);

    if (DK_d_draw_deferred_shader && gGeometryShader.program && gLightShader.program) {
        gIsGeometryPass = 0;
        lightPass();
    }

    // Trigger post render hooks.
    CB_Call(gPostRenderCallbacks);

    DK_PopModelMatrix();
    DK_EndPerspective();
    DK_EndLookAt();
}

void DK_InitRender(void) {
    // We do use textures, so enable that.
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_MULTISAMPLE);

    // We want triangle surface pixels to be shaded using interpolated values.
    glShadeModel(GL_SMOOTH);

    glClearStencil(0);

    // Also enable depth testing to get stuff in the right order.
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glDepthMask(GL_TRUE);
    EXIT_ON_OPENGL_ERROR();

    // We'll make sure stuff is rotated correctly, so we can cull back-faces.
    glEnable(GL_CULL_FACE);
    glFrontFace(GL_CCW);
    EXIT_ON_OPENGL_ERROR();

    glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
    EXIT_ON_OPENGL_ERROR();

    // Initialize our deferred shaders.
    initShaders();
    initGBuffer();

    // Define our viewport as the size of our window.
    glViewport(0, 0, DK_resolution_x, DK_resolution_y);
    EXIT_ON_OPENGL_ERROR();

    // Set our projection matrix to match our viewport. Null the OpenGL internal
    // ones, as we'll use our own because we use shaders, but we still set the
    // OpenGL ones for test purposes (picking, alternative rendering).
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    DK_OnModelMatrixChanged(onModelMatrixChanged);

    initSphere();
}

///////////////////////////////////////////////////////////////////////////////
// Material
///////////////////////////////////////////////////////////////////////////////

GLint DK_GetPositionAttributeLocation(void) {
    return gGeometryShader.vs_attributes.ModelVertex;
}

GLint DK_GetNormalAttributeLocation(void) {
    return gGeometryShader.vs_attributes.ModelNormal;
}

GLint DK_GetTextureCoordinateAttributeLocation(void) {
    return gGeometryShader.vs_attributes.TextureCoordinate;
}

void DK_SetMaterial(const DK_Material* material) {
    for (unsigned int i = 0; i < DK_MAX_MATERIAL_TEXTURES; ++i) {
        glActiveTexture(GL_TEXTURE0 + i);
        glDisable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, 0);
    }
    EXIT_ON_OPENGL_ERROR();

    if (gIsGeometryPass) {
        glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

        for (unsigned int i = 0; i < material->textureCount; ++i) {
            glActiveTexture(GL_TEXTURE0 + i);
            glBindTexture(GL_TEXTURE_2D, material->textures[i]);
            glUniform1i(gGeometryShader.fs_uniforms.Textures[i], i);
        }
        glUniform1i(gGeometryShader.fs_uniforms.TextureCount, material->textureCount);

        glUniform3fv(gGeometryShader.fs_uniforms.ColorDiffuse, 1, material->diffuseColor.v);
        glUniform1f(gGeometryShader.fs_uniforms.SpecularIntensity, material->specularIntensity);
        glUniform1f(gGeometryShader.fs_uniforms.SpecularExponent, material->specularExponent);
        glUniform1f(gGeometryShader.fs_uniforms.Emissivity, material->emissivity);

        EXIT_ON_OPENGL_ERROR();
    } else {
        glColor4f(material->diffuseColor.c.r,
                material->diffuseColor.c.g,
                material->diffuseColor.c.b,
                material->diffuseColor.c.a);

        if (material->textureCount > 0) {
            glActiveTexture(GL_TEXTURE0);
            glEnable(GL_TEXTURE_2D);
            glBindTexture(GL_TEXTURE_2D, material->textures[0]);
        }
    }
}

void DK_InitMaterial(DK_Material* material) {
    for (unsigned int i = 0; i < DK_MAX_MATERIAL_TEXTURES; ++i) {
        material->textures[i] = 0;
    }
    material->textureCount = 0;
    material->diffuseColor.c.r = 0.0f;
    material->diffuseColor.c.g = 0.0f;
    material->diffuseColor.c.b = 0.0f;
    material->diffuseColor.c.a = 1.0f;
    material->specularIntensity = 0.0f;
    material->specularExponent = 1.0f;
    material->emissivity = 0.0f;
    material->bumpMap = 0;
    material->normalMap = 0;
}

void DK_AddLight(const DK_Light* light) {
    if (gLightCount >= gLightCapacity) {
        gLightCapacity = gLightCapacity * 2 + 1;
        gLights = realloc(gLights, gLightCapacity * sizeof (DK_Light*));
    }

    // Copy values.
    gLights[gLightCount] = light;

    // Increment counter.
    ++gLightCount;
}

int DK_RemoveLight(const DK_Light* light) {
    // Find the light.
    for (unsigned int i = 0; i < gLightCount; ++i) {
        if (gLights[i] == light) {
            // Found it. Close the gap by shifting all following entries one up.
            --gLightCount;
            memmove(&gLights[i], &gLights[i + 1], (gLightCount - i) * sizeof (DK_Light*));
            return 1;
        }
    }
    return 0;
}

///////////////////////////////////////////////////////////////////////////////
// Events
///////////////////////////////////////////////////////////////////////////////

void DK_OnPreRender(callback method) {
    if (!gPreRenderCallbacks) {
        gPreRenderCallbacks = CB_New();
    }
    CB_Add(gPreRenderCallbacks, method);
}

void DK_OnRender(callback method) {
    if (!gRenderCallbacks) {
        gRenderCallbacks = CB_New();
    }
    CB_Add(gRenderCallbacks, method);
}

void DK_OnPostRender(callback method) {
    if (!gPostRenderCallbacks) {
        gPostRenderCallbacks = CB_New();
    }
    CB_Add(gPostRenderCallbacks, method);
}
