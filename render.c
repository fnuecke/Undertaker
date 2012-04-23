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

/** Shader programs */
GLuint deferred_render = 0, deferred_shade = 0;

/** Our framebuffer for deferred shading */
GLuint frame_buffer = 0;

/** Renderbuffers for diffuse, position, normals and depth */
GLuint diffuse_buffer = 0, position_buffer = 0, normals_buffer = 0, depth_buffer = 0;

/** The OpenGL textures for the render targets */
GLuint diffuse_texture = 0, position_texture = 0, normals_texture = 0;

/** The ids for the uniforms in our shader program we bind our texture to */
GLuint diffuse_id = 0, position_id = 0, normals_id = 0, camera_id = 0, worldMatrix_id = 0, texture_id = 0;

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
    GLuint vs = DK_shader_load("data/shaders/deferred_render.vert", GL_VERTEX_SHADER);
    GLuint fs = DK_shader_load("data/shaders/deferred_render.frag", GL_FRAGMENT_SHADER);
    if (vs && fs) {
        deferred_render = DK_shader_program(vs, fs);

        if (deferred_render) {
            // Get the handles from the shader
            diffuse_id = glGetUniformLocation(deferred_render, "tDiffuse");
            position_id = glGetUniformLocation(deferred_render, "tPosition");
            normals_id = glGetUniformLocation(deferred_render, "tNormals");
            camera_id = glGetUniformLocation(deferred_render, "cameraPosition");
        }
    } else {
        deferred_render = 0;
    }
    vs = DK_shader_load("data/shaders/deferred_shade.vert", GL_VERTEX_SHADER);
    fs = DK_shader_load("data/shaders/deferred_shade.frag", GL_FRAGMENT_SHADER);
    if (vs && fs) {
        deferred_shade = DK_shader_program(vs, fs);

        if (deferred_shade) {
            // Get the handles from the shader
            worldMatrix_id = glGetUniformLocationARB(deferred_shade, "WorldMatrix");
            texture_id = glGetUniformLocationARB(deferred_shade, "tDiffuse");
        }
    } else {
        deferred_shade = 0;
    }

    // Generate and bind our frame buffer.
    glGenFramebuffers(1, &frame_buffer);
    glBindFramebuffer(GL_FRAMEBUFFER, frame_buffer);

    // Create our render buffers.
    diffuse_buffer = create_render_buffer(GL_RGBA, GL_COLOR_ATTACHMENT0);
    position_buffer = create_render_buffer(GL_RGBA32F, GL_COLOR_ATTACHMENT1);
    normals_buffer = create_render_buffer(GL_RGBA16F, GL_COLOR_ATTACHMENT2);
    depth_buffer = create_render_buffer(GL_DEPTH_COMPONENT24, GL_DEPTH_ATTACHMENT);

    // Create our textures.
    diffuse_texture = create_texture(GL_RGBA, GL_UNSIGNED_BYTE, GL_COLOR_ATTACHMENT0);
    position_texture = create_texture(GL_RGBA32F, GL_FLOAT, GL_COLOR_ATTACHMENT1);
    normals_texture = create_texture(GL_RGBA16F, GL_FLOAT, GL_COLOR_ATTACHMENT2);

    // Check if all worked fine and unbind the FBO
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        fprintf(DK_log_target, "ERROR: Can't initialize an FBO render texture. FBO initialization failed.");
        exit(EXIT_FAILURE);
    }

    // All done, unbind it.
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

static void init_lighting(void) {
    // Light colors.
    GLfloat darkness[] = {0.0f, 0.0f, 0.0f, 1.0f};
    GLfloat ambient_global[] = {0.1f, 0.1f, 0.1f, 1.0f};
    GLfloat ambient_directed[] = {0.1f, 0.06f, 0.06f, 1.0f};
    GLfloat hand_diffuse[] = {0.5f, 0.4f, 0.3f, 1.0f};

    // And directions.
    GLfloat ambient_direction[] = {-1.0f, 1.0f, 1.0f, 0.0f};

    // Set up global ambient lighting.
    glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, 1);
    glEnable(GL_LIGHTING);
    glShadeModel(GL_SMOOTH);
    glEnable(GL_COLOR_MATERIAL);

    // Ambient overall light.
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, ambient_global);

    //Add ambient directed light.
    glLightfv(GL_LIGHT0, GL_AMBIENT, darkness);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, ambient_directed);
    glLightfv(GL_LIGHT0, GL_AMBIENT, darkness);
    glLightfv(GL_LIGHT0, GL_POSITION, ambient_direction);
    glEnable(GL_LIGHT0);

    // Assign created components to GL_LIGHT0
    glLightfv(GL_LIGHT1, GL_AMBIENT, darkness);
    glLightfv(GL_LIGHT1, GL_DIFFUSE, hand_diffuse);
    glLightfv(GL_LIGHT1, GL_SPECULAR, darkness);
    glLightf(GL_LIGHT1, GL_CONSTANT_ATTENUATION, 0.0f);
    glLightf(GL_LIGHT1, GL_QUADRATIC_ATTENUATION, 0.00125f);
    glEnable(GL_LIGHT1);
}

