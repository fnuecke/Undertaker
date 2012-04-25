#include <assert.h>
#include <math.h>

#include <SDL/SDL.h>
#include <GL/glew.h>

#include "camera.h"
#include "config.h"
#include "cursor.h"
#include "jobs.h"
#include "map.h"
#include "render.h"
#include "shader.h"
#include "textures.h"
#include "units.h"
#include "vmath.h"

/** Shader programs */
//GLuint deferred_render = 0, deferred_shade = 0;

/** Our framebuffer for deferred shading */
//GLuint frame_buffer = 0;

/** Renderbuffers for diffuse, position, normals and depth */
//GLuint diffuse_buffer = 0, position_buffer = 0, normals_buffer = 0, depth_buffer = 0;

/** The OpenGL textures for the render targets */
//GLuint diffuse_texture = 0, position_texture = 0, normals_texture = 0;

/** The ids for the uniforms in our shader program we bind our texture to */
//GLuint diffuse_id = 0, position_id = 0, normals_id = 0, camera_id = 0, texture_id = 0;

/** Matrices we use for resolving vertex positions to screen positions */
static struct {
    /** The current projection transform */
    mat4 projection;

    /** The current view transform */
    mat4 view;

    /** Precomputed view-projection transform */
    mat4 vp;

    /** The current model transform */
    mat4 model;

    /** The current model-view-projection transform */
    mat4 mvp;

    /** Precomputed surface normal transform (transpose of inverse of current model transform) */
    mat3 normal;
} matrix;

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
        GLuint ModelVertex;

        /** The vertex normal in model space */
        GLuint ModelNormal;

        /** The texture coordinate at the vertex */
        GLuint TextureCoordinate;
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
    } fs_uniforms;
} geometry_shader;

static struct {
    /** ID of the shader program */
    GLuint program;

    /** Uniforms for the vertex shader */
    struct {
        /** The current model-view-projection matrix */
        GLuint ModelViewProjectionMatrix;
    } vs_uniforms;

    /** Attributes for the vertex shader */
    struct {
        /** The vertex position model space */
        GLuint ModelVertex;

        /** The texture coordinate at the vertex */
        GLuint TextureCoordinate;
    } vs_attributes;

    /** Uniforms for the fragment shader */
    struct {
        /** The color texture of our GBuffer */
        GLuint ColorBuffer;

        /** The position texture of our GBuffer */
        GLuint PositionBuffer;

        /** The surface normal texture of our GBuffer */
        GLuint NormalBuffer;

        /** The current position of the camera in the world */
        GLuint WorldCameraPosition;

        /** The diffuse color of the current light to shade for */
        GLuint LightDiffuseColor;

        /** The specular color of the current light to shade for */
        GLuint LightSpecularColor;

        /** The position of the current light to shade for */
        GLuint LightWorldPosition;
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

///////////////////////////////////////////////////////////////////////////////
// Helper methods
///////////////////////////////////////////////////////////////////////////////

static GLenum create_render_buffer(GLenum internalformat, GLenum attachment) {
    // Generate and bind the render buffer.
    GLuint render_buffer;
    glGenRenderbuffers(1, &render_buffer);
    glBindRenderbuffer(GL_RENDERBUFFER, render_buffer);

    // Set it up.
    glRenderbufferStorage(GL_RENDERBUFFER, internalformat, DK_resolution_x, DK_resolution_y);

    // Bind it to the frame buffer.
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, attachment, GL_RENDERBUFFER, render_buffer);

    // And return it.
    return render_buffer;
}

static GLenum create_texture(GLenum internalformat, GLenum type, GLenum attachment) {
    // Generate and bind the texture.
    GLenum texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);

    // Allocate it.
    glTexImage2D(GL_TEXTURE_2D, 0, internalformat, DK_resolution_x, DK_resolution_y, 0, GL_RGBA, type, NULL);

    // Set some more parameters.
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // Attach it to the frame buffer.
    glFramebufferTexture2D(GL_FRAMEBUFFER, attachment, GL_TEXTURE_2D, texture, 0);

    // And return it.
    return texture;
}

static void init_shaders(void) {
    // Load shader for deferred shading.
    GLuint vs = DK_shader_load("data/shaders/geom.vert", GL_VERTEX_SHADER);
    GLuint fs = DK_shader_load("data/shaders/geom.frag", GL_FRAGMENT_SHADER);
    if (vs && fs) {
        const char* out_names[3] = {"Color", "Vertex", "Normal"};
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
        }
    } else {
        light_shader.program = 0;
        return;
    }

    // Generate and bind our frame buffer.
    glGenFramebuffers(1, &g_buffer.frame_buffer);
    glBindFramebuffer(GL_FRAMEBUFFER, g_buffer.frame_buffer);

    // Create our render buffers.
    g_buffer.color_buffer = create_render_buffer(GL_RGB, GL_COLOR_ATTACHMENT0);
    g_buffer.position_buffer = create_render_buffer(GL_RGB32F, GL_COLOR_ATTACHMENT1);
    g_buffer.normal_buffer = create_render_buffer(GL_RGB16F, GL_COLOR_ATTACHMENT2);
    g_buffer.depth_buffer = create_render_buffer(GL_DEPTH_COMPONENT24, GL_DEPTH_ATTACHMENT);

    // Create our textures.
    g_buffer.color_texture = create_texture(GL_RGB, GL_UNSIGNED_BYTE, GL_COLOR_ATTACHMENT0);
    g_buffer.position_texture = create_texture(GL_RGB32F, GL_FLOAT, GL_COLOR_ATTACHMENT1);
    g_buffer.normal_texture = create_texture(GL_RGB16F, GL_FLOAT, GL_COLOR_ATTACHMENT2);

    // Check if all worked fine and unbind the FBO
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        fprintf(DK_log_target, "ERROR: Can't initialize an FBO render texture. FBO initialization failed.");
        exit(EXIT_FAILURE);
    }

    // All done, unbind frame buffer.
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

