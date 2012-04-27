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
    GLuint frame_buffer;

    /** ID of the color render buffer */
    GLuint color_buffer;
    /** ID of the position render buffer */
    GLuint position_buffer;
    /** ID of the surface normal render buffer */
    GLuint normal_buffer;
    /** ID of the depth buffer */
    GLuint depth_buffer;

    /** ID of the color texture (actual data) */
    GLuint color_texture;
    /** ID of the position texture (actual data) */
    GLuint position_texture;
    /** ID of the surface normal texture (actual data) */
    GLuint normal_texture;
} g_buffer;

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

        /** The emissivity of the color */
        GLint ColorEmissivity;
    } fs_uniforms;
} geometry_shader;

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
        /** The color texture of our GBuffer */
        GLint ColorBuffer;

        /** The position texture of our GBuffer */
        GLint PositionBuffer;

        /** The surface normal texture of our GBuffer */
        GLint NormalBuffer;

        /** The current position of the camera in the world */
        GLint WorldCameraPosition;

        /** The diffuse color of the current light to shade for */
        GLint LightDiffuseColor;

        /** The specular color of the current light to shade for */
        GLint LightSpecularColor;

        /** The position of the current light to shade for */
        GLint LightWorldPosition;
    } fs_uniforms;
} light_shader;

/** Represents a single light in a scene */
typedef struct {
    /** The diffuse color (and power - can be larger than one). */
    float diffuse_color[3];

    /** The specular color (and power - can be larger than one). */
    float specular_color[3];

    /** The position of the light, in world space. */
    vec4 world_position;
} DK_Light;

/** Callbacks/hooks for different render stages */
static Callbacks* gPreRenderCallbacks = 0;
static Callbacks* gRenderCallbacks = 0;
static Callbacks* gPostRenderCallbacks = 0;

/** Flag if we're currently in the geometry rendering stage */
static int gGeometryPass = 0;

///////////////////////////////////////////////////////////////////////////////
// Shader and GBuffer setup
///////////////////////////////////////////////////////////////////////////////

