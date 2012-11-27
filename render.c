#include "render.h"

#include <assert.h>
#include <math.h>
#include <float.h>

#include <SDL/SDL.h>
#include <GL/glew.h>

#include "camera.h"
#include "config.h"
#include "cursor.h"
#include "events.h"
#include "frustum.h"
#include "graphics.h"
#include "job.h"
#include "log.h"
#include "map.h"
#include "shader.h"
#include "textures.h"
#include "unit.h"
#include "vmath.h"

/** The maximum number of lights that can show in a tile */
#define MAX_LIGHTS_PER_TILE 16
/** The size of a tile when subdividing the screen space for lighting */
#define LIGHT_TILE_SIZE 32

///////////////////////////////////////////////////////////////////////////////
// Deferred shading variables
///////////////////////////////////////////////////////////////////////////////

/** Holds information on our GBuffer */
static struct {
    /** ID of the frame buffer we use for offscreen rendering */
    GLuint frameBuffer;

    GLuint renderBuffer[3];
    GLuint depthBuffer;
    GLuint texture[4];
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

        /** The actual number of lights set in the following arrays */
        GLint LightCountIndex;

        /** The position of the light, in world space */
        GLint LightPositionIndex;

        /** The diffuse color of the light */
        GLint DiffuseLightColorIndex;

        /** Range of the diffuse light */
        GLint DiffuseLightRangeIndex;

        /** The specular color of the light */
        GLint SpecularLightColorIndex;

        /** Range of the specular light */
        GLint SpecularLightRangeIndex;
    } fs_uniforms;
    
    /** In-memory block corresponding to uniform buffer with light data */
    struct {
        int LightCount;
        float LightPosition[MAX_LIGHTS_PER_TILE * 3];
        float LightDiffuseColor[MAX_LIGHTS_PER_TILE * 3];
        float LightDiffuseRange[MAX_LIGHTS_PER_TILE];
        float LightSpecularColor[MAX_LIGHTS_PER_TILE * 3];
        float LightSpecularRange[MAX_LIGHTS_PER_TILE];
    } fs_uniform_buffer;
} gLightShader;

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
        /** Third depth buffer texture */
        GLint DepthBuffer;
    } fs_uniforms;
} gFogShader;

/** Flag if we're currently in the geometry rendering stage */
static bool gIsGeometryPass = false;

/** Represents lights in the scene */
static const MP_Light** gLights = 0;
static unsigned int gLightCapacity = 0;
static unsigned int gLightCount = 0;

/** Temporary reused list with lights in view frustum */
static const MP_Light** gVisibleLights = 0;
static unsigned int gVisibleLightCapacity = 0;
static unsigned int gVisibleLightCount = 0;

///////////////////////////////////////////////////////////////////////////////
// Shader and GBuffer setup
///////////////////////////////////////////////////////////////////////////////

