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

#define EXIT_ON_OPENGL_ERROR()\
    { \
        GLenum error = glGetError(); \
        if (error != GL_FALSE) { \
            fprintf(DK_log_target, "ERROR: OpenGL broke (%d) here: %s:%d\n", error, __FILE__, __LINE__); \
            exit(EXIT_FAILURE); \
        } \
    }

/** Holds information on our GBuffer */
static struct {
    /** ID of the frame buffer we use for offscreen rendering */
    GLuint frameBuffer;

    /** ID of the diffuse color render buffer */
    GLuint diffuseAlbedoBuffer;
    /** ID of the specular color render buffer */
    GLuint specularAlbedoBuffer;
    /** ID of the emissive color render buffer */
    GLuint emissiveAlbedoBuffer;
    /** ID of the vertex position render buffer */
    GLuint vertexPositionBuffer;
    /** ID of the surface normal + specular exponent render buffer */
    GLuint surfaceNormalAndSpecularExponentBuffer;
    /** ID of the depth buffer */
    GLuint depthBuffer;

    /** ID of the diffuse color render buffer */
    GLuint diffuseAlbedoTexture;
    /** ID of the specular color render buffer */
    GLuint specularAlbedoTexture;
    /** ID of the emissive color render buffer */
    GLuint emissiveAlbedoTexture;
    /** ID of the vertex position render buffer */
    GLuint vertexPositionTexture;
    /** ID of the surface normal + specular exponent render buffer */
    GLuint surfaceNormalAndSpecularExponentTexture;
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
        /** Textures to use for rendering */
        GLint Textures;

        /** Number of textures to use */
        GLint TextureCount;

        /** The diffuse color multiplier */
        GLint ColorDiffuse;

        /** The specular color multiplier */
        GLint ColorSpecular;

        /** The specular exponent */
        GLint SpecularExponent;

        /** The emissive color */
        GLint ColorEmissive;
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
        /** The diffuse color */
        GLint DiffuseAlbedo;

        /** The specular color */
        GLint SpecularAlbedo;

        /** The emissive color */
        GLint EmissiveAlbedo;

        /** The vertex position */
        GLint VertexPosition;

        /** The surface normal + specular exponent */
        GLint SurfaceNormalAndSpecularExponent;

        /** The current position of the camera in the world */
        GLint WorldCameraPosition;

        /** The diffuse color of the current light to shade for */
        GLint LightDiffuseColor;

        /** The specular color of the current light to shade for */
        GLint LightSpecularColor;

        /** The position of the current light to shade for */
        GLint LightWorldPosition;
    } fs_uniforms;
} gLightShader;

