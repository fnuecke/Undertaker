/* 
 * File:   render.h
 * Author: fnuecke
 *
 * Created on April 23, 2012, 2:11 PM
 */

#ifndef RENDER_H
#define	RENDER_H

#include "callbacks.h"
#include "vmath.h"

#ifdef	__cplusplus
extern "C" {
#endif

    /** Used to configure material used for rendering */
    typedef struct {
        /** Textures used for multi-texturing */
        GLuint textures[4];

        /** Number of textures to use */
        unsigned int texture_count;

        /** Texture to use as a bump map */
        GLuint bump_map;

        /** Texture to use as a normal map */
        GLuint normal_map;

        /** Diffuse multiplier for texture */
        vec3 diffuse_color;

        /** Specular multiplier for texture */
        vec3 specular_color;
    } DK_Material;

    /** (Re)initialize openGL */
    void DK_init_gl(void);

    /** Render the game to the screen */
    void DK_render(void);

    /** Set up the camera used for rendering, using two 3-component vectors */
    void DK_render_set_camera(const float* position, const float* target);

    /** Set material information to use from now on */
    void DK_render_set_material(const DK_Material* material);

    /** Initialize a material to its default values */
    void DK_material_init(DK_Material* material);

    /**
     * Register a method that should be called when an render pass is performed.
     * Methods are called in the order in which they are registered.
     * @param callback the method to call.
     */
    void DK_OnRender(callback method);

#ifdef	__cplusplus
}
#endif

#endif	/* RENDER_H */

