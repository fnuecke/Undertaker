#include <math.h>
#include <string.h>
#include <malloc.h>

#include <SDL/SDL.h>
#include <GL/glew.h>

#include "astar.h"
#include "bitset.h"
#include "block.h"
#include "callbacks.h"
#include "camera.h"
#include "config.h"
#include "cursor.h"
#include "graphics.h"
#include "jobs.h"
#include "map.h"
#include "picking.h"
#include "render.h"
#include "selection.h"
#include "simplexnoise.h"
#include "textures.h"
#include "units.h"
#include "vmath.h"

///////////////////////////////////////////////////////////////////////////////
// Constants
///////////////////////////////////////////////////////////////////////////////

/** Base z coordinates for vertices, based on z index in vertex buffer. */
static const float z_coords[5] = {
    -DK_WATER_LEVEL,
    -DK_WATER_LEVEL / 2.0f,
    0,
    DK_BLOCK_HEIGHT / 2.0f,
    DK_BLOCK_HEIGHT
};

///////////////////////////////////////////////////////////////////////////////
// Internal variables
///////////////////////////////////////////////////////////////////////////////

/**
 * The current map.
 */
static DK_Block* gMap = 0;

/**
 * Size of the current map (width and height).
 */
static unsigned short gMapSize = 0;

/**
 * Currently hovered block coordinates.
 */
static int gCursorX = 0, gCursorY = 0;

/**
 * The light the user's cursor emits.
 */
static DK_Light gHandLight;

/**
 * Currently doing a picking (select) render pass?
 */
static bool gIsPicking = false;

/**
 * List of methods to call when map size changes.
 */
static Callbacks* gMapSizeChangeCallbacks = 0;

/**
 * The material used for shading stuff we draw.
 */
static DK_Material gMaterial;

///////////////////////////////////////////////////////////////////////////////
// Map model data
///////////////////////////////////////////////////////////////////////////////

enum {
    SIDE_TOP,
    SIDE_NORTH,
    SIDE_SOUTH,
    SIDE_EAST,
    SIDE_WEST
};

static struct Vertex {
    /** Position of the vertex in world space */
    vec3 position; // 3 * 1 = 3

    /** The normal associated to the vertex, in different orientations */
    vec3 normal[5]; // 3 * 5 = 15

    /** Texture coordinates in different orientations */
    vec2 texCoord[5]; // 2 * 5 = 10

    // 3 + 15 + 10 = 28 * sizeof(float) = 112 byte
    // 128 % 112 = 16 -> need 16 byte padding

    /** Pad to 128 byte */
    char padding[16];
} *gVertices = 0;

/** Array and buffer IDs */
static GLuint gVertexBufferID = 0;

/** Are our buffers initialized, i.e. should we push changes to the GPU? */
static bool gShouldUpdateVertexBuffer = false;

/** Number of vertices in x and y direction */
static unsigned int gVerticesPerDimension = 0;

///////////////////////////////////////////////////////////////////////////////
// Utility methods
///////////////////////////////////////////////////////////////////////////////

/**
 * Converts the specified three dimensional index of a symmetric vertex or
 * normal array to the one dimensional index in that array.
 * @param x the x index.
 * @param y the y index.
 * @param z the z index.
 * @return the index in the one dimensional array.
 */
inline static unsigned int fi(unsigned int x, unsigned int y, unsigned int z) {
    return (z * gVerticesPerDimension * gVerticesPerDimension) +y * gVerticesPerDimension + x;
}

inline static int DK_NEQ(unsigned int a, unsigned int b) {
    return fabs(b - (a + 0.5f)) < 0.01f;
}

inline static int DK_NLT(unsigned int a, unsigned int b) {
    return b - (a + 0.5f) > 0.0f;
}

inline static int isBlockOpen(const DK_Block * block) {
    return block && (block->meta->level < DK_BLOCK_LEVEL_HIGH);
}

///////////////////////////////////////////////////////////////////////////////
// Map model updating: vertices
///////////////////////////////////////////////////////////////////////////////

/** Used to differentiate how blocks effect offsetting */
typedef enum {
    OFFSET_NONE,
    OFFSET_INCREASE,
    OFFSET_DECREASE
} DK_Offset;

/** Get influence of a specific block on noise */
static double getNoiseFromBlock(DK_Offset* offset, const DK_Block* block) {
    *offset = OFFSET_NONE;
    if (isBlockOpen(block) && block->owner == DK_PLAYER_NONE) {
        *offset = OFFSET_INCREASE;
        return 1.5;
    } else if (block->owner != DK_PLAYER_NONE) {
        *offset = OFFSET_DECREASE;
        return 0.5;
    }
    return 1;
}

/**
 * Gets noise factor (amplitude) based on neighboring blocks.
 * Also gets a directed offset based on where the neighboring block are (away
 * from empty blocks).
 */
static float computeOffset(float* offset, float x, float y) {
    DK_Offset offset_type;
    float offset_reduction[2] = {1, 1};

    // Get full coordinate for our doubles (which might be at half coordinates.
    const int i = (int) (x + 0.1f);
    const int j = (int) (y + 0.1f);

    // Find out what kind of point this is, among the following:
    // +---+---+---+
    // | A | B | C |
    // +---0-1-+---+
    // | D 2 3E| F |
    // +---+---+---+
    // | G | H | I |
    // +---+---+---+
    // We always want to check neighbors in a 1.5 radius; e.g. for 0 we want to
    // check A,B,D and E, for 1 A,B,C,D,E,F, for 2 A,B,D,E,G and H, for 3 we
    // want all. Note that i,j for (1,2,3) will always equal (x,y) for 0.
    int x_begin = i - 1, y_begin = j - 1;
    int x_end, y_end;
    float normalizer, factor;
    if (fabs(x - i - 0.5f) < 0.01f) {
        // X is halfway, so we're at 1 or 3.
        x_end = i + 1;
        if (fabs(y - j - 0.5f) < 0.01f) {
            // Y is halfway, so we're at 3, the most expensive case.
            y_end = j + 1;
            normalizer = 9.0f;
        } else {
            // Y is full, so we're at 1.
            y_end = j;
            normalizer = 6.0f;
        }
    } else {
        // X is full, so we're at 0 or 2.
        x_end = i;
        if (fabs(y - j - 0.5f) < 0.01f) {
            // Y is halfway, so we're at 2.
            y_end = j + 1;
            normalizer = 6.0f;
        } else {
            // Y is full, so we're at 0.
            y_end = j;
            normalizer = 4.0f;
        }
    }

    if (x_begin < 0) {
        x_begin = 0;
    }
    if (x_end > gMapSize - 1) {
        x_end = gMapSize - 1;
    }
    if (y_begin < 0) {
        y_begin = 0;
    }
    if (y_end > gMapSize - 1) {
        y_end = gMapSize - 1;
    }

    // Walk all the block necessary.
    factor = 0;
    for (unsigned int k = x_begin; (int) k <= x_end; ++k) {
        for (unsigned int l = y_begin; (int) l <= y_end; ++l) {
            factor += getNoiseFromBlock(&offset_type, DK_GetBlockAt(k, l));
            if (offset_type == OFFSET_INCREASE) {
                if (!DK_NEQ(k, x)) {
                    if (DK_NLT(k, x)) {
                        offset[0] += 1;
                    } else if ((int) k >= i) {
                        offset[0] -= 1;
                    }
                }
                if (!DK_NEQ(l, y)) {
                    if (DK_NLT(l, y)) {
                        offset[1] += 1;
                    } else if ((int) l >= j) {
                        offset[1] -= 1;
                    }
                }
            } else if (offset_type == OFFSET_DECREASE) {
                offset_reduction[0] *= DK_OWNED_NOISE_REDUCTION;
                offset_reduction[1] *= DK_OWNED_NOISE_REDUCTION;
            }
        }
    }

    if (fabsf(offset[0]) > 0.001f || fabsf(offset[1]) > 0.001f) {
        float len;
        offset[0] *= offset_reduction[0] / normalizer;
        offset[1] *= offset_reduction[1] / normalizer;
        len = sqrt(offset[0] * offset[0] + offset[1] * offset[1]);
        if (len > 1) {
            offset[0] /= len;
            offset[1] /= len;
        }
    }

    offset[0] *= DK_BLOCK_MAX_NOISE_OFFSET;
    offset[1] *= DK_BLOCK_MAX_NOISE_OFFSET;

    return factor / normalizer;
}

