#include <stdio.h>


#include <stdlib.h>

#include <GL/glew.h>

#include "config.h"
#include "shader.h"

/** Print info log of a shader or program in case it failed to compile / link */
static int verify(GLuint obj, const char* path) {
    int log_size = 0;
    char log[2048];
    GLint status = GL_TRUE;

    if (glIsShader(obj)) {
        // Check if compilation succeeded.
        glGetShaderiv(obj, GL_COMPILE_STATUS, &status);
        if (status == GL_FALSE) {
            // Shader compilation failed, return.
            glGetShaderInfoLog(obj, sizeof (log), &log_size, log);
            glDeleteShader(obj);
        }
    } else {
        // Check if linking succeeded.
        glGetProgramiv(obj, GL_LINK_STATUS, &status);
        if (status == GL_FALSE) {
            // Linking failed, return.
            glGetProgramInfoLog(obj, sizeof (log), &log_size, log);
            glDeleteProgram(obj);
        }
    }

    // Print info if we have any.
    if (log_size > 0) {
        if (path) {
            fprintf(DK_log_target, "WARNING: Failed compiling shader '%s':\n%s\n", path, log);
        } else {
            fprintf(DK_log_target, "WARNING: Failed linking program:\n%s\n", log);
        }
    }

    // And return the compile / link status.
    return status;
}

GLuint DK_shader_load(const char* path, GLenum type) {
    FILE* file;
    long file_length, read_count;
    char* buffer;
    GLuint shader;

    // Try to open the file.
    if (!(file = fopen(path, "r"))) {
        fprintf(DK_log_target, "WARNING: Can't open shader '%s' for reading.\n", path);
        return 0;
    }

    // Figure out its length.
    fseek(file, 0, SEEK_END);
    file_length = ftell(file);
    fseek(file, 0, SEEK_SET);

    // Allocate enough memory to read the source.
    if (!(buffer = calloc(file_length, sizeof (char)))) {
        fprintf(DK_log_target, "ERROR: Could not allocate enough memory for shader '%s'.\n", path);
        fclose(file);
        return 0;
    }

    // Read the contents.
    read_count = fread(buffer, sizeof (char), file_length, file);

    // Done with the file.
    fclose(file);

    // Zero terminate it to make it autoparseable.
    buffer[read_count - 1] = '\0';

    // Create the shader, load the source and try to compile it.
    shader = glCreateShader(type);
    glShaderSource(shader, 1, &buffer, NULL);
    glCompileShader(shader);

    // Free memory used by source.
    free(buffer);

    // Check if compilation succeeded.
    if (verify(shader, path) == GL_FALSE) {
        return 0;
    }

    // Compilation successful, return the shader id.
    return shader;
}

GLuint DK_shader_program(GLuint vs, GLuint fs) {
    GLuint program = glCreateProgram();
    if (vs) {
        glAttachShader(program, vs);
    }
    if (fs) {
        glAttachShader(program, fs);
    }
    glLinkProgram(program);

    // Check if linking succeeded.
    if (verify(program, NULL) == GL_FALSE) {
        return 0;
    }

    // Linking successful, return the program id.
    return program;
}