///////////////////////////////////////////////////////////////////////////////
// Header implementation
///////////////////////////////////////////////////////////////////////////////

void DK_init_gl(void) {
    // We do use textures, so enable that.
    glEnable(GL_TEXTURE_2D);

    // We want triangle surface pixels to be shaded using interpolated values.
    glShadeModel(GL_SMOOTH);

    // We do manual lighting via deferred shading.
    glDisable(GL_LIGHTING);

    // Also enable depth testing to get stuff in the right order.
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glDepthMask(GL_TRUE);

    // We'll make sure stuff is rotated correctly, so we can cull back-faces.
    glEnable(GL_CULL_FACE);
    glFrontFace(GL_CCW);

    glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);

    // Initialize our deferred shaders.
    init_shaders();

    // Load all textures we may need.
    init_textures();

    // Define our viewport as the size of our window.
    glViewport(0, 0, DK_resolution_x, DK_resolution_y);

    // Set our projection matrix to match our viewport. Null the OpenGL internal
    // ones, as we'll use our own because we use shaders.
    /*
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
     */
}

static void update_matrices(void) {
    // Precompute view-projection transform.
    mmulm(&matrix.vp, &matrix.projection, &matrix.view);

    // Precompute model-view-projection transform.
    mmulm(&matrix.mvp, &matrix.vp, &matrix.model);
}

static void camera_position(vec4* position) {
    const vec2* camera = DK_GetCameraPosition();
    position->v[0] = camera->v[0];
    position->v[1] = camera->v[1];
    position->v[2] = DK_CAMERA_HEIGHT - DK_GetCameraZoom() * DK_CAMERA_MAX_ZOOM;
    position->v[3] = 1.0f;
}

static void camera_target(vec4* target) {
    const vec2* camera = DK_GetCameraPosition();
    target->v[0] = camera->v[0];
    target->v[1] = camera->v[1] + DK_CAMERA_TARGET_DISTANCE;
    target->v[2] = 0.0f;
    target->v[3] = 1.0f;
}

static void begin_render(void) {
    // Set camera position.
    vec4 cam_position, cam_target;
    camera_position(&cam_position);
    camera_target(&cam_target);
    lookat(&matrix.view, &cam_position, &cam_target);

    // Set projection matrix.
    perspective(&matrix.projection, DK_field_of_view, DK_ASPECT_RATIO, 0.1f, 1000.0f);

    // Reset model transform.
    matrix.model = IDENTITY_MATRIX4;
    matrix.normal = IDENTITY_MATRIX3;

    // Update precomputed matrics.
    update_matrices();

    // Clear to black and set default vertex color to white.
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClearDepth(1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

    // Update cursor position (depends on view being set up).
    //DK_update_cursor();

    // Get cursor position and update hand light accordingly.
    /*
        {
            float x, y;
            DK_cursor(&x, &y);
            {
                GLfloat light_position[] = {x, y, DK_BLOCK_HEIGHT * 2, 1};
                glLightfv(GL_LIGHT1, GL_POSITION, light_position);
            }
        }
     */
}

static const GLenum buffers[] = {
    GL_COLOR_ATTACHMENT0,
    GL_COLOR_ATTACHMENT1,
    GL_COLOR_ATTACHMENT2
};

static void begin_deferred(void) {
    // Bind our frame buffer.
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, g_buffer.frame_buffer);

    // Clear the render targets.
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Start using the geometry shader.
    glUseProgram(geometry_shader.program);

    // Set uniforms for geometry shader.
    glUniformMatrix4fv(geometry_shader.vs_uniforms.ModelMatrix, 1, GL_FALSE, matrix.model.m);
    glUniformMatrix4fv(geometry_shader.vs_uniforms.ModelViewProjectionMatrix, 1, GL_FALSE, matrix.mvp.m);
    glUniformMatrix4fv(geometry_shader.vs_uniforms.NormalMatrix, 1, GL_FALSE, matrix.normal.m);

    //glActiveTexture(GL_TEXTURE0);
    //glEnable(GL_TEXTURE_2D);

    // Use our three buffers.
    glDrawBuffers(3, buffers);
}

