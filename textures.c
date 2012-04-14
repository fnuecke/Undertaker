#include <stdio.h>
#include <SDL.h>
#include <SDL_image.h>
#include <GL/gl.h>
#include <GL/glu.h>

#include "config.h"
#include "textures.h"

/** File name template for textures; must be same order as DK_Texture enum */
const char* DK_texture_names[DK_TEX_COUNT] = {
    "dirt_floor",
    "dirt_side",
    "dirt_top",
    "fluid_lava",
    "fluid_side",
    "fluid_water",
    "owner_blue",
    "owner_green",
    "owner_red",
    "owner_white",
    "owner_yellow",
    "rock_side",
    "rock_top",
    "wall_top_n",
    "wall_top_ne",
    "wall_top_ne_corner",
    "wall_top_nes",
    "wall_top_nesw",
    "wall_top_ns"
};

/** Number of variants for textures */
int DK_num_textures[DK_TEX_COUNT];

SDL_Surface* DK_test_texture;
GLuint DK_gl_test_texture;

/** Actual, loaded textures, as surface and openGL texture */
SDL_Surface* DK_textures[DK_TEX_COUNT][DK_TEX_MAX_VARIATIONS];
GLuint DK_gl_textures[DK_TEX_COUNT][DK_TEX_MAX_VARIATIONS];

GLuint DK_opengl_texture(DK_Texture texture, unsigned int hash) {
#if DK_D_DRAW_TEST_TEXTURE
    return DK_gl_test_texture;
#else
    if (DK_num_textures[texture] > 0) {
        return DK_gl_textures[texture][hash % DK_num_textures[texture]];
    } else {
        return 0;
    }
#endif
}

void DK_load_textures() {
    int textureId;
    char filename[64];
    for (textureId = 0; textureId < DK_TEX_COUNT; ++textureId) {
        int* count = &DK_num_textures[textureId];
        *count = 0;
        do {
            // Generate file name.
            sprintf(filename, "%s%s_%d%s", DK_TEX_DIR, DK_texture_names[textureId], *count, DK_TEX_FILETYPE);

            // Try to load the texture.
            SDL_Surface* texture = IMG_Load(filename);
            if (texture == NULL) {
                // Failed loading, assume the previous one was the last.
                break;
            }

            // Success, store as surface and as openGL texture.
            DK_textures[textureId][*count] = texture;

            // Next slot.
            ++(*count);
        } while (*count < DK_TEX_MAX_VARIATIONS);

        fprintf(stdout, "Found %d variations of texture '%s'.\n", *count, DK_texture_names[textureId]);
    }
    
    // Load test texture.
    sprintf(filename, "%s%s%s", DK_TEX_DIR, "test", DK_TEX_FILETYPE);

    DK_test_texture = IMG_Load(filename);
}

/** Generates an openGL texture from an SDL surface */
GLuint DK_surface2glTex(const SDL_Surface* surface) {
    // Generate a texture object and bind it.
    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);

    // Scaling settings.
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    // Set the data from our surface.
    gluBuild2DMipmaps(GL_TEXTURE_2D, surface->format->BytesPerPixel,
            surface->w, surface->h, (surface->format->Amask) ? GL_RGBA : GL_RGB,
            GL_UNSIGNED_BYTE, surface->pixels);

    return texture;
}

void DK_opengl_textures() {
    int textureId;
    for (textureId = 0; textureId < DK_TEX_COUNT; ++textureId) {
        int count;
        for (count = DK_num_textures[textureId] - 1; count >= 0; --count) {
            DK_gl_textures[textureId][count] = DK_surface2glTex(DK_textures[textureId][count]);
        }
    }
    
    DK_gl_test_texture = DK_surface2glTex(DK_test_texture);
}
