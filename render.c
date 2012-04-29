#include <assert.h>
#include <math.h>

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
        GLint Textures;

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

/** Represents a single light in a scene */
static const DK_Light** gLights = 0;
static unsigned int gLightCapacity = 0;
static unsigned int gLightCount = 0;

/** Callbacks/hooks for different render stages */
static Callbacks* gPreRenderCallbacks = 0;
static Callbacks* gRenderCallbacks = 0;
static Callbacks* gPostRenderCallbacks = 0;

/** Flag if we're currently in the geometry rendering stage */
static int gIsGeometryPass = 0;

///////////////////////////////////////////////////////////////////////////////
// Shader and GBuffer setup
///////////////////////////////////////////////////////////////////////////////

static void initShaders(void) {
    // Load shader programs for deferred shading.
    GLuint vs = DK_shader_load("data/shaders/deferredGeometryPass.vert", GL_VERTEX_SHADER);
    GLuint fs = DK_shader_load("data/shaders/deferredGeometryPass.frag", GL_FRAGMENT_SHADER);
    if (vs && fs) {
        const char* out_names[] = {"GBuffer0", "GBuffer1", "GBuffer2"};
        gGeometryShader.program = DK_shader_program(vs, fs, out_names, 3);

        if (gGeometryShader.program) {
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

            gGeometryShader.fs_uniforms.Textures =
                    glGetUniformLocation(gGeometryShader.program, "Textures");
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
        }
    } else {
        gGeometryShader.program = 0;
        return;
    }

    vs = DK_shader_load("data/shaders/deferredAmbientPass.vert", GL_VERTEX_SHADER);
    fs = DK_shader_load("data/shaders/deferredAmbientPass.frag", GL_FRAGMENT_SHADER);
    if (vs && fs) {
        const char* out_names[1] = {"Color"};
        gAmbientShader.program = DK_shader_program(vs, fs, out_names, 1);

        if (gAmbientShader.program) {
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
        }
    } else {
        gAmbientShader.program = 0;
        return;
    }

    vs = DK_shader_load("data/shaders/deferredLightPass.vert", GL_VERTEX_SHADER);
    fs = DK_shader_load("data/shaders/deferredLightPass.frag", GL_FRAGMENT_SHADER);
    if (vs && fs) {
        const char* out_names[1] = {"Color"};
        gLightShader.program = DK_shader_program(vs, fs, out_names, 1);

        if (gLightShader.program) {
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
    } else {
        gLightShader.program = 0;
        return;
    }
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

static void lightPass(void) {
    // Stop using geometry buffer.
    glUseProgram(0);

    // Done with our frame buffer.
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

    // Set projection matrix to orthogonal, because we want to draw a quad
    // filling the complete screen.
    DK_BeginOrthogonal();

    // Set view a bit back to avoid clipping.
    DK_BeginLookAt(1, 0, 0, 0, 0, 0);

    // No model transform for us.
    DK_PushModelMatrix();
    DK_SetModelMatrix(&IDENTITY_MATRIX4);

    // Figure out what to draw.
    if (DK_d_draw_deferred == DK_D_DEFERRED_FINAL) {
        // Do ambient lighting pass.
        glUseProgram(gAmbientShader.program);

        // The the global transformation matrix.
        glUniformMatrix4fv(gAmbientShader.vs_uniforms.ModelViewProjectionMatrix, 1, GL_FALSE, DK_GetModelViewProjectionMatrix()->m);

        // Set our three g-buffer textures.
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, gBuffer.texture[0]);
        glUniform1i(gAmbientShader.fs_uniforms.GBuffer0, 0);

        glUniform1f(gAmbientShader.fs_uniforms.AmbientLightPower, 0.1f);

        // Render the quad.
        glBegin(GL_QUADS);
        {
            glTexCoord2f(0.0f, 0.0f);
            glVertex3f(0.0f, 0.0f, 0.0f);
            glTexCoord2f(1.0f, 0.0f);
            glVertex3f(0.0f, (float) DK_resolution_x, 0.0f);
            glTexCoord2f(1.0f, 1.0f);
            glVertex3f(0.0f, (float) DK_resolution_x, (float) DK_resolution_y);
            glTexCoord2f(0.0f, 1.0f);
            glVertex3f(0.0f, 0.0f, (float) DK_resolution_y);
        }
        glEnd();

        glUseProgram(0);

        EXIT_ON_OPENGL_ERROR();

        // Do proper lighting pass.
        glUseProgram(gLightShader.program);

        // The the global transformation matrix.
        glUniformMatrix4fv(gLightShader.vs_uniforms.ModelViewProjectionMatrix, 1, GL_FALSE, DK_GetModelViewProjectionMatrix()->m);

        // Set camera position.
        glUniform3fv(gLightShader.fs_uniforms.CameraPosition, 1, DK_GetCameraPosition()->v);

        // Set our three g-buffer textures.
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, gBuffer.texture[0]);
        glUniform1i(gLightShader.fs_uniforms.GBuffer0, 0);

        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, gBuffer.texture[1]);
        glUniform1i(gLightShader.fs_uniforms.GBuffer1, 1);

        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, gBuffer.texture[2]);
        glUniform1i(gLightShader.fs_uniforms.GBuffer2, 2);

        EXIT_ON_OPENGL_ERROR();

        glEnable(GL_BLEND);
        glBlendFunc(GL_ONE, GL_ONE);

        // Draw lights.
        for (unsigned int i = 0; i < gLightCount; ++i) {
            const DK_Light* light = gLights[i];
            glUniform3fv(gLightShader.fs_uniforms.DiffuseLightColor, 1, light->diffuseColor.v);
            glUniform1f(gLightShader.fs_uniforms.DiffuseLightPower, light->diffusePower);
            glUniform3fv(gLightShader.fs_uniforms.SpecularLightColor, 1, light->specularColor.v);
            glUniform1f(gLightShader.fs_uniforms.SpecularLightPower, light->specularPower);
            glUniform3fv(gLightShader.fs_uniforms.LightPosition, 1, light->position.v);

            // Render the quad.
            glBegin(GL_QUADS);
            {
                glTexCoord2f(0.0f, 0.0f);
                glVertex3f(0.0f, 0.0f, 0.0f);
                glTexCoord2f(1.0f, 0.0f);
                glVertex3f(0.0f, (float) DK_resolution_x, 0.0f);
                glTexCoord2f(1.0f, 1.0f);
                glVertex3f(0.0f, (float) DK_resolution_x, (float) DK_resolution_y);
                glTexCoord2f(0.0f, 1.0f);
                glVertex3f(0.0f, 0.0f, (float) DK_resolution_y);
            }
            glEnd();

            EXIT_ON_OPENGL_ERROR();
        }

        glDisable(GL_BLEND);

        // Unbind the textures we used.
        glActiveTexture(GL_TEXTURE0);
        glDisable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, 0);

        glActiveTexture(GL_TEXTURE1);
        glDisable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, 0);
        glActiveTexture(GL_TEXTURE2);
        glDisable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, 0);
        glActiveTexture(GL_TEXTURE3);
        glDisable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, 0);
        glActiveTexture(GL_TEXTURE4);
        glDisable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, 0);

        // Done with our lighting shader.
        glUseProgram(0);

        EXIT_ON_OPENGL_ERROR();
    } else if (DK_d_draw_deferred == DK_D_DEPTH_BUFFER) {
        glBindFramebuffer(GL_READ_FRAMEBUFFER, gBuffer.frameBuffer);
        glBlitFramebuffer(0, 0, DK_resolution_x, DK_resolution_y,
                0, 0, DK_resolution_x, DK_resolution_y,
                GL_DEPTH_BUFFER_BIT, GL_NEAREST);
        glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);

        EXIT_ON_OPENGL_ERROR();
    } else {
        glActiveTexture(GL_TEXTURE0);
        glEnable(GL_TEXTURE_2D);
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

        EXIT_ON_OPENGL_ERROR();

        // Render the quad
        glBegin(GL_QUADS);
        {
            glTexCoord2f(0.0f, 0.0f);
            glVertex3f(0.0f, 0.0f, 0.0f);
            glTexCoord2f(1.0f, 0.0f);
            glVertex3f(0.0f, (float) DK_resolution_x, 0.0f);
            glTexCoord2f(1.0f, 1.0f);
            glVertex3f(0.0f, (float) DK_resolution_x, (float) DK_resolution_y);
            glTexCoord2f(0.0f, 1.0f);
            glVertex3f(0.0f, 0.0f, (float) DK_resolution_y);
        }
        glEnd();

        EXIT_ON_OPENGL_ERROR();

        // Unbind the textures we used.
        glActiveTexture(GL_TEXTURE0);
        glDisable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, 0);

        EXIT_ON_OPENGL_ERROR();
    }

    // Reset matrices.
    DK_PopModelMatrix();
    DK_EndLookAt();
    DK_EndOrthogonal();

    // Copy over the depth buffer from our frame buffer, to preserve the depths
    // in post-rendering (e.g. for selection outline).
    glBindFramebuffer(GL_READ_FRAMEBUFFER, gBuffer.frameBuffer);
    glBlitFramebuffer(0, 0, DK_resolution_x, DK_resolution_y,
            0, 0, DK_resolution_x, DK_resolution_y,
            GL_DEPTH_BUFFER_BIT, GL_NEAREST);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);

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

static const GLint textureUnits[4] = {0, 1, 2, 3};

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
        }

        glUniform1i(gGeometryShader.fs_uniforms.Textures, 0);
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
