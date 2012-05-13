#include "textures.h"

#include <stdio.h>

#include <SDL/SDL.h>
#include <SDL/SDL_image.h>
#include <GL/glew.h>

#include "config.h"
#include "log.h"

///////////////////////////////////////////////////////////////////////////////
// Variables
///////////////////////////////////////////////////////////////////////////////

/** Struct holding info on a single texture type */
typedef struct Texture {
    SDL_Surface** surface;
    GLuint* textureId;
    unsigned int count;
} Texture;

static Texture* gTextures = 0;
static unsigned int gTextureCount = 0;
static unsigned int gTextureCapacity = 0;

static SDL_Surface* gTestTextureSurface = 0;
static GLuint gTestTextureId = 0;

///////////////////////////////////////////////////////////////////////////////
// Utility methods
///////////////////////////////////////////////////////////////////////////////

static void loadTestTexture(void) {
    char filename[256];
    if (gTestTextureSurface) {
        return;
    }
    snprintf(filename, sizeof (filename), "%s%s%s", MP_TEX_DIR, "test", MP_TEX_FILETYPE);
    gTestTextureSurface = IMG_Load(filename);
    if (gTestTextureSurface == NULL) {
        MP_log_fatal("Failed loading dummy texture.\n");
    }
}

static Texture* getNextFreeEntry(void) {
    if (gTextureCount >= gTextureCapacity) {
        gTextureCapacity = gTextureCapacity * 2 + 1;
        if (!(gTextures = realloc(gTextures, gTextureCapacity * sizeof (Texture)))) {
            MP_log_fatal("Out of memory while resizing texture list.\n");
        }
    }
    return &gTextures[gTextureCount++];
}

/** Generates an openGL texture from an SDL surface */
static GLuint generateTexture(const SDL_Surface* surface) {
    // Generate a texture object and bind it.
    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);

    // Scaling settings.
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    // Set the data from our surface.
#ifdef GL_GENERATE_MIPMAP
    glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_TRUE);

    glTexImage2D(GL_TEXTURE_2D, 0, surface->format->BytesPerPixel,
                 surface->w, surface->h, 0, (surface->format->Amask) ? GL_RGBA : GL_RGB,
                 GL_UNSIGNED_BYTE, surface->pixels);
#else
    gluBuild2DMipmaps(GL_TEXTURE_2D, surface->format->BytesPerPixel,
                      surface->w, surface->h, (surface->format->Amask) ? GL_RGBA : GL_RGB,
                      GL_UNSIGNED_BYTE, surface->pixels);
#endif

    return texture;
}

///////////////////////////////////////////////////////////////////////////////
// Header implementation
///////////////////////////////////////////////////////////////////////////////

GLuint MP_GetTexture(MP_TextureID textureId, unsigned int hash) {
    if (!MP_d_draw_test_texture && textureId > 0 && textureId - 1 < gTextureCount) {
        const Texture* texture = &gTextures[textureId - 1];
        return texture->textureId[hash % texture->count];
    } else {
        return gTestTextureId;
    }
}

MP_TextureID MP_LoadTexture(const char* basename) {
    unsigned int capacity = 0;
    char filename[256];
    SDL_Surface* surface;
    Texture texture = {0, 0, 0};
    while (1) {
        // Generate file name.
        if (snprintf(filename, sizeof (filename), "%s%s_%d%s", MP_TEX_DIR, basename, texture.count, MP_TEX_FILETYPE) > (int) sizeof (filename)) {
            MP_log_error("Texture name too long: '%s'.\n", basename);
            break;
        }

        // Try to load the texture.
        surface = IMG_Load(filename);
        if (surface == NULL) {
            // Failed loading, assume the previous one was the last.
            break;
        }

        // Success, store as surface and as openGL texture.
        if (texture.count >= capacity) {
            capacity = capacity * 2 + 1;
            texture.surface = realloc(texture.surface, capacity * sizeof (SDL_Surface*));
            texture.textureId = realloc(texture.textureId, capacity * sizeof (GLuint));
        }
        texture.surface[texture.count] = surface;
        texture.textureId[texture.count] = 0;
        ++texture.count;
    }

    if (texture.count == 0) {
        MP_log_warning("Found no variations of texture '%s'.\n", basename);
    }

    if (texture.count > 0) {
        // If we have any entries at all, save this texture type.
        *getNextFreeEntry() = texture;
        return gTextureCount;
    } else {
        // Otherwise return zero.
        return 0;
    }
}

void MP_UnloadTextures(void) {
    MP_GL_DeleteTextures();
    if (gTestTextureSurface) {
        SDL_FreeSurface(gTestTextureSurface);
        gTestTextureSurface = 0;
    }
    for (unsigned int t = 0; t < gTextureCount; ++t) {
        Texture* texture = &gTextures[t];
        for (unsigned int v = 0; v < texture->count; ++v) {
            if (texture->surface[v]) {
                SDL_FreeSurface(texture->surface[v]);
                texture->surface[v] = 0;
            }
        }
        free(texture->surface);
        texture->surface = 0;
        free(texture->textureId);
        texture->textureId = 0;
    }
    if (gTextures) {
        free(gTextures);
        gTextures = 0;
    }
    gTextureCount = 0;
    gTextureCapacity = 0;
}

void MP_GL_GenerateTextures(void) {
    loadTestTexture();
    gTestTextureId = generateTexture(gTestTextureSurface);
    for (unsigned int t = 0; t < gTextureCount; ++t) {
        Texture* texture = &gTextures[t];
        for (unsigned int v = 0; v < texture->count; ++v) {
            texture->textureId[v] = generateTexture(texture->surface[v]);
        }
    }

    EXIT_ON_OPENGL_ERROR();
}

void MP_GL_DeleteTextures(void) {
    if (gTestTextureId) {
        glDeleteTextures(1, &gTestTextureId);
        gTestTextureId = 0;
    }
    for (unsigned int t = 0; t < gTextureCount; ++t) {
        Texture* texture = &gTextures[t];
        for (unsigned int v = 0; v < texture->count; ++v) {
            if (texture->textureId[v]) {
                glDeleteTextures(1, &texture->textureId[v]);
                texture->textureId[v] = 0;
            }
        }
    }

    EXIT_ON_OPENGL_ERROR();
}