static void initShaders(void) {
    // Load shader programs for deferred shading.
    const char* outGeometry[] = {"GBuffer0", "GBuffer1", "GBuffer2"};
    const char* outDeferred[1] = {"Color"};

    gGeometryShader.program = MP_LoadProgram("data/shaders/deferredGeometryPass.vert",
                                             "data/shaders/deferredGeometryPass.frag",
                                             outGeometry, 3);
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
        snprintf(name, sizeof (name), "Textures[%d]", i);
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

    gAmbientShader.program = MP_LoadProgram("data/shaders/deferredAmbientPass.vert",
                                            "data/shaders/deferredAmbientPass.frag",
                                            outDeferred, 1);
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

    gLightShader.program = MP_LoadProgram("data/shaders/deferredLightPass.vert",
                                          "data/shaders/deferredLightPass.frag",
                                          outDeferred, 1);
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

    gLightShader.fs_uniforms.LightCountIndex =
            glGetUniformLocation(gLightShader.program, "LightCount");
    gLightShader.fs_uniforms.LightPositionIndex =
            glGetUniformLocation(gLightShader.program, "LightPosition");
    gLightShader.fs_uniforms.DiffuseLightColorIndex =
            glGetUniformLocation(gLightShader.program, "DiffuseLightColor");
    gLightShader.fs_uniforms.DiffuseLightRangeIndex =
            glGetUniformLocation(gLightShader.program, "DiffuseLightRange");
    gLightShader.fs_uniforms.SpecularLightColorIndex =
            glGetUniformLocation(gLightShader.program, "SpecularLightColor");
    gLightShader.fs_uniforms.SpecularLightRangeIndex =
            glGetUniformLocation(gLightShader.program, "SpecularLightRange");

    EXIT_ON_OPENGL_ERROR();

    gFogShader.program = MP_LoadProgram("data/shaders/deferredFogPass.vert",
                                        "data/shaders/deferredFogPass.frag",
                                        outDeferred, 1);
    if (!gFogShader.program) {
        return;
    }
    // Get the uniform/attribute locations from the shader.
    gFogShader.vs_uniforms.ModelViewProjectionMatrix =
            glGetUniformLocation(gFogShader.program, "ModelViewProjectionMatrix");

    gFogShader.vs_attributes.ModelVertex =
            glGetAttribLocation(gFogShader.program, "ModelVertex");
    gFogShader.vs_attributes.TextureCoordinate =
            glGetAttribLocation(gFogShader.program, "TextureCoordinate");

    gFogShader.fs_uniforms.DepthBuffer =
            glGetUniformLocation(gFogShader.program, "DepthBuffer");
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
    glRenderbufferStorage(GL_RENDERBUFFER, internalformat, MP_resolutionX, MP_resolutionY);
    EXIT_ON_OPENGL_ERROR();

    // Bind it to the frame buffer.
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, attachment, GL_RENDERBUFFER, render_buffer);
    EXIT_ON_OPENGL_ERROR();

    // And return it.
    return render_buffer;
}

static GLenum createTexture(GLenum internalformat, GLenum format, GLenum type, GLenum attachment) {
    // Generate and bind the texture.
    GLenum texture;
    glGenTextures(1, &texture);
    EXIT_ON_OPENGL_ERROR();

    glBindTexture(GL_TEXTURE_2D, texture);
    EXIT_ON_OPENGL_ERROR();

    // Allocate it.
    glTexImage2D(GL_TEXTURE_2D, 0, internalformat, MP_resolutionX, MP_resolutionY, 0, format, type, NULL);
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
    gBuffer.texture[0] = createTexture(GL_RGBA16F, GL_RGBA, GL_FLOAT, GL_COLOR_ATTACHMENT0);
    gBuffer.texture[1] = createTexture(GL_RGBA16F, GL_RGBA, GL_FLOAT, GL_COLOR_ATTACHMENT1);
    gBuffer.texture[2] = createTexture(GL_RGBA16F, GL_RGBA, GL_FLOAT, GL_COLOR_ATTACHMENT2);
    gBuffer.texture[3] = createTexture(GL_DEPTH_COMPONENT24, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, GL_DEPTH_ATTACHMENT);

    // Check if all worked fine and unbind the FBO
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        MP_log_fatal("Failed initializing GBuffer.");
    }

    // All done, unbind frame buffer.
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    EXIT_ON_OPENGL_ERROR();
}

///////////////////////////////////////////////////////////////////////////////
// Helper methods
///////////////////////////////////////////////////////////////////////////////

static bool isDeferredShadingPossible(void) {
    return MP_DBG_useDeferredShader &&
            gGeometryShader.program &&
            gAmbientShader.program &&
            gLightShader.program &&
            gFogShader.program;
}