/** Represents a single light in a scene */
typedef struct {
    /** The diffuse color (and power - can be larger than one). */
    float diffuseColor[3];

    /** The specular color (and power - can be larger than one). */
    float specularColor[3];

    /** The position of the light, in world space. */
    vec4 world_position;
} DK_Light;

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
    // Load shader for deferred shading.
    GLuint vs = DK_shader_load("data/shaders/geom.vert", GL_VERTEX_SHADER);
    GLuint fs = DK_shader_load("data/shaders/geom.frag", GL_FRAGMENT_SHADER);
    if (vs && fs) {
        const char* out_names[] = {"DiffuseAlbedo", "SpecularAlbedo", "EmissiveAlbedo", "VertexPosition", "SurfaceNormalAndSpecularExponent"};
        gGeometryShader.program = DK_shader_program(vs, fs, out_names, 5);

        if (gGeometryShader.program) {
            // Get the uniform/attribute locations from the shader.
            gGeometryShader.vs_uniforms.ModelMatrix = glGetUniformLocation(gGeometryShader.program, "ModelMatrix");
            gGeometryShader.vs_uniforms.ModelViewProjectionMatrix = glGetUniformLocation(gGeometryShader.program, "ModelViewProjectionMatrix");
            gGeometryShader.vs_uniforms.NormalMatrix = glGetUniformLocation(gGeometryShader.program, "NormalMatrix");

            gGeometryShader.vs_attributes.ModelVertex = glGetAttribLocation(gGeometryShader.program, "ModelVertex");
            gGeometryShader.vs_attributes.ModelNormal = glGetAttribLocation(gGeometryShader.program, "ModelNormal");
            gGeometryShader.vs_attributes.TextureCoordinate = glGetAttribLocation(gGeometryShader.program, "TextureCoordinate");

            gGeometryShader.fs_uniforms.Textures = glGetUniformLocation(gGeometryShader.program, "Textures");
            gGeometryShader.fs_uniforms.TextureCount = glGetUniformLocation(gGeometryShader.program, "TextureCount");
            gGeometryShader.fs_uniforms.ColorDiffuse = glGetUniformLocation(gGeometryShader.program, "ColorDiffuse");
            gGeometryShader.fs_uniforms.ColorSpecular = glGetUniformLocation(gGeometryShader.program, "ColorSpecular");
            gGeometryShader.fs_uniforms.SpecularExponent = glGetUniformLocation(gGeometryShader.program, "SpecularExponent");
            gGeometryShader.fs_uniforms.ColorEmissive = glGetUniformLocation(gGeometryShader.program, "ColorEmissive");
            EXIT_ON_OPENGL_ERROR();
        }
    } else {
        gGeometryShader.program = 0;
        return;
    }
    vs = DK_shader_load("data/shaders/light.vert", GL_VERTEX_SHADER);
    fs = DK_shader_load("data/shaders/light.frag", GL_FRAGMENT_SHADER);
    if (vs && fs) {
        const char* out_names[1] = {"Color"};
        gLightShader.program = DK_shader_program(vs, fs, out_names, 1);

        if (gLightShader.program) {
            // Get the uniform/attribute locations from the shader.
            gLightShader.vs_uniforms.ModelViewProjectionMatrix = glGetUniformLocation(gLightShader.program, "ModelViewProjectionMatrix");

            gLightShader.vs_attributes.ModelVertex = glGetAttribLocation(gLightShader.program, "ModelVertex");
            gLightShader.vs_attributes.TextureCoordinate = glGetAttribLocation(gLightShader.program, "TextureCoordinate");

            gLightShader.fs_uniforms.DiffuseAlbedo = glGetUniformLocation(gLightShader.program, "DiffuseAlbedo");
            gLightShader.fs_uniforms.SpecularAlbedo = glGetUniformLocation(gLightShader.program, "SpecularAlbedo");
            gLightShader.fs_uniforms.EmissiveAlbedo = glGetUniformLocation(gLightShader.program, "EmissiveAlbedo");
            gLightShader.fs_uniforms.VertexPosition = glGetUniformLocation(gLightShader.program, "VertexPosition");
            gLightShader.fs_uniforms.SurfaceNormalAndSpecularExponent = glGetUniformLocation(gLightShader.program, "SurfaceNormalAndSpecularExponent");
            gLightShader.fs_uniforms.WorldCameraPosition = glGetUniformLocation(gLightShader.program, "WorldCameraPosition");
            gLightShader.fs_uniforms.LightDiffuseColor = glGetUniformLocation(gLightShader.program, "LightDiffuseColor");
            gLightShader.fs_uniforms.LightSpecularColor = glGetUniformLocation(gLightShader.program, "LightSpecularColor");
            gLightShader.fs_uniforms.LightWorldPosition = glGetUniformLocation(gLightShader.program, "LightWorldPosition");
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
    gBuffer.diffuseAlbedoBuffer = createRenderBuffer(GL_RGB, GL_COLOR_ATTACHMENT0);
    gBuffer.specularAlbedoBuffer = createRenderBuffer(GL_RGB, GL_COLOR_ATTACHMENT1);
    gBuffer.emissiveAlbedoBuffer = createRenderBuffer(GL_RGB, GL_COLOR_ATTACHMENT2);
    gBuffer.vertexPositionBuffer = createRenderBuffer(GL_RGB32F, GL_COLOR_ATTACHMENT3);
    gBuffer.surfaceNormalAndSpecularExponentBuffer = createRenderBuffer(GL_RGBA16F, GL_COLOR_ATTACHMENT4);
    gBuffer.depthBuffer = createRenderBuffer(GL_DEPTH_COMPONENT24, GL_DEPTH_ATTACHMENT);

    // Create our textures.
    gBuffer.diffuseAlbedoTexture = createTexture(GL_RGB, GL_UNSIGNED_BYTE, GL_COLOR_ATTACHMENT0);
    gBuffer.specularAlbedoTexture = createTexture(GL_RGB, GL_UNSIGNED_BYTE, GL_COLOR_ATTACHMENT1);
    gBuffer.emissiveAlbedoTexture = createTexture(GL_RGB, GL_UNSIGNED_BYTE, GL_COLOR_ATTACHMENT2);
    gBuffer.vertexPositionTexture = createTexture(GL_RGB32F, GL_FLOAT, GL_COLOR_ATTACHMENT3);
    gBuffer.surfaceNormalAndSpecularExponentTexture = createTexture(GL_RGBA16F, GL_FLOAT, GL_COLOR_ATTACHMENT4);

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

static vec4 getCameraPosition(void) {
    const vec2* camera = DK_GetCameraPosition();
    vec4 position = {
        {
            camera->v[0],
            camera->v[1],
            DK_CAMERA_HEIGHT - DK_GetCameraZoom() * DK_CAMERA_MAX_ZOOM,
            1.0f
        }
    };
    return position;
}

static vec4 getCameraTarget(void) {
    const vec2* camera = DK_GetCameraPosition();
    vec4 target = {
        {
            camera->v[0],
            camera->v[1] + DK_CAMERA_TARGET_DISTANCE,
            0.0f,
            1.0f
        }
    };
    return target;
}

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
        GL_COLOR_ATTACHMENT2,
        GL_COLOR_ATTACHMENT3,
        GL_COLOR_ATTACHMENT4
    };

    // Bind our frame buffer.
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, gBuffer.frameBuffer);

    // Clear the render targets.
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Start using the geometry shader.
    glUseProgram(gGeometryShader.program);

    // Use our three buffers.
    glDrawBuffers(5, buffers);

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

    // Figure out what to draw.
    if (DK_d_draw_deferred == DK_D_DEFERRED_FINAL) {
        // Get camera position.
        vec4 cam_position = getCameraPosition();

        // We use our shader to do the actual shading computations.
        glUseProgram(gLightShader.program);

        // The the global transformation matrix.
        glUniformMatrix4fv(gLightShader.vs_uniforms.ModelViewProjectionMatrix, 1, GL_FALSE, DK_GetModelViewProjectionMatrix()->m);

        // Set camera position.
        glUniform3fv(gLightShader.fs_uniforms.WorldCameraPosition, 1, cam_position.v);

        // Set our three g-buffer textures.
        glActiveTexture(GL_TEXTURE0);
        glDisable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, gBuffer.diffuseAlbedoTexture);
        glUniform1i(gLightShader.fs_uniforms.DiffuseAlbedo, 0);

        glActiveTexture(GL_TEXTURE1);
        glDisable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, gBuffer.specularAlbedoTexture);
        glUniform1i(gLightShader.fs_uniforms.SpecularAlbedo, 1);

        glActiveTexture(GL_TEXTURE2);
        glDisable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, gBuffer.emissiveAlbedoTexture);
        glUniform1i(gLightShader.fs_uniforms.EmissiveAlbedo, 2);

        glActiveTexture(GL_TEXTURE3);
        glDisable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, gBuffer.vertexPositionTexture);
        glUniform1i(gLightShader.fs_uniforms.VertexPosition, 3);

        glActiveTexture(GL_TEXTURE4);
        glDisable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, gBuffer.surfaceNormalAndSpecularExponentTexture);
        glUniform1i(gLightShader.fs_uniforms.SurfaceNormalAndSpecularExponent, 4);

        glUniform3f(gLightShader.fs_uniforms.LightDiffuseColor, 1, 1, 1);
        glUniform3f(gLightShader.fs_uniforms.LightSpecularColor, 1, 1, 1);
        glUniform3f(gLightShader.fs_uniforms.LightWorldPosition, DK_GetCursor()->v[0], DK_GetCursor()->v[1], 80);
        
        EXIT_ON_OPENGL_ERROR();
    } else {
        glActiveTexture(GL_TEXTURE0);
        glEnable(GL_TEXTURE_2D);
        switch (DK_d_draw_deferred) {
            case DK_D_DEFERRED_DIFFUSE:
                glBindTexture(GL_TEXTURE_2D, gBuffer.diffuseAlbedoTexture);
                break;
            case DK_D_DEFERRED_POSITION:
                glBindTexture(GL_TEXTURE_2D, gBuffer.vertexPositionTexture);
                break;
            case DK_D_DEFERRED_NORMALS:
                glBindTexture(GL_TEXTURE_2D, gBuffer.surfaceNormalAndSpecularExponentTexture);
                break;
            default:
                break;
        }

        EXIT_ON_OPENGL_ERROR();
    }

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

    if (DK_d_draw_deferred == DK_D_DEFERRED_FINAL) {
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
    const vec4 cameraPosition = getCameraPosition();
    const vec4 cameraTarget = getCameraTarget();
    DK_BeginLookAt(cameraPosition.v[0], cameraPosition.v[1], cameraPosition.v[2],
            cameraTarget.v[0], cameraTarget.v[1], cameraTarget.v[2]);

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
    // ones, as we'll use our own because we use shaders.
    /*
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
     */

    DK_OnModelMatrixChanged(onModelMatrixChanged);
}