///////////////////////////////////////////////////////////////////////////////
// Header implementation
///////////////////////////////////////////////////////////////////////////////

void DK_init_gl(void) {
    // Anti-alias?
    if (DK_use_antialiasing) {
        //glEnable(GL_MULTISAMPLE);
    }

    // We do use textures, so enable that.
    glEnable(GL_TEXTURE_2D);

    // We want triangle surface pixels to be shaded using interpolated values.
    glShadeModel(GL_SMOOTH);

    //init_lighting();

    glDisable(GL_LIGHTING);

    // Also enable depth testing to get stuff in the right order.
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    //glDepthMask(GL_TRUE);

    // We'll make sure stuff is rotated correctly, so we can cull back-faces.
    //glEnable(GL_CULL_FACE);
    //glFrontFace(GL_CCW);

    glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);

    init_shaders();

    /*
        if (DK_use_fog) {
            GLfloat fog_color[] = {0.0f, 0.0f, 0.0f, 1.0f};
            glFogi(GL_FOG_MODE, GL_LINEAR);
            glFogfv(GL_FOG_COLOR, fog_color);
            glFogf(GL_FOG_DENSITY, 0.1f);
            glHint(GL_FOG_HINT, GL_DONT_CARE);
            glFogf(GL_FOG_START, DK_CAMERA_HEIGHT + DK_BLOCK_SIZE);
            glFogf(GL_FOG_END, DK_CAMERA_HEIGHT + DK_BLOCK_SIZE * 4);
            glEnable(GL_FOG);
        }
     */

    // Load all textures we may need.
    DK_opengl_textures();

    // Define our viewport as the size of our window.
    glViewport(0, 0, DK_resolution_x, DK_resolution_y);

    // Set our projection matrix to match our viewport.
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(DK_field_of_view, DK_ASPECT_RATIO, 0.1, 1000.0);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