static void onModelMatrixChanged(void) {
    if (gIsGeometryPass && isDeferredShadingPossible()) {
        // Set uniforms for geometry shader.
        glUniformMatrix4fv(gGeometryShader.vs_uniforms.ModelMatrix, 1, GL_FALSE, MP_GetModelMatrix()->m);
        glUniformMatrix4fv(gGeometryShader.vs_uniforms.ModelViewProjectionMatrix, 1, GL_FALSE, MP_GetModelViewProjectionMatrix()->m);
        // TODO Pre-compute the normal matrix.
        //glUniformMatrix4fv(geometry_shader.vs_uniforms.NormalMatrix, 1, GL_FALSE, MP_GetNormalMatrix());

        EXIT_ON_OPENGL_ERROR();
    }
}

///////////////////////////////////////////////////////////////////////////////
// Deferred lighting
///////////////////////////////////////////////////////////////////////////////

static void renderQuad(float startX, float startY, float endX, float endY) {
    const float tx0 = startX / MP_resolutionX;
    const float tx1 = endX / MP_resolutionX;
    const float ty0 = startY / MP_resolutionY;
    const float ty1 = endY / MP_resolutionY;

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
}

static void ambientPass(void) {
    // Don't change the depth mask, don't care for it.
    glDepthMask(GL_FALSE);
    glDisable(GL_DEPTH_TEST);

    // Set projection matrix to orthogonal, because we want to draw a quad
    // filling the complete screen.
    MP_BeginOrthogonal();

    // Set view a bit back to avoid clipping.
    MP_BeginLookAt(1.0f + MP_CLIP_NEAR, 0, 0, 0, 0, 0);

    // No model transform for us.
    MP_PushModelMatrix();
    MP_SetModelMatrix(&IDENTITY_MATRIX4);

    // Use ambient shader program and set uniforms for it.
    glUseProgram(gAmbientShader.program);
    glUniformMatrix4fv(gAmbientShader.vs_uniforms.ModelViewProjectionMatrix, 1, GL_FALSE, MP_GetModelViewProjectionMatrix()->m);
    glUniform1i(gAmbientShader.fs_uniforms.GBuffer0, 0);
    glUniform1f(gAmbientShader.fs_uniforms.AmbientLightPower, MP_AMBIENT_LIGHT_COLOR_POWER);

    // Render the quad.
    renderQuad(0, 0, MP_resolutionX, MP_resolutionY);

    // Done with our program.
    glUseProgram(0);

    // Reset matrices.
    MP_PopModelMatrix();
    MP_EndLookAt();
    MP_EndOrthogonal();

    // Restore state.
    glDepthMask(GL_TRUE);
    glEnable(GL_DEPTH_TEST);

    EXIT_ON_OPENGL_ERROR();
}

static void fogPass(void) {
    // Blend lights additively.
    glEnable(GL_BLEND);
    glBlendFunc(GL_ZERO, GL_ONE_MINUS_SRC_COLOR);

    // No depth testing for orthogonal quad rendering.
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);

    // Set projection matrix to orthogonal, because we want to draw a quad
    // filling the complete screen.
    MP_BeginOrthogonal();

    // Set view a bit back to avoid clipping.
    MP_BeginLookAt(1.0f + MP_CLIP_NEAR, 0, 0, 0, 0, 0);

    // No model transform for us.
    MP_PushModelMatrix();
    MP_SetModelMatrix(&IDENTITY_MATRIX4);

    // Use lighting shader program.
    glUseProgram(gFogShader.program);
    glUniformMatrix4fv(gFogShader.vs_uniforms.ModelViewProjectionMatrix, 1,
                       GL_FALSE, MP_GetModelViewProjectionMatrix()->m);
    glUniform1i(gFogShader.fs_uniforms.DepthBuffer, 0);
    EXIT_ON_OPENGL_ERROR();

    // Render the quad.
    renderQuad(0, 0, MP_resolutionX, MP_resolutionY);

    // Pop the shader.
    glUseProgram(0);

    // Pop orthogonal view state.
    MP_PopModelMatrix();
    MP_EndLookAt();
    MP_EndOrthogonal();

    // Restore old state.
    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);

    EXIT_ON_OPENGL_ERROR();
}