static void updateVerticesAt(int x, int y) {
    for (unsigned int z = 0; z < 5; ++z) {
        const unsigned int idx = fi(x, y, z);
        vec3* v = &gVertices[idx].position;
        const float vx = (x - DK_MAP_BORDER) / 2.0f * DK_BLOCK_SIZE;
        const float vy = (y - DK_MAP_BORDER) / 2.0f * DK_BLOCK_SIZE;
        const float vz = z_coords[z];
#if DK_D_TERRAIN_NOISE
        float offset[2] = {0, 0};
        float factor = 1.0f;
        float offset_factor;
#if DK_D_USE_NOISE_OFFSET
        factor = computeOffset(offset, (x - DK_MAP_BORDER) / 2.0f, (y - DK_MAP_BORDER) / 2.0f);
        offset_factor = (0.25f - (fabs(fabs(z / 4.0f - 0.5f) - 0.25f) - 0.25f)) * z - 0.5f;
        offset[0] *= offset_factor;
        offset[1] *= offset_factor;
#endif
        v->d.x = vx + factor * snoise4(vx, vy, vz, 0) + offset[0];
        v->d.y = vy + factor * snoise4(vx, vy, vz, 1) + offset[1];
        v->d.z = vz + snoise4(vx, vy, vz, 2);
#else
        v->x = vx;
        v->y = vy;
        v->z = vz;
#endif

        // Update data on GPU?
        if (gShouldUpdateVertexBuffer) {
            glBindBuffer(GL_ARRAY_BUFFER, gVertexBufferID);
            glBufferSubData(GL_ARRAY_BUFFER, idx * sizeof (struct Vertex), sizeof (vec3), v);

            EXIT_ON_OPENGL_ERROR();
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
// Map model updating: normals
///////////////////////////////////////////////////////////////////////////////

/**
 * Compute the normal of a triangle described by the three specified vertices.
 */
static void computeNormal(vec3* normal, const vec3* v0, const vec3* v1, const vec3* v2) {
    vec3 a, b;
    v3sub(&a, v1, v0);
    v3sub(&b, v2, v0);
    v3cross(normal, &a, &b);
}

/**
 * Get an interpolated (smoothed) normal at v0, with the specified neighbors.
 */
static void interpolateNormal(vec3* n, const vec3* v0,
        const vec3* v1, const vec3* v2, const vec3* v3, const vec3* v4,
        const vec3* v5, const vec3* v6, const vec3* v7, const vec3* v8) {
    // Compute normals based on neighbors and accumulate.
    vec3 tmp;
    n->d.x = 0;
    n->d.y = 0;
    n->d.z = 0;
    computeNormal(&tmp, v0, v1, v2);
    v3iadd(n, &tmp);
    computeNormal(&tmp, v0, v2, v3);
    v3iadd(n, &tmp);
    computeNormal(&tmp, v0, v3, v4);
    v3iadd(n, &tmp);
    computeNormal(&tmp, v0, v4, v5);
    v3iadd(n, &tmp);
    computeNormal(&tmp, v0, v5, v6);
    v3iadd(n, &tmp);
    computeNormal(&tmp, v0, v6, v7);
    v3iadd(n, &tmp);
    computeNormal(&tmp, v0, v7, v8);
    v3iadd(n, &tmp);
    computeNormal(&tmp, v0, v8, v1);
    v3iadd(n, &tmp);

    // Then normalize.
    v3inormalize(n);
}

static void updateNormalsAt(int x, int y) {
#define P(x, y, z) (&gVertices[fi(x, y, z)].position)
    for (unsigned int z = 0; z < 5; ++z) {
        const unsigned int idx = fi(x, y, z);
        struct Vertex* vert = &gVertices[idx];
        const vec3* v0 = &vert->position;

        // Compute z normals.
        vec3* n = &vert->normal[SIDE_TOP];

        // Compute normals based on neighbors and accumulate.
        interpolateNormal(n, v0,
                P(x, y - 1, z),
                P(x + 1, y - 1, z),
                P(x + 1, y, z),
                P(x + 1, y + 1, z),
                P(x, y + 1, z),
                P(x - 1, y + 1, z),
                P(x - 1, y, z),
                P(x - 1, y - 1, z));

        // If we don't get out of bounds issues, compute x and y, too.
        if (z > 1 && z < 4) {
            // Compute y normals.
            n = &vert->normal[SIDE_NORTH];

            // Compute normals based on neighbors and accumulate.
            interpolateNormal(n, v0,
                    P(x + 1, y, z + 1),
                    P(x + 1, y, z),
                    P(x + 1, y, z - 1),
                    P(x, y, z - 1),
                    P(x - 1, y, z - 1),
                    P(x - 1, y, z),
                    P(x - 1, y, z + 1),
                    P(x, y, z + 1));

            // Other side is just the same, but with inverted direction.
            vert->normal[SIDE_SOUTH] = *n;
            v3imuls(&vert->normal[SIDE_SOUTH], -1);

            // Compute x normals.
            n = &vert->normal[SIDE_EAST];

            // Compute normals based on neighbors and accumulate.
            interpolateNormal(n, v0,
                    P(x, y - 1, z + 1),
                    P(x, y - 1, z),
                    P(x, y - 1, z - 1),
                    P(x, y, z - 1),
                    P(x, y + 1, z - 1),
                    P(x, y + 1, z),
                    P(x, y + 1, z + 1),
                    P(x, y, z + 1));

            // Other side is just the same, but with inverted direction.
            vert->normal[SIDE_WEST] = *n;
            v3imuls(&vert->normal[SIDE_WEST], -1);
        }

        // Update data on GPU?
        if (gShouldUpdateVertexBuffer) {
            glBindBuffer(GL_ARRAY_BUFFER, gVertexBufferID);
            glBufferSubData(GL_ARRAY_BUFFER, idx * sizeof (struct Vertex) + sizeof (vec3), sizeof (vec3) * 5, vert->normal);

            EXIT_ON_OPENGL_ERROR();
        }
    }
#undef P
}

///////////////////////////////////////////////////////////////////////////////
// Block updating
///////////////////////////////////////////////////////////////////////////////

static void updateBlock(DK_Block* block) {
    unsigned short x, y;

    // Ignore requests for invalid blocks.
    if (!block) {
        return;
    }

    if (!DK_GetBlockCoordinates(&x, &y, block)) {
        // Out of bounds, ignore.
        return;
    }

    // Update model.
    {
        const int start_x = x * 2 - 1 + DK_MAP_BORDER;
        const int start_y = y * 2 - 1 + DK_MAP_BORDER;
        const int end_x = x * 2 + 3 + DK_MAP_BORDER;
        const int end_y = y * 2 + 3 + DK_MAP_BORDER;

        for (int lx = start_x; lx <= end_x; ++lx) {
            for (int ly = start_y; ly <= end_y; ++ly) {
                updateVerticesAt(lx, ly);
            }
        }

        for (int lx = start_x; lx <= end_x; ++lx) {
            for (int ly = start_y; ly <= end_y; ++ly) {
                updateNormalsAt(lx, ly);
            }
        }
    }

    // Update jobs for this block by deselecting it.
    for (int i = 0; i < DK_PLAYER_COUNT; ++i) {
        if (!DK_DeselectBlock(DK_PLAYER_NONE + i, x, y)) {
            DK_UpdateJobsForBlock(DK_PLAYER_NONE + i, x, y);
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
// Map model rendering
///////////////////////////////////////////////////////////////////////////////

static void pushTexture(int x, int y, unsigned int z, DK_TextureID textureId) {
    const unsigned int variation = (unsigned int) ((snoise2(x, y + z) + 1) / 2 * DK_TEX_MAX_VARIATIONS);
    gMaterial.textures[gMaterial.textureCount] = DK_GetTexture(textureId, variation);
    ++gMaterial.textureCount;
    DK_SetMaterial(&gMaterial);
}

static void setDiffuseColor(float r, float g, float b, float a) {
    gMaterial.diffuseColor.c.r = r;
    gMaterial.diffuseColor.c.g = g;
    gMaterial.diffuseColor.c.b = b;
    gMaterial.diffuseColor.c.a = a;
    DK_SetMaterial(&gMaterial);
}

static void beginDraw(void) {
    // Bind our vertex buffer as the array we use for attribute lookups.
    glBindBuffer(GL_ARRAY_BUFFER, gVertexBufferID);

    // Use it for the attributes we need.
    glEnableVertexAttribArray(DK_GetPositionAttributeLocation());
    glEnableVertexAttribArray(DK_GetNormalAttributeLocation());
    glEnableVertexAttribArray(DK_GetTextureCoordinateAttributeLocation());

    // Set the pointers that don't change.
    glVertexAttribPointer(DK_GetPositionAttributeLocation(),
            3, GL_FLOAT, GL_FALSE, sizeof (struct Vertex), 0);

    EXIT_ON_OPENGL_ERROR();
}

static void endDraw(void) {
    // Stop using vertex buffer for attributes..
    glDisableVertexAttribArray(DK_GetPositionAttributeLocation());
    glDisableVertexAttribArray(DK_GetNormalAttributeLocation());
    glDisableVertexAttribArray(DK_GetTextureCoordinateAttributeLocation());

    // Unbind vertex buffer.
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    EXIT_ON_OPENGL_ERROR();
}

#define BUFFER_OFFSET(i) ((char *)NULL + (i))
#define NORMAL(side) BUFFER_OFFSET(sizeof(vec3) * (1 + side))
#define TEXTURE(side) BUFFER_OFFSET(sizeof(vec3) * 6 + sizeof(vec2) * side)

static void drawTop(int x, int y, unsigned int z) {
    GLuint indices[6];
    x = x * 2 + DK_MAP_BORDER;
    y = y * 2 + DK_MAP_BORDER;

    glVertexAttribPointer(DK_GetNormalAttributeLocation(),
            3, GL_FLOAT, GL_FALSE, sizeof (struct Vertex), NORMAL(SIDE_TOP));
    glVertexAttribPointer(DK_GetTextureCoordinateAttributeLocation(),
            2, GL_FLOAT, GL_FALSE, sizeof (struct Vertex), TEXTURE(SIDE_TOP));

    indices[0] = fi(x, y + 2, z);
    indices[1] = fi(x, y + 1, z);
    indices[2] = fi(x + 1, y + 2, z);
    indices[3] = fi(x + 1, y + 1, z);
    indices[4] = fi(x + 2, y + 2, z);
    indices[5] = fi(x + 2, y + 1, z);
    glDrawElements(GL_QUAD_STRIP, 6, GL_UNSIGNED_INT, &indices);

    indices[0] = fi(x, y + 1, z);
    indices[1] = fi(x, y, z);
    indices[2] = fi(x + 1, y + 1, z);
    indices[3] = fi(x + 1, y, z);
    indices[4] = fi(x + 2, y + 1, z);
    indices[5] = fi(x + 2, y, z);
    glDrawElements(GL_QUAD_STRIP, 6, GL_UNSIGNED_INT, &indices);

    EXIT_ON_OPENGL_ERROR();
}

static void drawNorth(int x, int y, unsigned int z) {
    GLuint indices[6];
    x = x * 2 + DK_MAP_BORDER;
    y = y * 2 + DK_MAP_BORDER;

    glVertexAttribPointer(DK_GetNormalAttributeLocation(),
            3, GL_FLOAT, GL_FALSE, sizeof (struct Vertex), NORMAL(SIDE_NORTH));
    glVertexAttribPointer(DK_GetTextureCoordinateAttributeLocation(),
            2, GL_FLOAT, GL_FALSE, sizeof (struct Vertex), TEXTURE(SIDE_NORTH));

    indices[0] = fi(x + 2, y, z + 2);
    indices[1] = fi(x + 2, y, z + 1);
    indices[2] = fi(x + 1, y, z + 2);
    indices[3] = fi(x + 1, y, z + 1);
    indices[4] = fi(x, y, z + 2);
    indices[5] = fi(x, y, z + 1);
    glDrawElements(GL_QUAD_STRIP, 6, GL_UNSIGNED_INT, &indices);

    indices[0] = fi(x + 2, y, z + 1);
    indices[1] = fi(x + 2, y, z);
    indices[2] = fi(x + 1, y, z + 1);
    indices[3] = fi(x + 1, y, z);
    indices[4] = fi(x, y, z + 1);
    indices[5] = fi(x, y, z);
    glDrawElements(GL_QUAD_STRIP, 6, GL_UNSIGNED_INT, &indices);

    EXIT_ON_OPENGL_ERROR();
}

static void drawSouth(int x, int y, unsigned int z) {
    GLuint indices[6];
    x = x * 2 + DK_MAP_BORDER;
    y = y * 2 + DK_MAP_BORDER;

    glVertexAttribPointer(DK_GetNormalAttributeLocation(),
            3, GL_FLOAT, GL_FALSE, sizeof (struct Vertex), NORMAL(SIDE_SOUTH));
    glVertexAttribPointer(DK_GetTextureCoordinateAttributeLocation(),
            2, GL_FLOAT, GL_FALSE, sizeof (struct Vertex), TEXTURE(SIDE_SOUTH));

    indices[0] = fi(x, y, z + 2);
    indices[1] = fi(x, y, z + 1);
    indices[2] = fi(x + 1, y, z + 2);
    indices[3] = fi(x + 1, y, z + 1);
    indices[4] = fi(x + 2, y, z + 2);
    indices[5] = fi(x + 2, y, z + 1);
    glDrawElements(GL_QUAD_STRIP, 6, GL_UNSIGNED_INT, &indices);

    indices[0] = fi(x, y, z + 1);
    indices[1] = fi(x, y, z);
    indices[2] = fi(x + 1, y, z + 1);
    indices[3] = fi(x + 1, y, z);
    indices[4] = fi(x + 2, y, z + 1);
    indices[5] = fi(x + 2, y, z);
    glDrawElements(GL_QUAD_STRIP, 6, GL_UNSIGNED_INT, &indices);

    EXIT_ON_OPENGL_ERROR();
}

static void drawEast(int x, int y, unsigned int z) {
    GLuint indices[6];
    x = x * 2 + DK_MAP_BORDER;
    y = y * 2 + DK_MAP_BORDER;

    glVertexAttribPointer(DK_GetNormalAttributeLocation(),
            3, GL_FLOAT, GL_FALSE, sizeof (struct Vertex), NORMAL(SIDE_EAST));
    glVertexAttribPointer(DK_GetTextureCoordinateAttributeLocation(),
            2, GL_FLOAT, GL_FALSE, sizeof (struct Vertex), TEXTURE(SIDE_EAST));

    indices[0] = fi(x, y, z + 2);
    indices[1] = fi(x, y, z + 1);
    indices[2] = fi(x, y + 1, z + 2);
    indices[3] = fi(x, y + 1, z + 1);
    indices[4] = fi(x, y + 2, z + 2);
    indices[5] = fi(x, y + 2, z + 1);
    glDrawElements(GL_QUAD_STRIP, 6, GL_UNSIGNED_INT, &indices);

    indices[0] = fi(x, y, z + 1);
    indices[1] = fi(x, y, z);
    indices[2] = fi(x, y + 1, z + 1);
    indices[3] = fi(x, y + 1, z);
    indices[4] = fi(x, y + 2, z + 1);
    indices[5] = fi(x, y + 2, z);
    glDrawElements(GL_QUAD_STRIP, 6, GL_UNSIGNED_INT, &indices);

    EXIT_ON_OPENGL_ERROR();
}

static void drawWest(int x, int y, unsigned int z) {
    GLuint indices[6];
    x = x * 2 + DK_MAP_BORDER;
    y = y * 2 + DK_MAP_BORDER;

    glVertexAttribPointer(DK_GetNormalAttributeLocation(),
            3, GL_FLOAT, GL_FALSE, sizeof (struct Vertex), NORMAL(SIDE_WEST));
    glVertexAttribPointer(DK_GetTextureCoordinateAttributeLocation(),
            2, GL_FLOAT, GL_FALSE, sizeof (struct Vertex), TEXTURE(SIDE_WEST));

    indices[0] = fi(x, y + 2, z + 2);
    indices[1] = fi(x, y + 2, z + 1);
    indices[2] = fi(x, y + 1, z + 2);
    indices[3] = fi(x, y + 1, z + 1);
    indices[4] = fi(x, y, z + 2);
    indices[5] = fi(x, y, z + 1);
    glDrawElements(GL_QUAD_STRIP, 6, GL_UNSIGNED_INT, &indices);

    indices[0] = fi(x, y + 2, z + 1);
    indices[1] = fi(x, y + 2, z);
    indices[2] = fi(x, y + 1, z + 1);
    indices[3] = fi(x, y + 1, z);
    indices[4] = fi(x, y, z + 1);
    indices[5] = fi(x, y, z);
    glDrawElements(GL_QUAD_STRIP, 6, GL_UNSIGNED_INT, &indices);

    EXIT_ON_OPENGL_ERROR();
}

#undef NORMAL
#undef TEXTURE
#undef BUFFER_OFFSET

static void renderSelectionOutline(void) {
    GLuint indices[4];
    DK_Selection selection = DK_GetSelection();

    selection.startX += DK_MAP_BORDER / 2;
    selection.startY += DK_MAP_BORDER / 2;
    selection.endX += DK_MAP_BORDER / 2;
    selection.endY += DK_MAP_BORDER / 2;

    beginDraw();

    // Set up for line drawing.
    glLineWidth(3.0f + DK_GetCameraZoom() * 3.0f);
    DK_InitMaterial(&gMaterial);
    setDiffuseColor(DK_MAP_OUTLINE_COLOR_R,
            DK_MAP_OUTLINE_COLOR_G,
            DK_MAP_OUTLINE_COLOR_B,
            DK_MAP_OUTLINE_COLOR_A);

    // Paint on top of previous image data.
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Don't change the depth buffer.
    glDepthMask(GL_FALSE);

    for (int x = selection.startX; x <= selection.endX; ++x) {
        int map_x = x - DK_MAP_BORDER / 2;
        for (int y = selection.startY; y <= selection.endY; ++y) {
            int map_y = y - DK_MAP_BORDER / 2;
            if (!DK_IsBlockSelectable(DK_PLAYER_ONE, map_x, map_y)) {
                continue;
            }

            // Draw north outline.
            if (y == selection.endY ||
                    isBlockOpen(DK_GetBlockAt(map_x, map_y + 1))) {
                indices[0] = fi(x * 2, y * 2 + 2, 4);
                indices[1] = fi(x * 2 + 1, y * 2 + 2, 4);
                indices[2] = fi(x * 2 + 2, y * 2 + 2, 4);
                glDrawElements(GL_LINE_STRIP, 3, GL_UNSIGNED_INT, &indices);

                indices[0] = fi(x * 2, y * 2 + 2, 2);
                indices[1] = fi(x * 2 + 1, y * 2 + 2, 2);
                indices[2] = fi(x * 2 + 2, y * 2 + 2, 2);
                glDrawElements(GL_LINE_STRIP, 3, GL_UNSIGNED_INT, &indices);

                // Top-down lines?
                if (x == selection.startX ||
                        ((isBlockOpen(DK_GetBlockAt(map_x, map_y + 1)) ^
                        isBlockOpen(DK_GetBlockAt(map_x - 1, map_y + 1))) ||
                        isBlockOpen(DK_GetBlockAt(map_x - 1, map_y)))) {
                    // Draw north west top-to-bottom line.
                    indices[0] = fi(x * 2, y * 2 + 2, 4);
                    indices[1] = fi(x * 2, y * 2 + 2, 3);
                    indices[2] = fi(x * 2, y * 2 + 2, 2);
                    glDrawElements(GL_LINE_STRIP, 3, GL_UNSIGNED_INT, &indices);
                }
                if (x == selection.endX ||
                        ((isBlockOpen(DK_GetBlockAt(map_x, map_y + 1)) ^
                        isBlockOpen(DK_GetBlockAt(map_x + 1, map_y + 1))) ||
                        isBlockOpen(DK_GetBlockAt(map_x + 1, map_y)))) {
                    // Draw north east top-to-bottom line.
                    indices[0] = fi(x * 2 + 2, y * 2 + 2, 4);
                    indices[1] = fi(x * 2 + 2, y * 2 + 2, 3);
                    indices[2] = fi(x * 2 + 2, y * 2 + 2, 2);
                    glDrawElements(GL_LINE_STRIP, 3, GL_UNSIGNED_INT, &indices);
                }
            }

            // Draw south outline.
            if (y == selection.startY ||
                    isBlockOpen(DK_GetBlockAt(map_x, map_y - 1))) {
                //DK_PushModelMatrix();
                indices[0] = fi(x * 2, y * 2, 4);
                indices[1] = fi(x * 2 + 1, y * 2, 4);
                indices[2] = fi(x * 2 + 2, y * 2, 4);
                glDrawElements(GL_LINE_STRIP, 3, GL_UNSIGNED_INT, &indices);

                indices[0] = fi(x * 2, y * 2, 2);
                indices[1] = fi(x * 2 + 1, y * 2, 2);
                indices[2] = fi(x * 2 + 2, y * 2, 2);
                glDrawElements(GL_LINE_STRIP, 3, GL_UNSIGNED_INT, &indices);

                // Top-down lines?
                if (x == selection.startX ||
                        ((isBlockOpen(DK_GetBlockAt(map_x, map_y - 1)) ^
                        isBlockOpen(DK_GetBlockAt(map_x - 1, map_y - 1))) ||
                        isBlockOpen(DK_GetBlockAt(map_x - 1, map_y)))) {
                    // Draw south west top-to-bottom line.
                    indices[0] = fi(x * 2, y * 2, 4);
                    indices[1] = fi(x * 2, y * 2, 3);
                    indices[2] = fi(x * 2, y * 2, 2);
                    glDrawElements(GL_LINE_STRIP, 3, GL_UNSIGNED_INT, &indices);
                }
                if (x == selection.endX ||
                        ((isBlockOpen(DK_GetBlockAt(map_x, map_y - 1)) ^
                        isBlockOpen(DK_GetBlockAt(map_x + 1, map_y - 1))) ||
                        isBlockOpen(DK_GetBlockAt(map_x + 1, map_y)))) {
                    // Draw south east top-to-bottom line.
                    indices[0] = fi(x * 2 + 2, y * 2, 4);
                    indices[1] = fi(x * 2 + 2, y * 2, 3);
                    indices[2] = fi(x * 2 + 2, y * 2, 2);
                    glDrawElements(GL_LINE_STRIP, 3, GL_UNSIGNED_INT, &indices);
                }
            }

            // Draw east outline.
            if (x == selection.endX ||
                    isBlockOpen(DK_GetBlockAt(map_x + 1, map_y))) {
                indices[0] = fi(x * 2 + 2, y * 2, 4);
                indices[1] = fi(x * 2 + 2, y * 2 + 1, 4);
                indices[2] = fi(x * 2 + 2, y * 2 + 2, 4);
                glDrawElements(GL_LINE_STRIP, 3, GL_UNSIGNED_INT, &indices);

                indices[0] = fi(x * 2 + 2, y * 2, 2);
                indices[1] = fi(x * 2 + 2, y * 2 + 1, 2);
                indices[2] = fi(x * 2 + 2, y * 2 + 2, 2);
                glDrawElements(GL_LINE_STRIP, 3, GL_UNSIGNED_INT, &indices);
            }

            // Draw west outline.
            if (x == selection.startX ||
                    isBlockOpen(DK_GetBlockAt(map_x - 1, map_y))) {
                indices[0] = fi(x * 2, y * 2, 4);
                indices[1] = fi(x * 2, y * 2 + 1, 4);
                indices[2] = fi(x * 2, y * 2 + 2, 4);
                glDrawElements(GL_LINE_STRIP, 3, GL_UNSIGNED_INT, &indices);

                indices[0] = fi(x * 2, y * 2, 2);
                indices[1] = fi(x * 2, y * 2 + 1, 2);
                indices[2] = fi(x * 2, y * 2 + 2, 2);
                glDrawElements(GL_LINE_STRIP, 3, GL_UNSIGNED_INT, &indices);
            }
        }
    }

    // Reset stuff.
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);

    endDraw();
}

static void renderSelectionOverlay(void) {
    const vec3* camera = DK_GetCameraPosition();
    const int x_begin = (int) (camera->v[0] / DK_BLOCK_SIZE) - DK_RENDER_AREA_X / 2;
    const int y_begin = (int) (camera->v[1] / DK_BLOCK_SIZE) - DK_RENDER_AREA_Y_OFFSET;
    const int x_end = x_begin + DK_RENDER_AREA_X;
    const int y_end = y_begin + DK_RENDER_AREA_Y;

    beginDraw();

    // Set up coloring.
    DK_InitMaterial(&gMaterial);
    setDiffuseColor(DK_MAP_SELECTED_COLOR_R,
            DK_MAP_SELECTED_COLOR_G,
            DK_MAP_SELECTED_COLOR_B,
            DK_MAP_SELECTED_COLOR_A);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glDepthMask(GL_FALSE);

    for (int x = x_begin; x < x_end; ++x) {
        for (int y = y_begin; y < y_end; ++y) {
            // Selected by the local player?
            if (x >= 0 && y >= 0 && x < gMapSize && y < gMapSize &&
                    DK_IsBlockSelected(DK_PLAYER_ONE, x, y)) {
                DK_PushModelMatrix();
                DK_TranslateModelMatrix(0, 0, DK_MAP_SELECTION_OFFSET);

                // Draw top.
                drawTop(x, y, 4);

                // North wall.
                if (y + 1 < gMapSize && isBlockOpen(DK_GetBlockAt(x, y + 1))) {
                    DK_PushModelMatrix();
                    DK_TranslateModelMatrix(0, DK_MAP_SELECTION_OFFSET, 0);
                    drawNorth(x, y + 1, 2);
                    DK_PopModelMatrix();
                }

                // South wall.
                if (y > 0 && isBlockOpen(DK_GetBlockAt(x, y - 1))) {
                    DK_PushModelMatrix();
                    DK_TranslateModelMatrix(0, -DK_MAP_SELECTION_OFFSET, 0);
                    drawSouth(x, y, 2);
                    DK_PopModelMatrix();
                }

                // West wall.
                if (x > 0 && isBlockOpen(DK_GetBlockAt(x - 1, y))) {
                    DK_PushModelMatrix();
                    DK_TranslateModelMatrix(-DK_MAP_SELECTION_OFFSET, 0, 0);
                    drawWest(x, y, 2);
                    DK_PopModelMatrix();
                }

                // East wall.
                if (x + 1 < gMapSize && isBlockOpen(DK_GetBlockAt(x + 1, y))) {

                    DK_PushModelMatrix();
                    DK_TranslateModelMatrix(DK_MAP_SELECTION_OFFSET, 0, 0);
                    drawEast(x + 1, y, 2);
                    DK_PopModelMatrix();
                }
                DK_PopModelMatrix();
            }
        }
    }

    endDraw();

    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
}

static void onRender(void) {
    const vec3* camera = DK_GetCameraPosition();
    const int x_begin = (int) (camera->v[0] / DK_BLOCK_SIZE) - DK_RENDER_AREA_X / 2;
    const int y_begin = (int) (camera->v[1] / DK_BLOCK_SIZE) - DK_RENDER_AREA_Y_OFFSET;
    const int x_end = x_begin + DK_RENDER_AREA_X;
    const int y_end = y_begin + DK_RENDER_AREA_Y;

    const DK_BlockMeta* defaultMeta = DK_GetBlockMeta(1);
    const DK_BlockMeta* meta;
    DK_Player owner;

    beginDraw();

    for (int x = x_begin; x < x_end; ++x) {
        for (int y = y_begin; y < y_end; ++y) {
            DK_Block* block = DK_GetBlockAt(x, y);

            // Mark for select mode, coordinates of that block.
            glLoadName(((unsigned short) y << 16) | (unsigned short) x);

            // Get block info.
            if (block) {
                meta = block->meta;
                owner = block->owner;
            } else {
                // Out of bounds, use default block type.
                meta = defaultMeta;
                owner = DK_PLAYER_NONE;
            }

            // Set up material.
            DK_InitMaterial(&gMaterial);
            pushTexture(x, y, meta->level, meta->textures[meta->level][DK_BLOCK_TEXTURE_TOP]);
            if (owner != DK_PLAYER_NONE) {
                // Check surroundings for blocks of the same type, to know which
                // overlay to show. We first check the four direct neighbors
                // (not checking diagonally yet).
                //  ? | ? | ? | ? | ?
                // ---+---+---+---+---
                //  ? |   | t |   | ?
                // ---+---+---+---+---
                //  ? | l |   | r | ?
                // ---+---+---+---+---
                //  ? |   | b |   | ?
                // ---+---+---+---+---
                //  ? | ? | ? | ? | ?
                // Any edge that is marked thus will determine which texture we
                // use as the overlay.

                typedef enum Edges {
                    EDGE_NONE = 0,
                    EDGE_TOP = 1,
                    EDGE_RIGHT = 2,
                    EDGE_BOTTOM = 4,
                    EDGE_LEFT = 8
                } Edges;

                Edges edges = EDGE_NONE;
                DK_Block* b;
                if ((b = DK_GetBlockAt(x, y + 1)) &&
                        b->meta->id == meta->id &&
                        b->owner == owner) {
                    edges |= EDGE_TOP;
                }
                if ((b = DK_GetBlockAt(x + 1, y)) &&
                        b->meta->id == meta->id &&
                        b->owner == owner) {
                    edges |= EDGE_RIGHT;
                }
                if ((b = DK_GetBlockAt(x, y - 1)) &&
                        b->meta->id == meta->id &&
                        b->owner == owner) {
                    edges |= EDGE_BOTTOM;
                }
                if ((b = DK_GetBlockAt(x - 1, y)) &&
                        b->meta->id == meta->id &&
                        b->owner == owner) {
                    edges |= EDGE_LEFT;
                }

                if (edges != EDGE_NONE) {

                }

                // Finally, the marker of the player's color on top of it all.
                pushTexture(x, y, meta->level, meta->textures[meta->level][DK_BLOCK_TEXTURE_TOP_OWNED_OVERLAY]);
            }

            // Render top element.
            drawTop(x, y, (meta->level - 1) * 2);

            // Check for walling, for this level and the lower ones.
            for (DK_BlockLevel level = meta->level; level < DK_BLOCK_LEVEL_HIGH; ++level) {
                // North wall.
                if ((block = DK_GetBlockAt(x, y + 1))) {
                    meta = block->meta;
                    owner = block->owner;
                } else {
                    meta = defaultMeta;
                    owner = DK_PLAYER_NONE;
                }
                if (meta->level > level) {
                    // Set up material.
                    DK_InitMaterial(&gMaterial);
                    pushTexture(x, y + 1, level, meta->textures[level][DK_BLOCK_TEXTURE_SIDE]);

                    // Render.
                    drawSouth(x, y + 1, (level - 1) * 2);
                }

                // South wall.
                if ((block = DK_GetBlockAt(x, y - 1))) {
                    meta = block->meta;
                    owner = block->owner;
                } else {
                    meta = defaultMeta;
                    owner = DK_PLAYER_NONE;
                }
                if (meta->level > level) {
                    // Set up material.
                    DK_InitMaterial(&gMaterial);
                    pushTexture(x, y - 1, level, meta->textures[level][DK_BLOCK_TEXTURE_SIDE]);

                    // Render.
                    drawNorth(x, y, (level - 1) * 2);
                }

                // West wall.
                if ((block = DK_GetBlockAt(x - 1, y))) {
                    meta = block->meta;
                    owner = block->owner;
                } else {
                    meta = defaultMeta;
                    owner = DK_PLAYER_NONE;
                }
                if (meta->level > level) {
                    // Set up material.
                    DK_InitMaterial(&gMaterial);
                    pushTexture(x - 1, y, level, meta->textures[level][DK_BLOCK_TEXTURE_SIDE]);

                    // Render.
                    drawEast(x, y, (level - 1) * 2);
                }

                // East wall.
                if ((block = DK_GetBlockAt(x + 1, y))) {
                    meta = block->meta;
                    owner = block->owner;
                } else {
                    meta = defaultMeta;
                    owner = DK_PLAYER_NONE;
                }
                if (meta->level > level) {
                    // Set up material.
                    DK_InitMaterial(&gMaterial);
                    pushTexture(x + 1, y, level, meta->textures[level][DK_BLOCK_TEXTURE_SIDE]);

                    // Render.
                    drawWest(x + 1, y, (level - 1) * 2);
                }
            }
        }
    }

    endDraw();
}

///////////////////////////////////////////////////////////////////////////////
// Updating
///////////////////////////////////////////////////////////////////////////////

static void onPreRender(void) {
    int mouse_x, mouse_y;
    GLuint selected_name;

    SDL_GetMouseState(&mouse_x, &mouse_y);

    gIsPicking = 1;
    selected_name = DK_Pick(mouse_x, DK_resolution_y - mouse_y, &onRender);
    gIsPicking = 0;

    gCursorX = (short) (selected_name & 0xFFFF);
    gCursorY = (short) (selected_name >> 16);

    gHandLight.position.d.x = DK_GetCursor(DK_CURSOR_LEVEL_TOP)->v[0];
    gHandLight.position.d.y = DK_GetCursor(DK_CURSOR_LEVEL_TOP)->v[1];
    gHandLight.position.d.z = DK_HAND_LIGHT_HEIGHT;
}

///////////////////////////////////////////////////////////////////////////////
// Header implementation
///////////////////////////////////////////////////////////////////////////////

void DK_SetMapSize(unsigned short size) {
    // Reallocate data only if the size changed.
    if (size != gMapSize) {
        // Free old map data.
        free(gMap);

        // Allocate new map data.
        if (!(gMap = calloc(size * size, sizeof (DK_Block)))) {
            fprintf(stderr, "Out of memory while allocating map data.\n");
            exit(EXIT_FAILURE);
        }

        // Free old map model data.
        free(gVertices);

        // Allocate new map model data.
        gVerticesPerDimension = (size + DK_MAP_BORDER) * 2 + 1;
        gVertices = calloc(gVerticesPerDimension * gVerticesPerDimension * 5, sizeof (struct Vertex));

        if (!gVertices) {
            fprintf(stderr, "Out of memory while allocating map model data.\n");
            exit(EXIT_FAILURE);
        }

        DK_GL_DeleteMap();
    }

    // Remember new map size (needed *now* for offset computation).
    gMapSize = size;

    // Update whatever depends on the map size.
    CB_Call(gMapSizeChangeCallbacks);

    // Initialize map data.
    for (unsigned int x = 0; x < size; ++x) {
        for (unsigned int y = 0; y < size; ++y) {
            DK_Block* block = DK_GetBlockAt(x, y);
            block->meta = DK_GetBlockMeta(1);
            block->durability = block->meta->durability;
            block->strength = block->meta->strength;
            block->gold = block->meta->gold;
            block->room = NULL;
        }
    }

    // Initialize map model data.

    // Pass 1: set vertex positions.
    for (unsigned int x = 0; x < gVerticesPerDimension; ++x) {
        for (unsigned int y = 0; y < gVerticesPerDimension; ++y) {
            // Set actual vertex position, applying noise and such.
            updateVerticesAt(x, y);

            for (unsigned int z = 0; z < 5; ++z) {
                struct Vertex* v = &gVertices[fi(x, y, z)];

                // Initialize normals to defaults (especially the border cases, i.e.
                // at 0 and max, because those aren't computed dynamically to avoid
                // having to check the border cases).
                v->normal[SIDE_TOP].d.x = 0;
                v->normal[SIDE_TOP].d.y = 0;
                v->normal[SIDE_TOP].d.z = 1;
                v->normal[SIDE_NORTH].d.x = 0;
                v->normal[SIDE_NORTH].d.y = 1;
                v->normal[SIDE_NORTH].d.z = 0;
                v->normal[SIDE_SOUTH].d.x = 0;
                v->normal[SIDE_SOUTH].d.y = -1;
                v->normal[SIDE_SOUTH].d.z = 0;
                v->normal[SIDE_EAST].d.x = 1;
                v->normal[SIDE_EAST].d.y = 0;
                v->normal[SIDE_EAST].d.z = 0;
                v->normal[SIDE_WEST].d.x = -1;
                v->normal[SIDE_WEST].d.y = 0;
                v->normal[SIDE_WEST].d.z = 0;

                // Set texture coordinate.
                v->texCoord[SIDE_TOP].t.u = x / 2.0f;
                v->texCoord[SIDE_TOP].t.v = (gVerticesPerDimension - 1 - y) / 2.0f;
                v->texCoord[SIDE_NORTH].t.u = (gVerticesPerDimension - 1 - x) / 2.0f;
                v->texCoord[SIDE_NORTH].t.v = (4 - z) / 2.0f;
                v->texCoord[SIDE_SOUTH].t.u = x / 2.0f;
                v->texCoord[SIDE_SOUTH].t.v = (4 - z) / 2.0f;
                v->texCoord[SIDE_EAST].t.u = y / 2.0f;
                v->texCoord[SIDE_EAST].t.v = (4 - z) / 2.0f;
                v->texCoord[SIDE_WEST].t.u = (gVerticesPerDimension - 1 - y) / 2.0f;
                v->texCoord[SIDE_WEST].t.v = (4 - z) / 2.0f;
            }
        }
    }

    // Pass 2: compute normals.
    for (unsigned int x = 1; x < gVerticesPerDimension - 1; ++x) {
        for (unsigned int y = 1; y < gVerticesPerDimension - 1; ++y) {
            updateNormalsAt(x, y);
        }
    }

    DK_GL_GenerateMap();
}

unsigned short DK_GetMapSize(void) {
    return gMapSize;
}

void DK_GL_GenerateMap(void) {
    if (!gVertexBufferID) {
        glGenBuffers(1, &gVertexBufferID);
    }
    glBindBuffer(GL_ARRAY_BUFFER, gVertexBufferID);

    glBufferData(GL_ARRAY_BUFFER, gVerticesPerDimension * gVerticesPerDimension * 5 * sizeof (struct Vertex), gVertices, GL_DYNAMIC_DRAW);
    gShouldUpdateVertexBuffer = 1;

    EXIT_ON_OPENGL_ERROR();
}

void DK_GL_DeleteMap(void) {
    if (gVertexBufferID) {
        glDeleteBuffers(1, &gVertexBufferID);
        gVertexBufferID = 0;

        EXIT_ON_OPENGL_ERROR();
    }
    gShouldUpdateVertexBuffer = 0;
}

void DK_InitMap(void) {
    gHandLight.diffuseColor.c.r = DK_HAND_LIGHT_COLOR_R;
    gHandLight.diffuseColor.c.g = DK_HAND_LIGHT_COLOR_G;
    gHandLight.diffuseColor.c.b = DK_HAND_LIGHT_COLOR_B;
    gHandLight.diffusePower = DK_HAND_LIGHT_POWER;
    gHandLight.specularColor.c.r = DK_HAND_LIGHT_COLOR_R;
    gHandLight.specularColor.c.g = DK_HAND_LIGHT_COLOR_G;
    gHandLight.specularColor.c.b = DK_HAND_LIGHT_COLOR_B;
    gHandLight.specularPower = DK_HAND_LIGHT_POWER;
    DK_AddLight(&gHandLight);

    DK_OnPreRender(onPreRender);
    DK_OnRender(onRender);
    DK_OnPostRender(renderSelectionOverlay);
    DK_OnPostRender(renderSelectionOutline);
}

///////////////////////////////////////////////////////////////////////////////
// Accessors
///////////////////////////////////////////////////////////////////////////////

DK_Block * DK_GetBlockAt(int x, int y) {
    if (x >= 0 && y >= 0 && x < gMapSize && y < gMapSize) {
        return &gMap[y * gMapSize + x];
    }
    return NULL;
}

int DK_GetBlockCoordinates(unsigned short* x, unsigned short* y, const DK_Block * block) {
    if (block) {
        unsigned int idx = block - gMap;
        *x = idx % gMapSize;
        *y = idx / gMapSize;

        return 1;
    }
    return 0;
}

DK_Block * DK_GetBlockUnderCursor(int* x, int* y) {
    if (x) {
        *x = gCursorX;
    }
    if (y) {
        *y = gCursorY;
    }
    return DK_GetBlockAt(gCursorX, gCursorY);
}

int DK_DamageBlock(DK_Block* block, unsigned int damage) {
    // Already destroyed (nothing to do)?
    if (block->durability <= 0) {
        return 1;
    }

    // Check if this is the final blow.
    if (block->durability > damage) {
        block->durability -= damage;
        return 0;
    }

    // Block is destroyed.
    block->meta = block->meta->becomes;
    block->durability = block->meta->durability;
    block->strength = block->meta->strength;
    block->gold = block->meta->gold;
    block->owner = DK_PLAYER_NONE;

    // Update visual representation of the surroundings.
    updateBlock(block);

    return 1;
}

int DK_ConvertBlock(DK_Block* block, unsigned int strength, DK_Player player) {
    // Get the actual coordinates.
    unsigned short x = 0, y = 0;
    DK_GetBlockCoordinates(&x, &y, block);

    // First reduce any enemy influence.
    if (block->owner != player) {
        // Not this player's, reduce strength.
        if (block->strength > strength) {
            block->strength -= strength;
            return 0;
        }

        // Block is completely converted.
        DK_SetBlockOwner(block, player);

        // See if we're now cornering a neutral block. In that case we
        // automatically convert that one, too.
        //
        // +---+---+---...
        // | o | b |
        // +---+---+---...
        // | b |   |
        // +---+---+---...
        // ...
        //
        // We must be one of the three marked blocks, and the ones marked 'b'
        // must be walls, the one marked be 'o' must be an open tile. An that
        // in all possible orientations.
        if (block->meta->level == DK_BLOCK_LEVEL_HIGH) {
            // We're one of the 'b' blocks (possibly). Check our surroundings.
            for (int nx = -1; nx < 2; nx += 2) {
                for (int ny = -1; ny < 2; ny += 2) {
                    // This double loop gives us the coordinates of our diagonal
                    // neighbors, meaning they have to be owned blocks.
                    if ((block = DK_GetBlockAt(x + nx, y + ny)) &&
                            block->owner == player &&
                            block->meta->level == DK_BLOCK_LEVEL_HIGH &&
                            // OK so far, now wee need to check if one of the
                            // blocks completing the square is open and the
                            // other un-owned.
                            ((
                            (block = DK_GetBlockAt(x, y + ny)) &&
                            block->owner == player &&
                            block->meta->level < DK_BLOCK_LEVEL_HIGH &&
                            (block = DK_GetBlockAt(x + nx, y)) &&
                            block->owner == DK_PLAYER_NONE &&
                            block->meta->level == DK_BLOCK_LEVEL_HIGH
                            ) || (
                            // If it didn't work that way around, check the
                            // other way.
                            (block = DK_GetBlockAt(x + nx, y)) &&
                            block->owner == player &&
                            block->meta->level < DK_BLOCK_LEVEL_HIGH &&
                            (block = DK_GetBlockAt(x, y + ny)) &&
                            block->owner == DK_PLAYER_NONE &&
                            block->meta->level == DK_BLOCK_LEVEL_HIGH
                            ))) {
                        // All conditions are met for converting the block.
                        DK_SetBlockOwner(block, player);
                    }
                }
            }
        } else {
            // We're on a lower level tile, check the diagonal for un-owned high
            // blocks, and the inbetweens for owned high blocks.
            for (int nx = -1; nx < 2; nx += 2) {
                for (int ny = -1; ny < 2; ny += 2) {
                    // This double loop gives us the coordinates of our diagonal
                    // neighbors, meaning they have to be un-owned blocks.
                    if ((block = DK_GetBlockAt(x + nx, y + ny)) &&
                            block->owner == DK_PLAYER_NONE &&
                            block->meta->level == DK_BLOCK_LEVEL_HIGH &&
                            // OK so far, now wee need to check if the blocks
                            // completing the square are high and owned.
                            (block = DK_GetBlockAt(x, y + ny)) &&
                            block->owner == player &&
                            block->meta->level == DK_BLOCK_LEVEL_HIGH &&
                            (block = DK_GetBlockAt(x + nx, y)) &&
                            block->owner == player &&
                            block->meta->level == DK_BLOCK_LEVEL_HIGH) {
                        // All conditions are met for converting the block.
                        DK_SetBlockOwner(block, player);
                    }
                }
            }
        }
    } else {
        // Owned by this player, repair it.
        const unsigned int max_strength = block->meta->strength;
        block->strength += strength;
        if (block->strength < max_strength) {
            return 0;
        }

        // Completely repaired.
        block->strength = max_strength;

        // Update jobs nearby.
        DK_UpdateJobsForBlock(player, x, y);
    }
    return 1;
}

void DK_SetBlockMeta(DK_Block* block, const DK_BlockMeta* meta) {
    if (!block || !meta) {
        return;
    }

    block->meta = meta;

    // Update values.
    block->durability = block->meta->durability;
    block->strength = block->meta->strength;
    block->gold = block->meta->gold;

    updateBlock(block);
}

void DK_SetBlockOwner(DK_Block* block, DK_Player player) {
    if (!block) {
        return;
    }

    block->owner = player;

    // Update values.
    block->durability = block->meta->durability;
    block->strength = block->meta->strength;

    updateBlock(block);
}

///////////////////////////////////////////////////////////////////////////////
// Events
///////////////////////////////////////////////////////////////////////////////

void DK_OnMapSizeChange(callback method) {
    if (!gMapSizeChangeCallbacks) {
        gMapSizeChangeCallbacks = CB_New();
    }
    CB_Add(gMapSizeChangeCallbacks, method);
}
