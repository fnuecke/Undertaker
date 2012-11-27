/* 
 * Author: fnuecke
 *
 * Created on April 23, 2012, 2:11 PM
 */

#ifndef RENDER_H
#define	RENDER_H

#include <GL/glew.h>

#include "types.h"
#include "vmath.h"

/**
 * Maximum number of textures on one material.
 */
#define MP_MAX_MATERIAL_TEXTURES 4

#ifdef	__cplusplus
extern "C" {
#endif

    ///////////////////////////////////////////////////////////////////////////
    // Types
    ///////////////////////////////////////////////////////////////////////////

    /**
     * Describes a light in the world.
     */
    typedef struct {
        /** The position of the light, in world space */
        vec3 position;

        /** The diffuse color */
        vec3 diffuseColor;

        /** The diffuse lightpower */
        float diffuseRange;

        /** The specular color */
        vec3 specularColor;

        /** The specular light power */
        float specularRange;
    } MP_Light;

    /**
     * Used to configure material used for geometry pass of deferred shading.
     */
    typedef struct {
        /** Textures used for multi-texturing */
        GLuint textures[MP_MAX_MATERIAL_TEXTURES];

        /** Number of textures to use */
        unsigned int textureCount;

        /** Texture to use as a bump map */
        GLuint bumpMap;

        /** Texture to use as a normal map */
        GLuint normalMap;

        /** Diffuse multiplier for texture */
        vec4 diffuseColor;

        /** Specular multiplier for texture */
        float specularIntensity;

        /** Specular exponent for the material */
        float specularExponent;

        /** Emissivity of the color, i.e. how much light it provides to itself */
        float emissivity;
    } MP_Material;

    ///////////////////////////////////////////////////////////////////////////
    // Accessors
    ///////////////////////////////////////////////////////////////////////////

    /**
     * Get the attribute location in the deferred shader that's used for the
     * position of vertices.
     * @return the attribute location for the vertex position.
     */
    GLint MP_GetPositionAttributeLocation(void);

    /**
     * Get the attribute location in the deferred shader that's used for the
     * normal of vertices.
     * @return the attribute location for the vertex normal.
     */
    GLint MP_GetNormalAttributeLocation(void);

    /**
     * Get the attribute location in the deferred shader that's used for the
     * texture coordinate of vertices.
     * @return the attribute location for the vertex texture coordinate.
     */
    GLint MP_GetTextureCoordinateAttributeLocation(void);

    /**
     * Set material information to use from now on. Note that no reference will
     * be kept, all values will be copied into an internal buffer.
     * @param material the material to set.
     */
    void MP_SetMaterial(const MP_Material* material);

    /**
     * Initialize a material to its default values.
     * @param material the material to initialize.
     */
    void MP_InitMaterial(MP_Material* material);

    /**
     * Add a light to the world. Only the pointer will be tracked, so it is the
     * responsibility of the caller to make sure the light does not get invalid
     * before it is removed again.
     * @param light the light to add.
     */
    void MP_AddLight(const MP_Light* light);

    /**
     * Remove a light from the world.
     * @param light the light to remove.
     * @return whether the light was removed (1) or not (0).
     */
    bool MP_RemoveLight(const MP_Light* light);

    ///////////////////////////////////////////////////////////////////////////
    // Initialization / Rendering
    ///////////////////////////////////////////////////////////////////////////

    /**
     * Initialize rendering system for events.
     */
    void MP_InitRender(void);

    /**
     * Render the game to the screen.
     */
    void MP_Render(void);

    int MP_DEBUG_VisibleLightCount(void);

#ifdef	__cplusplus
}
#endif

#endif