static float clamp(float f) {
    if (f < 0.0f) return 0.0f;
    if (f > 1.0f) return 1.0f;
    return f;
}

static void drawLights(void) {
    int lightCount = 0;
    float positions[MAX_LIGHTS_PER_TILE * 3] = {0};
    float diffuseColors[MAX_LIGHTS_PER_TILE * 3] = {0};
    float diffuseRanges[MAX_LIGHTS_PER_TILE] = {0};
    float specularColors[MAX_LIGHTS_PER_TILE * 3] = {0};
    float specularRanges[MAX_LIGHTS_PER_TILE] = {0};
    frustum viewFrustum, tileFrustum;

    // Get our current (projected view) frustum.
    viewFrustum = *MP_GetRenderFrustum();

    // Find all lights in view frustum.
    gVisibleLightCount = 0;
    for (unsigned int i = 0; i < gLightCount; ++i) {
        const MP_Light* light = gLights[i];
        const float radius = light->diffuseRange > light->specularRange ? light->diffuseRange : light->specularRange;
        if (MP_IsSphereInFrustum(&viewFrustum, &light->position, radius)) {
            // Make sure our storage is big enough.
            if (gVisibleLightCount >= gVisibleLightCapacity) {
                gVisibleLightCapacity = gVisibleLightCapacity * 3 / 2 + 1;
                if (!(gVisibleLights = realloc(gVisibleLights, sizeof (MP_Light*) * gVisibleLightCapacity))) {
                    MP_log_fatal("Out of memory while allocating visible light data.\n");
                }
            }
            gVisibleLights[gVisibleLightCount++] = light;
        }
    }

    // Skip rest if there are no visible lights.
    if (gVisibleLightCount == 0) {
        return;
    }

    // Disable testing and changing the depth buffer (we just paint on top of
    // what we have with an orthogonally viewed quad).
    glDepthMask(GL_FALSE);
    glDisable(GL_DEPTH_TEST);

    // Blend lights additively.
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE);

    // Set projection matrix to orthogonal, because we want to draw a quad
    // filling the complete screen.
    MP_BeginOrthogonal();

    // Set view a bit back to avoid clipping.
    MP_BeginLookAt(10.0f + MP_CLIP_NEAR, 0, 0, 0, 0, 0);

    // No model transform for us.
    MP_PushModelMatrix();
    MP_SetModelMatrix(&IDENTITY_MATRIX4);

    // Use lighting shader program.
    glUseProgram(gLightShader.program);
    glUniformMatrix4fv(gLightShader.vs_uniforms.ModelViewProjectionMatrix, 1,
                       GL_FALSE, MP_GetModelViewProjectionMatrix()->m);
    glUniform1i(gLightShader.fs_uniforms.GBuffer0, 0);
    glUniform1i(gLightShader.fs_uniforms.GBuffer1, 1);
    glUniform1i(gLightShader.fs_uniforms.GBuffer2, 2);
    glUniform3fv(gLightShader.fs_uniforms.CameraPosition, 1, MP_GetCameraPosition()->v);
    EXIT_ON_OPENGL_ERROR();

    // Render the screen space in tiles, where for each tile we check the lights
    // interacting with that tile again (from the already filtered list).
    for (int y = 0; y < MP_resolutionY; y += LIGHT_TILE_SIZE) {
        // We generate the frustum for the tile by interpolating the left and
        // right / top and bottom planes based on our tile position.
        float v0 = clamp(y / (float) MP_resolutionY);
        float v1 = clamp((y + LIGHT_TILE_SIZE) / (float) MP_resolutionY);
        for (int x = 0; x < MP_resolutionX; x += LIGHT_TILE_SIZE) {
            float u0 = clamp(x / (float) MP_resolutionX);
            float u1 = clamp((x + LIGHT_TILE_SIZE) / (float) MP_resolutionX);
            MP_FrustumSegment(&tileFrustum, &viewFrustum, u0, v0, u1, v1);
            // Find lights relevant to this quad.
            lightCount = 0;
            for (unsigned int i = 0; i < gVisibleLightCount && lightCount < MAX_LIGHTS_PER_TILE; ++i) {
                const MP_Light* light = gVisibleLights[i];
                const float radius = light->diffuseRange > light->specularRange ? light->diffuseRange : light->specularRange;
                // Check if the light intersects with the frustum of this tile.
                if (MP_IsSphereInFrustum(&tileFrustum, &light->position, radius)) {
                    positions[lightCount * 3] = light->position.v[0];
                    positions[lightCount * 3 + 1] = light->position.v[1];
                    positions[lightCount * 3 + 2] = light->position.v[2];
                    diffuseColors[lightCount * 3] = light->diffuseColor.v[0];
                    diffuseColors[lightCount * 3 + 1] = light->diffuseColor.v[1];
                    diffuseColors[lightCount * 3 + 2] = light->diffuseColor.v[2];
                    diffuseRanges[lightCount] = light->diffuseRange;
                    specularColors[lightCount * 3] = light->specularColor.v[0];
                    specularColors[lightCount * 3 + 1] = light->specularColor.v[1];
                    specularColors[lightCount * 3 + 2] = light->specularColor.v[2];
                    specularRanges[lightCount] = light->specularRange;
                    ++lightCount;
                }
            }
            if (lightCount > 0) {
                // Set data for these lights.
                glUniform1i(gLightShader.fs_uniforms.LightCountIndex, lightCount);
                glUniform3fv(gLightShader.fs_uniforms.LightPositionIndex, MAX_LIGHTS_PER_TILE, positions);
                glUniform3fv(gLightShader.fs_uniforms.DiffuseLightColorIndex, MAX_LIGHTS_PER_TILE, diffuseColors);
                glUniform1fv(gLightShader.fs_uniforms.DiffuseLightRangeIndex, MAX_LIGHTS_PER_TILE, diffuseRanges);
                glUniform3fv(gLightShader.fs_uniforms.SpecularLightColorIndex, MAX_LIGHTS_PER_TILE, specularColors);
                glUniform1fv(gLightShader.fs_uniforms.SpecularLightRangeIndex, MAX_LIGHTS_PER_TILE, specularRanges);

                // Render the quad. OpenGL's coordinate system starts in the
                // bottom left, but we use one in the upper left, so we need to
                // invert y-coordinate.
                renderQuad(x, MP_resolutionY - (y + LIGHT_TILE_SIZE), x + LIGHT_TILE_SIZE, MP_resolutionY - y);
            }
        }
    }

    // Pop the shader.
    glUseProgram(0);

    // Pop orthogonal view state.
    MP_PopModelMatrix();
    MP_EndLookAt();
    MP_EndOrthogonal();

    // Restore old state.
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);

    EXIT_ON_OPENGL_ERROR();
}

