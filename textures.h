/* 
 * File:   textures.h
 * Author: fnuecke
 *
 * Created on April 14, 2012, 9:48 AM
 */

#ifndef TEXTURES_H
#define	TEXTURES_H

#include <GL/glew.h>

#include "types.h"

#ifdef	__cplusplus
extern "C" {
#endif

    /**
     * Get a specific texture. There may be a number of variations per texture,
     * where the actual returned variation is determined by the given hash.
     * @param textureId the id of the texture to get.
     * @param hash a value that will determine the variation of the texture.
     */
    GLuint MP_GetTexture(MP_TextureID textureId, unsigned int hash);

    /**
     * Tries to load a texture into memory. The specified base name will be
     * suffixed with a variation counter (starting at zero) to allow for varying
     * textures per texture type.
     * @param basename the base texture name.
     * @return the id of the loaded texture (for GetTexture), or 0 on failure.
     */
    MP_TextureID MP_LoadTexture(const char* basename);

    /**
     * Unloads all textures from memory. This will also delete them from the GPU
     * (i.e. calls MP_DeleteTextures).
     */
    void MP_UnloadTextures(void);

    /**
     * Generate textures for OpenGL, i.e. sends the textures to the GPU.
     */
    void MP_GL_GenerateTextures(void);

    /**
     * Deletes textures in OpenGL, i.e. frees the memory on the GPU.
     */
    void MP_GL_DeleteTextures(void);

#ifdef	__cplusplus
}
#endif

#endif	/* TEXTURES_H */

