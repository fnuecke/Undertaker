/* 
 * File:   render.h
 * Author: fnuecke
 *
 * Created on April 23, 2012, 2:11 PM
 */

#ifndef RENDER_H
#define	RENDER_H

#include <GL/glew.h>

#include "callbacks.h"
#include "vmath.h"

/** Maximum number of textures on one material */
#define DK_MAX_MATERIAL_TEXTURES 4

#ifdef	__cplusplus
extern "C" {
#endif

    /** Used to configure material used for rendering */
    typedef struct {
        /** Textures used for multi-texturing */
        GLuint textures[DK_MAX_MATERIAL_TEXTURES];

        /** Number of textures to use */
        unsigned int texture_count;

        /** Texture to use as a bump map */
        GLuint bump_map;

        /** Texture to use as a normal map */
        GLuint normal_map;

        /** Diffuse multiplier for texture */
        vec4 diffuse_color;

        /** Specular multiplier for texture */
        vec3 specular_color;

        /** Emissivity of the color, i.e. how much light it provides to itself */
        float emissivity;
    } DK_Material;

    /** (Re)initialize openGL */
    void DK_InitRender(void);

    /** Render the game to the screen */
    void DK_Render(void);

    /** Set up the camera used for rendering, using two 3-component vectors */
    void DK_render_set_camera(const float* position, const float* target);

    /** Set material information to use from now on */
    void DK_SetMaterial(const DK_Material* material);

    /** Initialize a material to its default values */
    void DK_InitMaterial(DK_Material* material);

    /**
     * Initialize rendering system for events.
     */
    void DK_InitRender(void);

    /**
     * Register methods here that need to execute before rendering, but after
     * the view has been set up.
     * @param callback the method to call.
     */
    void DK_OnPreRender(callback method);

    /**
     * Register a method that should be called when an render pass is performed.
     * Methods are called in the order in which they are registered.
     * @param callback the method to call.
     */
    void DK_OnRender(callback method);

    /**
     * Register methods that need to render on top of the finished world render,
     * such as overlays.
     * @param callback the method to call.
     */
    void DK_OnPostRender(callback method);

#ifdef	__cplusplus
}
#endif

#endif	/* RENDER_H */