static void begin_render(void) {
    // Set "camera" position.
    const float* camera = DK_camera_position();
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    gluLookAt(
            // Eye position is camera position. Z is lowered by zooming.
            camera[0],
            camera[1],
            DK_CAMERA_HEIGHT - DK_camera_zoom() * DK_CAMERA_MAX_ZOOM,
            // Eye position is a little in front of camera position.
            camera[0],
            camera[1] + DK_CAMERA_TARGET_DISTANCE,
            0,
            // We use Z as the up vector.
            0, 0, 1);

    // Clear to black.
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClearDepth(1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glColor3f(1.0f, 1.0f, 1.0f);

    // Update cursor position (depends on view being set up).
    DK_update_cursor();

    // Get cursor position and update hand light accordingly.
    {
        float x, y;
        DK_cursor(&x, &y);
        {
            GLfloat light_position[] = {x, y, DK_BLOCK_HEIGHT * 2, 1};
            glLightfv(GL_LIGHT1, GL_POSITION, light_position);
        }
    }
}

static void end_render(void) {
    SDL_GL_SwapBuffers();
}

static const GLenum buffers[] = {
    GL_COLOR_ATTACHMENT0,
    GL_COLOR_ATTACHMENT1,
    GL_COLOR_ATTACHMENT2
};

static void begin_deferred(void) {
    // Bind our frame buffer and set the viewport to the proper size.
    glBindFramebuffer(GL_FRAMEBUFFER, frame_buffer);
    glPushAttrib(GL_VIEWPORT_BIT);
    glViewport(0, 0, DK_resolution_x, DK_resolution_y);

    // Clear the render targets
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glActiveTexture(GL_TEXTURE0);
    glEnable(GL_TEXTURE_2D);

    glDrawBuffers(3, buffers);

    // Save the current world matrix to compensate the normals in the shader
    {
        float worldMatrix[16];
        glGetFloatv(GL_MODELVIEW_MATRIX, worldMatrix);

        glUseProgram(deferred_shade);

        glUniformMatrix4fv(worldMatrix_id, 1, 0, worldMatrix);
    }
}

static void end_deferred(void) {
    glUseProgram(0);

    glPopAttrib();
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

static void render_deferred(void) {
    // Rendering the three buffers into a quad, so we want an orthogonal projection.
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0, DK_resolution_x, 0, DK_resolution_y, 0.1f, 2.0f);

    // Move one unit away from the plane.
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    glTranslatef(0, 0, -1.0);

    // Figure out what to draw.
    switch (DK_d_draw_deferred) {
        case DK_D_DEFERRED_DIFFUSE:
            glActiveTexture(GL_TEXTURE0);
            glEnable(GL_TEXTURE_2D);
            glBindTexture(GL_TEXTURE_2D, diffuse_texture);
            break;
        case DK_D_DEFERRED_POSITION:
            glActiveTexture(GL_TEXTURE0);
            glEnable(GL_TEXTURE_2D);
            glBindTexture(GL_TEXTURE_2D, position_texture);
            break;
        case DK_D_DEFERRED_NORMALS:
            glActiveTexture(GL_TEXTURE0);
            glEnable(GL_TEXTURE_2D);
            glBindTexture(GL_TEXTURE_2D, normals_texture);
            break;
        default:
        {
            // We use our shader to do the actual shading computations.
            glUseProgram(deferred_render);

            glActiveTexture(GL_TEXTURE0);
            glEnable(GL_TEXTURE_2D);
            glBindTexture(GL_TEXTURE_2D, diffuse_texture);
            glUniform1i(diffuse_id, 0);

            glActiveTexture(GL_TEXTURE1);
            glEnable(GL_TEXTURE_2D);
            glBindTexture(GL_TEXTURE_2D, position_texture);
            glUniform1i(position_id, 1);

            glActiveTexture(GL_TEXTURE2);
            glEnable(GL_TEXTURE_2D);
            glBindTexture(GL_TEXTURE_2D, normals_texture);
            glUniform1i(normals_id, 2);

            glUniform3fv(camera_id, 1, DK_camera_position());
            break;
        }
    }

    // Render the quad
    glColor3f(1.0f, 1.0f, 1.0f);

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
    switch (DK_d_draw_deferred) {
        case DK_D_DEFERRED_DIFFUSE:
            glActiveTexture(GL_TEXTURE0);
            glDisable(GL_TEXTURE_2D);
            glBindTexture(GL_TEXTURE_2D, 0);
            break;
        case DK_D_DEFERRED_POSITION:
            glActiveTexture(GL_TEXTURE0);
            glDisable(GL_TEXTURE_2D);
            glBindTexture(GL_TEXTURE_2D, 0);
            break;
        case DK_D_DEFERRED_NORMALS:
            glActiveTexture(GL_TEXTURE0);
            glDisable(GL_TEXTURE_2D);
            glBindTexture(GL_TEXTURE_2D, 0);
            break;
        default:
        {
            glActiveTexture(GL_TEXTURE0);
            glDisable(GL_TEXTURE_2D);
            glBindTexture(GL_TEXTURE_2D, 0);

            glActiveTexture(GL_TEXTURE1);
            glDisable(GL_TEXTURE_2D);
            glBindTexture(GL_TEXTURE_2D, 0);

            glActiveTexture(GL_TEXTURE2);
            glDisable(GL_TEXTURE_2D);
            glBindTexture(GL_TEXTURE_2D, 0);
            break;
        }
    }

    glUseProgram(0);

    // Reset to the matrices.
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
}

void DK_render(void) {
    begin_render();

    if (deferred_render && deferred_shade) {
        begin_deferred();
    }

    // Render game components.
    DK_render_map();
    DK_render_units();
    DK_render_jobs();

    if (deferred_render && deferred_shade) {
        end_deferred();
        render_deferred();
    }

    end_render();
}

void DK_set_texture(GLuint texture) {
    glBindTexture(GL_TEXTURE_2D, texture);
    glUniform1i(texture_id, 0);
}
