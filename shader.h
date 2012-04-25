/* 
 * File:   shader.h
 * Author: fnuecke
 *
 * Created on April 23, 2012, 4:07 PM
 */

#ifndef SHADER_H
#define	SHADER_H

#include <GL/glew.h>

#ifdef	__cplusplus
extern "C" {
#endif

/** Load a shader's source into the specified buffer */
GLuint DK_shader_load(const char* path, GLenum type);

/** Create a shader program from the specified vertex and fragment shaders */
GLuint DK_shader_program(GLuint vs, GLuint fs, const char** out_names, unsigned int out_count);

#ifdef	__cplusplus
}
#endif

#endif	/* SHADER_H */