///////////////////////////////////////////////////////////////////////////////
// Material
///////////////////////////////////////////////////////////////////////////////

//static const GLint textureUnits[4] = {0, 1, 2, 3};

void DK_SetMaterial(const DK_Material* material) {
    for (unsigned int i = 0; i < DK_MAX_MATERIAL_TEXTURES; ++i) {
        glActiveTexture(GL_TEXTURE0 + i);
        glBindTexture(GL_TEXTURE_2D, 0);
    }
    EXIT_ON_OPENGL_ERROR();

    if (gIsGeometryPass) {
        glColor3f(1.0f, 1.0f, 1.0f);
        glDisable(GL_TEXTURE_2D);

        for (unsigned int i = 0; i < material->textureCount; ++i) {
            glActiveTexture(GL_TEXTURE0 + i);
            glBindTexture(GL_TEXTURE_2D, material->textures[i]);
        }

        glUniform1i(gGeometryShader.fs_uniforms.Textures, 0);
        glUniform1i(gGeometryShader.fs_uniforms.TextureCount, material->textureCount);

        glUniform3fv(gGeometryShader.fs_uniforms.ColorDiffuse, 1, material->diffuseColor.v);
        glUniform3fv(gGeometryShader.fs_uniforms.ColorSpecular, 1, material->specularColor.v);
        glUniform3fv(gGeometryShader.fs_uniforms.ColorEmissive, 1, material->emissiveColor.v);
        
        glUniform1f(gGeometryShader.fs_uniforms.SpecularExponent, material->specularExponent);
        EXIT_ON_OPENGL_ERROR();
    } else {
        glColor4f(material->diffuseColor.v[0],
                material->diffuseColor.v[1],
                material->diffuseColor.v[2],
                material->diffuseColor.v[3]);
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
    material->diffuseColor.v[0] = 0.0f;
    material->diffuseColor.v[1] = 0.0f;
    material->diffuseColor.v[2] = 0.0f;
    material->diffuseColor.v[3] = 0.0f;
    material->specularColor.v[0] = 0.0f;
    material->specularColor.v[1] = 0.0f;
    material->specularColor.v[2] = 0.0f;
    material->specularExponent = 0.0f;
    material->emissiveColor.v[0] = 0.0f;
    material->emissiveColor.v[1] = 0.0f;
    material->emissiveColor.v[2] = 0.0f;
    material->bumpMap = 0;
    material->normalMap = 0;
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
