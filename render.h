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

    ///////////////////////////////////////////////////////////////////////////
    // Types
    ///////////////////////////////////////////////////////////////////////////

    /** Describes a light in the world */
    typedef struct {
        /** The position of the light, in world space */
        vec3 position;

        /** The diffuse color */
        vec3 diffuseColor;

        /** The diffuse lightpower */
        float diffusePower;

        /** The specular color */
        vec3 specularColor;

        /** The specular light power */
        float specularPower;
    } DK_Light;

    /** Used to configure material used for rendering */
    typedef struct {
        /** Textures used for multi-texturing */
        GLuint textures[DK_MAX_MATERIAL_TEXTURES];

        /** Number of textures to use */
        unsigned int textureCount;

        /** Texture to use as a bump map */
        GLuint bumpMap;

        /** Texture to use as a normal map */
        GLuint normalMap;

        /** Diffuse multiplier for texture */
        vec4 diffuseColor;

        /** Specular multiplier for texture */
        vec3 specularColor;

        /** Emissivity of the color, i.e. how much light it provides to itself */
        vec3 emissiveColor;

        /** Specular exponent for the material */
        float specularExponent;
    } DK_Material;

    ///////////////////////////////////////////////////////////////////////////
    // Accessors
    ///////////////////////////////////////////////////////////////////////////

    /**
     * Set material information to use from now on. Note that no reference will
     * be kept, all values will be copied into an internal buffer.
     * @param material the material to set.
     */
    void DK_SetMaterial(const DK_Material* material);

    /**
     * Initialize a material to its default values.
     * @param material the material to initialize.
     */
    void DK_InitMaterial(DK_Material* material);

    /**
     * Add a light to the world. Only the pointer will be tracked, so it is the
     * responsibility of the caller to make sure the light does not get invalid
     * before it is removed again.
     * @param light the light to add.
     */
    void DK_AddLight(const DK_Light* light);

    /**
     * Remove a light from the world.
     * @param light the light to remove.
     * @return whether the light was removed (1) or not (0).
     */
    int DK_RemoveLight(const DK_Light* light);

    ///////////////////////////////////////////////////////////////////////////
    // Initialization / Rendering
    ///////////////////////////////////////////////////////////////////////////

    /**
     * Initialize rendering system for events.
     */
    void DK_InitRender(void);

    /**
     * Render the game to the screen.
     */
    void DK_Render(void);

    ///////////////////////////////////////////////////////////////////////////
    // Events
    ///////////////////////////////////////////////////////////////////////////

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