static void end_deferred(void) {
    glUseProgram(0);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
}

static void render_deferred(void) {
    // Figure out what to draw.
    if (DK_d_draw_deferred == DK_D_DEFERRED_FINAL) {
        vec4 cam_position;

        // Set projection matrix.
        orthogonal(&matrix.projection, 0, DK_resolution_x, 0, DK_resolution_y, 0.1f, 2.0f);

        // Reset other matrices, set view a bit back to avoid clipping.
        matrix.model = IDENTITY_MATRIX4;
        matrix.normal = IDENTITY_MATRIX3;
        matrix.view = IDENTITY_MATRIX4;
        mitranslate(&matrix.view, 0, 0, -1.0f);

        // Update precomputed matrices.
        update_matrices();

        // We use our shader to do the actual shading computations.
        glUseProgram(light_shader.program);

        // The the global transformation matrix.
        glUniformMatrix4fv(geometry_shader.vs_uniforms.ModelViewProjectionMatrix, 1, GL_FALSE, matrix.mvp.m);

        // Set camera position.
        camera_position(&cam_position);
        glUniform3fv(light_shader.fs_uniforms.WorldCameraPosition, 1, cam_position.v);

        // Set our three g-buffer textures.
        glActiveTexture(GL_TEXTURE0);
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, g_buffer.color_texture);
        glUniform1i(light_shader.fs_uniforms.ColorBuffer, 0);

        glActiveTexture(GL_TEXTURE1);
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, g_buffer.position_texture);
        glUniform1i(light_shader.fs_uniforms.PositionBuffer, 1);

        glActiveTexture(GL_TEXTURE2);
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, g_buffer.normal_texture);
        glUniform1i(light_shader.fs_uniforms.NormalBuffer, 2);
    } else {
        glMatrixMode(GL_PROJECTION);
        glPushMatrix();
        glLoadIdentity();
        glOrtho(0, DK_resolution_x, 0, DK_resolution_y, 0.1f, 2.0f);

        glMatrixMode(GL_MODELVIEW);
        glPushMatrix();
        glLoadIdentity();
        glTranslatef(0, 0, -1.0f);

        glDisable(GL_LIGHTING);
        glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

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
    }

    // Render the quad
    glBegin(GL_QUADS);
    {
        glTexCoord2f(0.0f, 0.0f);
        glVertex3f(0.0f, 0.0f, 0.0f);
        glTexCoord2f(1.0f, 0.0f);
        glVertex3f((float) DK_resolution_x, 0.0f, 0.0f);
        glTexCoord2f(1.0f, 1.0f);
        glVertex3f((float) DK_resolution_x, (float) DK_resolution_y, 0.0f);
        glTexCoord2f(0.0f, 1.0f);
        glVertex3f(0.0f, (float) DK_resolution_y, 0.0f);
    }
    glEnd();

    // Reset OpenGL state.
    glActiveTexture(GL_TEXTURE0);
    glDisable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, 0);
    glActiveTexture(GL_TEXTURE1);
    glDisable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, 0);
    glActiveTexture(GL_TEXTURE2);
    glDisable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, 0);

    if (DK_d_draw_deferred == DK_D_DEFERRED_FINAL) {
        // Done with our lighting shader.
        glUseProgram(0);
    } else {
        // Reset to the matrices.
        glMatrixMode(GL_PROJECTION);
        glPopMatrix();
        glMatrixMode(GL_MODELVIEW);
        glPopMatrix();
    }
}

void DK_render(void) {
    begin_render();

    if (geometry_shader.program && light_shader.program) {
        begin_deferred();
    }

    // Render game components.
    DK_render_map();
    DK_RenderUnits();
    DK_RenderJobs();

    if (geometry_shader.program && light_shader.program) {
        end_deferred();
        render_deferred();
    }
}

//static const GLint textureUnits[4] = {0, 1, 2, 3};

void DK_render_set_material(const DK_Material* material) {
    for (unsigned int i = 0; i < sizeof (material->textures); ++i) {
        glActiveTexture(GL_TEXTURE0 + i);
        glBindTexture(GL_TEXTURE_2D, 0);
    }
    for (unsigned int i = 0; i < material->texture_count; ++i) {
        glActiveTexture(GL_TEXTURE0 + i);
        glBindTexture(GL_TEXTURE_2D, material->textures[i]);
    }
    glUniform1i(geometry_shader.fs_uniforms.Textures, 0);
    glUniform1i(geometry_shader.fs_uniforms.TextureCount, material->texture_count);
}

void DK_material_init(DK_Material* material) {
    for (unsigned int i = 0; i < sizeof (material->textures); ++i) {
        material->textures[i] = 0;
    }
    material->texture_count = 0;
    material->diffuse_color.v[0] = 1.0f;
    material->diffuse_color.v[1] = 1.0f;
    material->diffuse_color.v[2] = 1.0f;
    material->specular_color.v[0] = 1.0f;
    material->specular_color.v[1] = 1.0f;
    material->specular_color.v[2] = 1.0f;
    material->bump_map = 0;
    material->normal_map = 0;
}