static void initShaders(void) {
    // Load shader for deferred shading.
    GLuint vs = DK_shader_load("data/shaders/geom.vert", GL_VERTEX_SHADER);
    GLuint fs = DK_shader_load("data/shaders/geom.frag", GL_FRAGMENT_SHADER);
    if (vs && fs) {
        const char* out_names[3] = {"Vertex", "Normal", "Color"};
        geometry_shader.program = DK_shader_program(vs, fs, out_names, 3);

        if (geometry_shader.program) {
            // Get the uniform/attribute locations from the shader.
            geometry_shader.vs_uniforms.ModelMatrix = glGetUniformLocation(geometry_shader.program, "ModelMatrix");
            geometry_shader.vs_uniforms.ModelViewProjectionMatrix = glGetUniformLocation(geometry_shader.program, "ModelViewProjectionMatrix");
            geometry_shader.vs_uniforms.NormalMatrix = glGetUniformLocation(geometry_shader.program, "NormalMatrix");
            geometry_shader.vs_attributes.ModelVertex = glGetAttribLocation(geometry_shader.program, "ModelVertex");
            geometry_shader.vs_attributes.ModelNormal = glGetAttribLocation(geometry_shader.program, "ModelNormal");
            geometry_shader.vs_attributes.TextureCoordinate = glGetAttribLocation(geometry_shader.program, "TextureCoordinate");
            geometry_shader.fs_uniforms.Textures = glGetUniformLocation(geometry_shader.program, "Textures");
            geometry_shader.fs_uniforms.TextureCount = glGetUniformLocation(geometry_shader.program, "TextureCount");
            geometry_shader.fs_uniforms.ColorDiffuse = glGetUniformLocation(geometry_shader.program, "ColorDiffuse");
            geometry_shader.fs_uniforms.ColorSpecular = glGetUniformLocation(geometry_shader.program, "ColorSpecular");
            geometry_shader.fs_uniforms.ColorEmissivity = glGetUniformLocation(geometry_shader.program, "ColorEmissivity");
            EXIT_ON_OPENGL_ERROR();
        }
    } else {
        geometry_shader.program = 0;
        return;
    }
    vs = DK_shader_load("data/shaders/light.vert", GL_VERTEX_SHADER);
    fs = DK_shader_load("data/shaders/light.frag", GL_FRAGMENT_SHADER);
    if (vs && fs) {
        const char* out_names[1] = {"Color"};
        light_shader.program = DK_shader_program(vs, fs, out_names, 1);

        if (light_shader.program) {
            // Get the uniform/attribute locations from the shader.
            light_shader.vs_uniforms.ModelViewProjectionMatrix = glGetUniformLocation(light_shader.program, "ModelViewProjectionMatrix");
            light_shader.vs_attributes.ModelVertex = glGetAttribLocation(light_shader.program, "ModelVertex");
            light_shader.vs_attributes.TextureCoordinate = glGetAttribLocation(light_shader.program, "TextureCoordinate");
            light_shader.fs_uniforms.ColorBuffer = glGetUniformLocation(light_shader.program, "ColorBuffer");
            light_shader.fs_uniforms.PositionBuffer = glGetUniformLocation(light_shader.program, "PositionBuffer");
            light_shader.fs_uniforms.NormalBuffer = glGetUniformLocation(light_shader.program, "NormalBuffer");
            light_shader.fs_uniforms.WorldCameraPosition = glGetUniformLocation(light_shader.program, "WorldCameraPosition");
            light_shader.fs_uniforms.LightDiffuseColor = glGetUniformLocation(light_shader.program, "LightDiffuseColor");
            light_shader.fs_uniforms.LightSpecularColor = glGetUniformLocation(light_shader.program, "LightSpecularColor");
            light_shader.fs_uniforms.LightWorldPosition = glGetUniformLocation(light_shader.program, "LightWorldPosition");
            EXIT_ON_OPENGL_ERROR();
        }
    } else {
        light_shader.program = 0;
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
    glGenFramebuffers(1, &g_buffer.frame_buffer);
    EXIT_ON_OPENGL_ERROR();

    glBindFramebuffer(GL_FRAMEBUFFER, g_buffer.frame_buffer);
    EXIT_ON_OPENGL_ERROR();

    // Create our render buffers.
    g_buffer.position_buffer = createRenderBuffer(GL_RGBA32F, GL_COLOR_ATTACHMENT0);
    g_buffer.normal_buffer = createRenderBuffer(GL_RGB16F, GL_COLOR_ATTACHMENT1);
    g_buffer.color_buffer = createRenderBuffer(GL_RGBA, GL_COLOR_ATTACHMENT2);
    g_buffer.depth_buffer = createRenderBuffer(GL_DEPTH_COMPONENT24, GL_DEPTH_ATTACHMENT);

    // Create our textures.
    g_buffer.position_texture = createTexture(GL_RGBA32F, GL_FLOAT, GL_COLOR_ATTACHMENT0);
    g_buffer.normal_texture = createTexture(GL_RGB16F, GL_FLOAT, GL_COLOR_ATTACHMENT1);
    g_buffer.color_texture = createTexture(GL_RGBA, GL_UNSIGNED_BYTE, GL_COLOR_ATTACHMENT2);

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
    if (gGeometryPass) {
        // Set uniforms for geometry shader.
        glUniformMatrix4fv(geometry_shader.vs_uniforms.ModelMatrix, 1, GL_FALSE, DK_GetModelMatrix()->m);
        glUniformMatrix4fv(geometry_shader.vs_uniforms.ModelViewProjectionMatrix, 1, GL_FALSE, DK_GetModelViewProjectionMatrix()->m);
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
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, g_buffer.frame_buffer);

    // Clear the render targets.
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Start using the geometry shader.
    glUseProgram(geometry_shader.program);

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

    // Figure out what to draw.
    if (DK_d_draw_deferred == DK_D_DEFERRED_FINAL) {
        // Get camera position.
        vec4 cam_position = getCameraPosition();

        // We use our shader to do the actual shading computations.
        glUseProgram(light_shader.program);

        // The the global transformation matrix.
        glUniformMatrix4fv(light_shader.vs_uniforms.ModelViewProjectionMatrix, 1, GL_FALSE, DK_GetModelViewProjectionMatrix()->m);

        // Set camera position.
        glUniform3fv(light_shader.fs_uniforms.WorldCameraPosition, 1, cam_position.v);

        // Set our three g-buffer textures.
        glActiveTexture(GL_TEXTURE0);
        glDisable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, g_buffer.color_texture);
        glUniform1i(light_shader.fs_uniforms.ColorBuffer, 0);

        glActiveTexture(GL_TEXTURE1);
        glDisable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, g_buffer.position_texture);
        glUniform1i(light_shader.fs_uniforms.PositionBuffer, 1);

        glActiveTexture(GL_TEXTURE2);
        glDisable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, g_buffer.normal_texture);
        glUniform1i(light_shader.fs_uniforms.NormalBuffer, 2);

        EXIT_ON_OPENGL_ERROR();
    } else {
        glActiveTexture(GL_TEXTURE0);
        glEnable(GL_TEXTURE_2D);
        switch (DK_d_draw_deferred) {
            case DK_D_DEFERRED_DIFFUSE:
                glBindTexture(GL_TEXTURE_2D, g_buffer.color_texture);
                break;
            case DK_D_DEFERRED_POSITION:
                glBindTexture(GL_TEXTURE_2D, g_buffer.position_texture);
                break;
            case DK_D_DEFERRED_NORMALS:
                glBindTexture(GL_TEXTURE_2D, g_buffer.normal_texture);
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
    glBindFramebuffer(GL_READ_FRAMEBUFFER, g_buffer.frame_buffer);
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

    if (DK_d_draw_deferred_shader && geometry_shader.program && light_shader.program) {
        geometryPass();
        gGeometryPass = 1;
    }

    // Render game components.
    CB_Call(gRenderCallbacks);

    if (DK_d_draw_deferred_shader && geometry_shader.program && light_shader.program) {
        gGeometryPass = 0;
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

    if (gGeometryPass) {
        glColor3f(1.0f, 1.0f, 1.0f);
        glDisable(GL_TEXTURE_2D);

        for (unsigned int i = 0; i < material->texture_count; ++i) {
            glActiveTexture(GL_TEXTURE0 + i);
            glBindTexture(GL_TEXTURE_2D, material->textures[i]);
        }

        glUniform1i(geometry_shader.fs_uniforms.Textures, 0);
        glUniform1i(geometry_shader.fs_uniforms.TextureCount, material->texture_count);

        glUniform3fv(geometry_shader.fs_uniforms.ColorDiffuse, 1, material->diffuse_color.v);
        glUniform3fv(geometry_shader.fs_uniforms.ColorSpecular, 1, material->specular_color.v);

        glUniform1f(geometry_shader.fs_uniforms.ColorEmissivity, material->emissivity);
        EXIT_ON_OPENGL_ERROR();
    } else {
        glColor4f(material->diffuse_color.v[0],
                material->diffuse_color.v[1],
                material->diffuse_color.v[2],
                material->diffuse_color.v[3]);
        if (material->texture_count > 0) {
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
    material->texture_count = 0;
    material->diffuse_color.v[0] = 1.0f;
    material->diffuse_color.v[1] = 1.0f;
    material->diffuse_color.v[2] = 1.0f;
    material->diffuse_color.v[3] = 1.0f;
    material->specular_color.v[0] = 1.0f;
    material->specular_color.v[1] = 1.0f;
    material->specular_color.v[2] = 1.0f;
    material->emissivity = 0;
    material->bump_map = 0;
    material->normal_map = 0;
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