static void lightPass(void) {
    // Done with our frame buffer.
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

    // Copy over the depth buffer from our frame buffer, to preserve the depths
    // in post-rendering (e.g. for lighting and selection outline).
    glBindFramebuffer(GL_READ_FRAMEBUFFER, gBuffer.frameBuffer);
    glBlitFramebuffer(0, 0, MP_resolutionX, MP_resolutionY,
                      0, 0, MP_resolutionX, MP_resolutionY,
                      GL_DEPTH_BUFFER_BIT, GL_NEAREST);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);

    // Figure out what to draw.
    if (MP_DBG_deferredBuffer == MP_DBG_BUFFER_FINAL) {
        // Set our three g-buffer textures.
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, gBuffer.texture[0]);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, gBuffer.texture[1]);
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, gBuffer.texture[2]);

        // Do ambient lighting pass.
        ambientPass();

        // Draw lights.
        drawLights();

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, gBuffer.texture[3]);
        fogPass();
    } else {
        glActiveTexture(GL_TEXTURE0);
        switch (MP_DBG_deferredBuffer) {
            case MP_DBG_BUFFER_DIFFUSE:
                glBindTexture(GL_TEXTURE_2D, gBuffer.texture[0]);
                break;
            case MP_DBG_BUFFER_POSITION:
                glBindTexture(GL_TEXTURE_2D, gBuffer.texture[1]);
                break;
            case MP_DBG_BUFFER_NORMALS:
                glBindTexture(GL_TEXTURE_2D, gBuffer.texture[2]);
                break;
            case MP_DBG_BUFFER_DEPTH:
                glBindTexture(GL_TEXTURE_2D, gBuffer.texture[3]);
                break;
            default:
                break;
        }

        // Not using a shader, so enable textures in fixed function pipeline.
        glEnable(GL_TEXTURE_2D);

        // Set projection matrix to orthogonal, because we want to draw a quad
        // filling the complete screen.
        MP_BeginOrthogonal();

        // Set view a bit back to avoid clipping.
        MP_BeginLookAt(1.0f + MP_CLIP_NEAR, 0, 0, 0, 0, 0);

        // No model transform for us.
        MP_PushModelMatrix();
        MP_SetModelMatrix(&IDENTITY_MATRIX4);

        // Render the quad.
        renderQuad(0, 0, MP_resolutionX, MP_resolutionY);

        // Reset matrices.
        MP_PopModelMatrix();
        MP_EndLookAt();
        MP_EndOrthogonal();

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

void MP_Render(void) {
    // Set camera position.
    const vec3* cameraPosition = MP_GetCameraPosition();
    const vec3* cameraTarget = MP_GetCameraTarget();
    MP_BeginLookAt(cameraPosition->d.x, cameraPosition->d.y, cameraPosition->d.z,
                   cameraTarget->d.x, cameraTarget->d.y, cameraTarget->d.z);

    // Set projection matrix.
    if (MP_DBG_drawPickingMode) {
        int x, y;
        SDL_GetMouseState(&x, &y);
        MP_BeginPerspectiveForPicking(x, MP_resolutionY - y);
    } else {
        MP_BeginPerspective();
    }

    // Reset model transform.
    MP_PushModelMatrix();
    MP_SetModelMatrix(&IDENTITY_MATRIX4);

    // Clear to black and set default vertex color to white.
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClearDepth(1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

    // Trigger pre render hooks.
    MP_DispatchPreRenderEvent();

    if (isDeferredShadingPossible()) {
        // Begin geometry pass for deferred shading.
        static const GLenum buffers[] = {GL_COLOR_ATTACHMENT0,
                                         GL_COLOR_ATTACHMENT1,
                                         GL_COLOR_ATTACHMENT2};

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

        gIsGeometryPass = true;

        // Push current matrix state to shader.
        onModelMatrixChanged();
    }

    // Render game components.
    MP_DispatchRenderEvent();

    if (isDeferredShadingPossible()) {
        // Stop using geometry shader.
        glUseProgram(0);
        gIsGeometryPass = false;

        // Do our light pass.
        lightPass();
    }

    // Trigger post render hooks.
    MP_DispatchPostRenderEvent();

    MP_PopModelMatrix();
    MP_EndPerspective();
    MP_EndLookAt();
}

void MP_InitRender(void) {
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
    glViewport(0, 0, MP_resolutionX, MP_resolutionY);
    EXIT_ON_OPENGL_ERROR();

    // Set our projection matrix to match our viewport. Null the OpenGL internal
    // ones, as we'll use our own because we use shaders, but we still set the
    // OpenGL ones for test purposes (picking, alternative rendering).
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    MP_AddModelMatrixChangedEventListener(onModelMatrixChanged);
}

///////////////////////////////////////////////////////////////////////////////
// Material
///////////////////////////////////////////////////////////////////////////////

GLint MP_GetPositionAttributeLocation(void) {
    return gGeometryShader.vs_attributes.ModelVertex;
}

GLint MP_GetNormalAttributeLocation(void) {
    return gGeometryShader.vs_attributes.ModelNormal;
}

GLint MP_GetTextureCoordinateAttributeLocation(void) {
    return gGeometryShader.vs_attributes.TextureCoordinate;
}

void MP_SetMaterial(const MP_Material* material) {
    if (gIsGeometryPass) {
        for (unsigned int i = 0; i < MP_MAX_MATERIAL_TEXTURES; ++i) {
            glActiveTexture(GL_TEXTURE0 + i);
            glDisable(GL_TEXTURE_2D);
            glBindTexture(GL_TEXTURE_2D, 0);
        }

        if (isDeferredShadingPossible()) {
            glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

            for (unsigned int i = 0; i < material->textureCount; ++i) {
                glActiveTexture(GL_TEXTURE0 + i);
                glBindTexture(GL_TEXTURE_2D, material->textures[i]);
                glUniform1i(gGeometryShader.fs_uniforms.Textures[i], i);
            }
            glUniform1i(gGeometryShader.fs_uniforms.TextureCount,
                        material->textureCount);

            glUniform3fv(gGeometryShader.fs_uniforms.ColorDiffuse, 1,
                         material->diffuseColor.v);
            glUniform1f(gGeometryShader.fs_uniforms.SpecularIntensity,
                        material->specularIntensity);
            glUniform1f(gGeometryShader.fs_uniforms.SpecularExponent,
                        material->specularExponent);
            glUniform1f(gGeometryShader.fs_uniforms.Emissivity,
                        material->emissivity);

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

        EXIT_ON_OPENGL_ERROR();
    } else {
        // Otherwise we're not really interested in shading stuff properly, just
        // set the color.
        glColor4f(material->diffuseColor.c.r,
                  material->diffuseColor.c.g,
                  material->diffuseColor.c.b,
                  material->diffuseColor.c.a);
    }
}

void MP_InitMaterial(MP_Material* material) {
    for (unsigned int i = 0; i < MP_MAX_MATERIAL_TEXTURES; ++i) {
        material->textures[i] = 0;
    }
    material->textureCount = 0;
    material->diffuseColor.c.r = 1.0f;
    material->diffuseColor.c.g = 1.0f;
    material->diffuseColor.c.b = 1.0f;
    material->diffuseColor.c.a = 1.0f;
    material->specularIntensity = 0.0f;
    material->specularExponent = 1.0f;
    material->emissivity = 0.0f;
    material->bumpMap = 0;
    material->normalMap = 0;
}

void MP_AddLight(const MP_Light* light) {
    // Find the light.
    for (unsigned int i = 0; i < gLightCount; ++i) {
        if (gLights[i] == light) {
            // Already have that light.
            return;
        }
    }

    // Not yet in list.
    if (gLightCount >= gLightCapacity) {
        gLightCapacity = gLightCapacity * 3 / 2 + 1;
        if (!(gLights = realloc(gLights, gLightCapacity * sizeof (MP_Light*)))) {
            MP_log_fatal("Out of memory while allocating light data.\n");
        }
    }

    // Save pointer.
    gLights[gLightCount++] = light;
}

bool MP_RemoveLight(const MP_Light* light) {
    // Find the light.
    for (unsigned int i = 0; i < gLightCount; ++i) {
        if (gLights[i] == light) {
            // Found it. Close the gap by shifting all following entries one up.
            --gLightCount;
            memmove(&gLights[i], &gLights[i + 1],
                    (gLightCount - i) * sizeof (MP_Light*));
            return true;
        }
    }
    return false;
}

int MP_DEBUG_VisibleLightCount(void) {
    return gVisibleLightCount;
}
