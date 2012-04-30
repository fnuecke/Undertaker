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

    /** Create a shader program from the specified vertex and fragment shaders */
    GLuint DK_LoadProgram(const char* vsPath, const char* fsPath, const char** out_names, unsigned int out_count);

#ifdef	__cplusplus
}
#endif

#endif	/* SHADER_H */
