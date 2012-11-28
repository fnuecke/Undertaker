/* 
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

    /**
     * Create a shader program from the vertex and fragment shaders that will be
     * loaded from the specified files.
     * @param vsPath the path to the vertex shader.
     * @param fsPath the path to the fragment shader.
     * @param outNames a list of names of out attributes of the fragment shader.
     * @param outCount the number of out attributes names given in outNames.
     * @return the loaded shader, or 0 if loading failed.
     */
    GLuint MP_LoadProgram(const char* vsPath, const char* fsPath, const char** outNames, unsigned int outCount);

#ifdef	__cplusplus
}
#endif

#endif
