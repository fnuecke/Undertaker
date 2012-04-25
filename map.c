#include <math.h>
#include <string.h>
#include <malloc.h>

#include <SDL/SDL.h>
#include <GL/glew.h>

#include "astar.h"
#include "bitset.h"
#include "camera.h"
#include "config.h"
#include "jobs.h"
#include "map.h"
#include "picking.h"
#include "render.h"
#include "selection.h"
#include "simplexnoise.h"
#include "textures.h"
#include "units.h"

///////////////////////////////////////////////////////////////////////////////
// Macros
///////////////////////////////////////////////////////////////////////////////

#define XYZD2I(x, y, z, dx, dy) (((z) * (dx) * (dy)) + (y) * (dy) + (x))
#define XYZ2IF(x, y, z) XYZD2I(x, y, z, vertices_count_full, vertices_count_full)
#define XYZ2IH(x, y, z) XYZD2I(x, y, z, vertices_count_full, vertices_count_semi)

#define DK_NEQ(a, b) (fabs(b - (a + 0.5)) < 0.01f)
#define DK_NLT(a, b) (b - (a + 0.5) > 0)

///////////////////////////////////////////////////////////////////////////////
// Internal variables
///////////////////////////////////////////////////////////////////////////////

/** The current map */
static DK_Block* map = 0;

/** Current map size */
unsigned short DK_map_size = 0;

/** Currently hovered block coordinates */
static int cursor_x = 0, cursor_y = 0;

/** Currently doing a picking (select) render pass? */
static char picking = 0;

///////////////////////////////////////////////////////////////////////////////
// Map model data
///////////////////////////////////////////////////////////////////////////////

/** Vector type */
typedef struct {
    GLfloat x, y, z;
} v3f;

/** Vertices making up the map model */
static v3f* vertices = 0;

/** Normals at a specific vertex, for walls and toppings separately */
static v3f* normals_x = 0;
static v3f* normals_y = 0;
static v3f* normals_z = 0;

/** Number of vertices in x and y direction */
static unsigned int vertices_count_full = 0;
static unsigned int vertices_count_semi = 0;

// Base z coordinates for vertices.
static const float z_coords[5] = {
    -DK_WATER_LEVEL,
    -DK_WATER_LEVEL / 2.0f,
    0,
    DK_BLOCK_HEIGHT / 2.0f,
    DK_BLOCK_HEIGHT
};

///////////////////////////////////////////////////////////////////////////////
// Picking
///////////////////////////////////////////////////////////////////////////////

inline static GLuint pick_block(int x, int y) {
    GLuint result;
    picking = 1;
    result = DK_pick(x, y, &DK_render_map);
    picking = 0;
    return result;
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
static double noise_factor_block(DK_Offset* offset, const DK_Block* block) {
    *offset = OFFSET_NONE;
    if (DK_block_is_open(block) && block->owner == DK_PLAYER_NONE) {
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
static float compute_offset(float* offset, float x, float y) {
    DK_Offset offset_type;
    float offset_reduction[2] = {1, 1};

    // Get full coordinate for our doubles (which might be at half coordinates.
    const int i = (int) (x + 0.1f), j = (int) (y + 0.1f);

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
    int x_end, y_end, k, l;
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
    if (x_end > DK_map_size - 1) {
        x_end = DK_map_size - 1;
    }
    if (y_begin < 0) {
        y_begin = 0;
    }
    if (y_end > DK_map_size - 1) {
        y_end = DK_map_size - 1;
    }

    // Walk all the block necessary.
    factor = 0;
    for (k = x_begin; k <= x_end; ++k) {
        for (l = y_begin; l <= y_end; ++l) {
            factor += noise_factor_block(&offset_type, &map[k + l * DK_map_size]);
            if (offset_type == OFFSET_INCREASE) {
                if (!DK_NEQ(k, x)) {
                    if (DK_NLT(k, x)) {
                        offset[0] += 1;
                    } else if (k >= i) {
                        offset[0] -= 1;
                    }
                }
                if (!DK_NEQ(l, y)) {
                    if (DK_NLT(l, y)) {
                        offset[1] += 1;
                    } else if (l >= j) {
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

static void update_vertices(int x, int y) {
    unsigned int z;
    for (z = 0; z < 5; ++z) {
        v3f* v = &vertices[XYZ2IF(x, y, z)];
        const float vx = (x - DK_MAP_BORDER) / 2.0f * DK_BLOCK_SIZE;
        const float vy = (y - DK_MAP_BORDER) / 2.0f * DK_BLOCK_SIZE;
        const float vz = z_coords[z];
#if DK_D_TERRAIN_NOISE
        float offset[2] = {0, 0};
        float factor = 1.0f;
        float offset_factor;
#if DK_D_USE_NOISE_OFFSET
        factor = compute_offset(offset, (x - DK_MAP_BORDER) / 2.0f, (y - DK_MAP_BORDER) / 2.0f);
        offset_factor = (0.25f - (fabs(fabs(z / 4.0f - 0.5f) - 0.25f) - 0.25f)) * z - 0.5f;
        offset[0] *= offset_factor;
        offset[1] *= offset_factor;
#endif
        v->x = vx + factor * snoise4(vx, vy, vz, 0) + offset[0];
        v->y = vy + factor * snoise4(vx, vy, vz, 1) + offset[1];
        v->z = vz + snoise4(vx, vy, vz, 2);
#else
        v->x = vx;
        v->y = vy;
        v->z = vz;
#endif
    }
}

///////////////////////////////////////////////////////////////////////////////
// Map model updating: normals
///////////////////////////////////////////////////////////////////////////////

inline static void crossv3f(v3f* cross, const v3f* a, const v3f* b) {
    cross->x = a->y * b->z - a->z * b->y;
    cross->y = a->z * b->x - a->x * b->z;
    cross->z = a->x * b->y - a->y * b->x;
}

inline static void iaddv3f(v3f* a, const v3f* b) {
    a->x += b->x;
    a->y += b->y;
    a->z += b->z;
}

inline static void subv3f(v3f* sub, const v3f* a, const v3f* b) {
    sub->x = a->x - b->x;
    sub->y = a->y - b->y;
    sub->z = a->z - b->z;
}

inline static float lenv3f(const v3f* v) {
    return sqrtf(v->x * v->x + v->y * v->y + v->z * v->z);
}

inline static void idivsv3f(v3f* v, float s) {
    v->x /= s;
    v->y /= s;
    v->z /= s;
}

static void normalv3f(v3f* normal, const v3f* v0, const v3f* v1, const v3f* v2) {
    v3f a, b;
    subv3f(&a, v1, v0);
    subv3f(&b, v2, v0);
    crossv3f(normal, &a, &b);
}

static void interpolate_normal(v3f* n, const v3f* v0,
        const v3f* v1, const v3f* v2, const v3f* v3, const v3f* v4,
        const v3f* v5, const v3f* v6, const v3f* v7, const v3f* v8) {
    // Compute normals based on neighbors and accumulate.
    v3f tmp;
    n->x = 0;
    n->y = 0;
    n->z = 0;
    normalv3f(&tmp, v0, v1, v2);
    iaddv3f(n, &tmp);
    normalv3f(&tmp, v0, v2, v3);
    iaddv3f(n, &tmp);
    normalv3f(&tmp, v0, v3, v4);
    iaddv3f(n, &tmp);
    normalv3f(&tmp, v0, v4, v5);
    iaddv3f(n, &tmp);
    normalv3f(&tmp, v0, v5, v6);
    iaddv3f(n, &tmp);
    normalv3f(&tmp, v0, v6, v7);
    iaddv3f(n, &tmp);
    normalv3f(&tmp, v0, v7, v8);
    iaddv3f(n, &tmp);
    normalv3f(&tmp, v0, v8, v1);
    iaddv3f(n, &tmp);

    // Then normalize.
    idivsv3f(n, lenv3f(n));
}

static void update_normals(int x, int y) {
    unsigned int z;
    for (z = 1; z < 4; ++z) {
        if ((x & 1) == 0) {
            // Compute x normals.
            v3f* n = &normals_x[XYZ2IH(x / 2, y, z)];

            // Vertex the normal sits on.
            const v3f* v0 = &vertices[XYZ2IF(x, y, z)];

            // Neighboring vertices.
            const v3f* v1 = &vertices[XYZ2IF(x, y - 1, z + 1)];
            const v3f* v2 = &vertices[XYZ2IF(x, y - 1, z)];
            const v3f* v3 = &vertices[XYZ2IF(x, y - 1, z - 1)];
            const v3f* v4 = &vertices[XYZ2IF(x, y, z - 1)];
            const v3f* v5 = &vertices[XYZ2IF(x, y + 1, z - 1)];
            const v3f* v6 = &vertices[XYZ2IF(x, y + 1, z)];
            const v3f* v7 = &vertices[XYZ2IF(x, y + 1, z + 1)];
            const v3f* v8 = &vertices[XYZ2IF(x, y, z + 1)];

            // Compute normals based on neighbors and accumulate.
            interpolate_normal(n, v0, v1, v2, v3, v4, v5, v6, v7, v8);
        }

        if ((y & 1) == 0) {
            // Compute y normals.
            v3f* n = &normals_y[XYZ2IH(x, y / 2, z)];

            // Vertex the normal sits on.
            const v3f* v0 = &vertices[XYZ2IF(x, y, z)];

            // Neighboring vertices.
            const v3f* v1 = &vertices[XYZ2IF(x + 1, y, z + 1)];
            const v3f* v2 = &vertices[XYZ2IF(x + 1, y, z)];
            const v3f* v3 = &vertices[XYZ2IF(x + 1, y, z - 1)];
            const v3f* v4 = &vertices[XYZ2IF(x, y, z - 1)];
            const v3f* v5 = &vertices[XYZ2IF(x - 1, y, z - 1)];
            const v3f* v6 = &vertices[XYZ2IF(x - 1, y, z)];
            const v3f* v7 = &vertices[XYZ2IF(x - 1, y, z + 1)];
            const v3f* v8 = &vertices[XYZ2IF(x, y, z + 1)];

            // Compute normals based on neighbors and accumulate.
            interpolate_normal(n, v0, v1, v2, v3, v4, v5, v6, v7, v8);
        }
    }

    for (z = 0; z < 5; z += 2) {
        // Compute z normals.
        v3f* n = &normals_z[XYZ2IF(x, y, z / 2)];

        // Vertex the normal sits on.
        const v3f* v0 = &vertices[XYZ2IF(x, y, z)];

        // Neighboring vertices.
        const v3f* v1 = &vertices[XYZ2IF(x, y - 1, z)];
        const v3f* v2 = &vertices[XYZ2IF(x + 1, y - 1, z)];
        const v3f* v3 = &vertices[XYZ2IF(x + 1, y, z)];
        const v3f* v4 = &vertices[XYZ2IF(x + 1, y + 1, z)];
        const v3f* v5 = &vertices[XYZ2IF(x, y + 1, z)];
        const v3f* v6 = &vertices[XYZ2IF(x - 1, y + 1, z)];
        const v3f* v7 = &vertices[XYZ2IF(x - 1, y, z)];
        const v3f* v8 = &vertices[XYZ2IF(x - 1, y - 1, z)];

        // Compute normals based on neighbors and accumulate.
        interpolate_normal(n, v0, v1, v2, v3, v4, v5, v6, v7, v8);
    }
}

///////////////////////////////////////////////////////////////////////////////
// Block updating
///////////////////////////////////////////////////////////////////////////////

static void update_block(DK_Block* block) {
    unsigned short x, y;

    // Ignore requests for invalid blocks.
    if (!block) {
        return;
    }

    if (!DK_block_coordinates(&x, &y, block)) {
        // Out of bounds, ignore.
        return;
    }

    // Update values.
    if (block->owner == DK_PLAYER_NONE) {
        if (block->type == DK_BLOCK_DIRT) {
            block->strength = DK_BLOCK_DIRT_STRENGTH;
            block->health = DK_BLOCK_DIRT_HEALTH;
        } else if (block->type == DK_BLOCK_NONE) {
            block->strength = DK_BLOCK_NONE_STRENGTH;
        }
    } else {
        if (block->type == DK_BLOCK_DIRT) {
            block->strength = DK_BLOCK_DIRT_OWNED_STRENGTH;
            block->health = DK_BLOCK_DIRT_HEALTH;
        } else if (block->type == DK_BLOCK_NONE) {
            block->strength = DK_BLOCK_NONE_OWNED_STRENGTH;
        }
    }

    // Update model.
    {
        int start_x = x * 2 - 1 + DK_MAP_BORDER;
        int start_y = y * 2 - 1 + DK_MAP_BORDER;
        int end_x = x * 2 + 3 + DK_MAP_BORDER;
        int end_y = y * 2 + 3 + DK_MAP_BORDER;

        int lx, ly;
        for (lx = start_x; lx < end_x; ++lx) {
            for (ly = start_y; ly < end_y; ++ly) {
                update_vertices(lx, ly);
            }
        }

        for (lx = start_x; lx < end_x; ++lx) {
            for (ly = start_y; ly < end_y; ++ly) {
                update_normals(lx, ly);
            }
        }
    }

    // Update jobs for this block by deselecting it.
    {
        int i;
        for (i = 0; i < DK_PLAYER_COUNT; ++i) {
            DK_block_deselect(i, x, y);
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
// Map model rendering
///////////////////////////////////////////////////////////////////////////////

static void set_texture(int x, int y, unsigned int z, DK_Texture texture) {
    unsigned int variation = (unsigned int) ((snoise2(x, y + z) + 1) / 2 * DK_TEX_MAX_VARIATIONS);
    DK_Material material;
    DK_material_init(&material);
    material.textures[0] = DK_opengl_texture(texture, variation);
    material.texture_count = 1;
    DK_render_set_material(&material);
}

static void draw_top(int x, int y, unsigned int z, DK_Texture texture) {
    unsigned int idx;

    x += DK_MAP_BORDER / 2;
    y += DK_MAP_BORDER / 2;

    set_texture(x, y, z, texture);

    glBegin(GL_QUAD_STRIP);
    {
        glTexCoord2d(0, 0);
        idx = XYZ2IF(x * 2, y * 2 + 2, z / 2);
        glNormal3f(normals_z[idx].x, normals_z[idx].y, normals_z[idx].z);
        idx = XYZ2IF(x * 2, y * 2 + 2, z);
        glVertex3f(vertices[idx].x, vertices[idx].y, vertices[idx].z);

        glTexCoord2d(0, 0.5);
        idx = XYZ2IF(x * 2, y * 2 + 1, z / 2);
        glNormal3f(normals_z[idx].x, normals_z[idx].y, normals_z[idx].z);
        idx = XYZ2IF(x * 2, y * 2 + 1, z);
        glVertex3f(vertices[idx].x, vertices[idx].y, vertices[idx].z);

        glTexCoord2d(0.5, 0);
        idx = XYZ2IF(x * 2 + 1, y * 2 + 2, z / 2);
        glNormal3f(normals_z[idx].x, normals_z[idx].y, normals_z[idx].z);
        idx = XYZ2IF(x * 2 + 1, y * 2 + 2, z);
        glVertex3f(vertices[idx].x, vertices[idx].y, vertices[idx].z);

        glTexCoord2d(0.5, 0.5);
        idx = XYZ2IF(x * 2 + 1, y * 2 + 1, z / 2);
        glNormal3f(normals_z[idx].x, normals_z[idx].y, normals_z[idx].z);
        idx = XYZ2IF(x * 2 + 1, y * 2 + 1, z);
        glVertex3f(vertices[idx].x, vertices[idx].y, vertices[idx].z);

        glTexCoord2d(1, 0);
        idx = XYZ2IF(x * 2 + 2, y * 2 + 2, z / 2);
        glNormal3f(normals_z[idx].x, normals_z[idx].y, normals_z[idx].z);
        idx = XYZ2IF(x * 2 + 2, y * 2 + 2, z);
        glVertex3f(vertices[idx].x, vertices[idx].y, vertices[idx].z);

        glTexCoord2d(1, 0.5);
        idx = XYZ2IF(x * 2 + 2, y * 2 + 1, z / 2);
        glNormal3f(normals_z[idx].x, normals_z[idx].y, normals_z[idx].z);
        idx = XYZ2IF(x * 2 + 2, y * 2 + 1, z);
        glVertex3f(vertices[idx].x, vertices[idx].y, vertices[idx].z);
    }
    glEnd();

    glBegin(GL_QUAD_STRIP);
    {
        glTexCoord2d(0, 0.5);
        idx = XYZ2IF(x * 2, y * 2 + 1, z / 2);
        glNormal3f(normals_z[idx].x, normals_z[idx].y, normals_z[idx].z);
        idx = XYZ2IF(x * 2, y * 2 + 1, z);
        glVertex3f(vertices[idx].x, vertices[idx].y, vertices[idx].z);

        glTexCoord2d(0, 1);
        idx = XYZ2IF(x * 2, y * 2, z / 2);
        glNormal3f(normals_z[idx].x, normals_z[idx].y, normals_z[idx].z);
        idx = XYZ2IF(x * 2, y * 2, z);
        glVertex3f(vertices[idx].x, vertices[idx].y, vertices[idx].z);

        glTexCoord2d(0.5, 0.5);
        idx = XYZ2IF(x * 2 + 1, y * 2 + 1, z / 2);
        glNormal3f(normals_z[idx].x, normals_z[idx].y, normals_z[idx].z);
        idx = XYZ2IF(x * 2 + 1, y * 2 + 1, z);
        glVertex3f(vertices[idx].x, vertices[idx].y, vertices[idx].z);

        glTexCoord2d(0.5, 1);
        idx = XYZ2IF(x * 2 + 1, y * 2, z / 2);
        glNormal3f(normals_z[idx].x, normals_z[idx].y, normals_z[idx].z);
        idx = XYZ2IF(x * 2 + 1, y * 2, z);
        glVertex3f(vertices[idx].x, vertices[idx].y, vertices[idx].z);

        glTexCoord2d(1, 0.5);
        idx = XYZ2IF(x * 2 + 2, y * 2 + 1, z / 2);
        glNormal3f(normals_z[idx].x, normals_z[idx].y, normals_z[idx].z);
        idx = XYZ2IF(x * 2 + 2, y * 2 + 1, z);
        glVertex3f(vertices[idx].x, vertices[idx].y, vertices[idx].z);

        glTexCoord2d(1, 1);
        idx = XYZ2IF(x * 2 + 2, y * 2, z / 2);
        glNormal3f(normals_z[idx].x, normals_z[idx].y, normals_z[idx].z);
        idx = XYZ2IF(x * 2 + 2, y * 2, z);
        glVertex3f(vertices[idx].x, vertices[idx].y, vertices[idx].z);
    }
    glEnd();
}

static void draw_east(int x, int y, unsigned int z, DK_Texture texture) {
    unsigned int idx;

    x += DK_MAP_BORDER / 2;
    y += DK_MAP_BORDER / 2;

    set_texture(x, y, z, texture);

    glBegin(GL_QUAD_STRIP);
    {
        glTexCoord2d(0, 0);
        idx = XYZ2IH(x, y * 2, z + 2);
        glNormal3f(normals_x[idx].x, normals_x[idx].y, normals_x[idx].z);
        idx = XYZ2IF(x * 2, y * 2, z + 2);
        glVertex3f(vertices[idx].x, vertices[idx].y, vertices[idx].z);

        glTexCoord2d(0, 0.5);
        idx = XYZ2IH(x, y * 2, z + 1);
        glNormal3f(normals_x[idx].x, normals_x[idx].y, normals_x[idx].z);
        idx = XYZ2IF(x * 2, y * 2, z + 1);
        glVertex3f(vertices[idx].x, vertices[idx].y, vertices[idx].z);

        glTexCoord2d(0.5, 0);
        idx = XYZ2IH(x, y * 2 + 1, z + 2);
        glNormal3f(normals_x[idx].x, normals_x[idx].y, normals_x[idx].z);
        idx = XYZ2IF(x * 2, y * 2 + 1, z + 2);
        glVertex3f(vertices[idx].x, vertices[idx].y, vertices[idx].z);

        glTexCoord2d(0.5, 0.5);
        idx = XYZ2IH(x, y * 2 + 1, z + 1);
        glNormal3f(normals_x[idx].x, normals_x[idx].y, normals_x[idx].z);
        idx = XYZ2IF(x * 2, y * 2 + 1, z + 1);
        glVertex3f(vertices[idx].x, vertices[idx].y, vertices[idx].z);

        glTexCoord2d(1, 0);
        idx = XYZ2IH(x, y * 2 + 2, z + 2);
        glNormal3f(normals_x[idx].x, normals_x[idx].y, normals_x[idx].z);
        idx = XYZ2IF(x * 2, y * 2 + 2, z + 2);
        glVertex3f(vertices[idx].x, vertices[idx].y, vertices[idx].z);

        glTexCoord2d(1, 0.5);
        idx = XYZ2IH(x, y * 2 + 2, z + 1);
        glNormal3f(normals_x[idx].x, normals_x[idx].y, normals_x[idx].z);
        idx = XYZ2IF(x * 2, y * 2 + 2, z + 1);
        glVertex3f(vertices[idx].x, vertices[idx].y, vertices[idx].z);
    }
    glEnd();

    glBegin(GL_QUAD_STRIP);
    {
        glTexCoord2d(0, 0.5);
        idx = XYZ2IH(x, y * 2, z + 1);
        glNormal3f(normals_x[idx].x, normals_x[idx].y, normals_x[idx].z);
        idx = XYZ2IF(x * 2, y * 2, z + 1);
        glVertex3f(vertices[idx].x, vertices[idx].y, vertices[idx].z);

        glTexCoord2d(0, 1);
        idx = XYZ2IH(x, y * 2, z);
        glNormal3f(normals_x[idx].x, normals_x[idx].y, normals_x[idx].z);
        idx = XYZ2IF(x * 2, y * 2, z);
        glVertex3f(vertices[idx].x, vertices[idx].y, vertices[idx].z);

        glTexCoord2d(0.5, 0.5);
        idx = XYZ2IH(x, y * 2 + 1, z + 1);
        glNormal3f(normals_x[idx].x, normals_x[idx].y, normals_x[idx].z);
        idx = XYZ2IF(x * 2, y * 2 + 1, z + 1);
        glVertex3f(vertices[idx].x, vertices[idx].y, vertices[idx].z);

        glTexCoord2d(0.5, 1);
        idx = XYZ2IH(x, y * 2 + 1, z);
        glNormal3f(normals_x[idx].x, normals_x[idx].y, normals_x[idx].z);
        idx = XYZ2IF(x * 2, y * 2 + 1, z);
        glVertex3f(vertices[idx].x, vertices[idx].y, vertices[idx].z);

        glTexCoord2d(1, 0.5);
        idx = XYZ2IH(x, y * 2 + 2, z + 1);
        glNormal3f(normals_x[idx].x, normals_x[idx].y, normals_x[idx].z);
        idx = XYZ2IF(x * 2, y * 2 + 2, z + 1);
        glVertex3f(vertices[idx].x, vertices[idx].y, vertices[idx].z);

        glTexCoord2d(1, 1);
        idx = XYZ2IH(x, y * 2 + 2, z);
        glNormal3f(normals_x[idx].x, normals_x[idx].y, normals_x[idx].z);
        idx = XYZ2IF(x * 2, y * 2 + 2, z);
        glVertex3f(vertices[idx].x, vertices[idx].y, vertices[idx].z);
    }
    glEnd();
}

static void draw_west(int x, int y, unsigned int z, DK_Texture texture) {
    unsigned int idx;

    x += DK_MAP_BORDER / 2;
    y += DK_MAP_BORDER / 2;

    set_texture(x, y, z, texture);

    glBegin(GL_QUAD_STRIP);
    {
        glTexCoord2d(0, 0);
        idx = XYZ2IH(x, y * 2, z + 2);
        glNormal3f(-1 * normals_x[idx].x, -1 * normals_x[idx].y, -1 * normals_x[idx].z);
        idx = XYZ2IF(x * 2, y * 2 + 2, z + 2);
        glVertex3f(vertices[idx].x, vertices[idx].y, vertices[idx].z);

        glTexCoord2d(0, 0.5);
        idx = XYZ2IH(x, y * 2, z + 1);
        glNormal3f(-1 * normals_x[idx].x, -1 * normals_x[idx].y, -1 * normals_x[idx].z);
        idx = XYZ2IF(x * 2, y * 2 + 2, z + 1);
        glVertex3f(vertices[idx].x, vertices[idx].y, vertices[idx].z);

        glTexCoord2d(0.5, 0);
        idx = XYZ2IH(x, y * 2 + 1, z + 2);
        glNormal3f(-1 * normals_x[idx].x, -1 * normals_x[idx].y, -1 * normals_x[idx].z);
        idx = XYZ2IF(x * 2, y * 2 + 1, z + 2);
        glVertex3f(vertices[idx].x, vertices[idx].y, vertices[idx].z);

        glTexCoord2d(0.5, 0.5);
        idx = XYZ2IH(x, y * 2 + 1, z + 1);
        glNormal3f(-1 * normals_x[idx].x, -1 * normals_x[idx].y, -1 * normals_x[idx].z);
        idx = XYZ2IF(x * 2, y * 2 + 1, z + 1);
        glVertex3f(vertices[idx].x, vertices[idx].y, vertices[idx].z);

        glTexCoord2d(1, 0);
        idx = XYZ2IH(x, y * 2 + 2, z + 2);
        glNormal3f(-1 * normals_x[idx].x, -1 * normals_x[idx].y, -1 * normals_x[idx].z);
        idx = XYZ2IF(x * 2, y * 2, z + 2);
        glVertex3f(vertices[idx].x, vertices[idx].y, vertices[idx].z);

        glTexCoord2d(1, 0.5);
        idx = XYZ2IH(x, y * 2 + 2, z + 1);
        glNormal3f(-1 * normals_x[idx].x, -1 * normals_x[idx].y, -1 * normals_x[idx].z);
        idx = XYZ2IF(x * 2, y * 2, z + 1);
        glVertex3f(vertices[idx].x, vertices[idx].y, vertices[idx].z);
    }
    glEnd();

    glBegin(GL_QUAD_STRIP);
    {
        glTexCoord2d(0, 0.5);
        idx = XYZ2IH(x, y * 2, z + 1);
        glNormal3f(-1 * normals_x[idx].x, -1 * normals_x[idx].y, -1 * normals_x[idx].z);
        idx = XYZ2IF(x * 2, y * 2 + 2, z + 1);
        glVertex3f(vertices[idx].x, vertices[idx].y, vertices[idx].z);

        glTexCoord2d(0, 1);
        idx = XYZ2IH(x, y * 2, z);
        glNormal3f(-1 * normals_x[idx].x, -1 * normals_x[idx].y, -1 * normals_x[idx].z);
        idx = XYZ2IF(x * 2, y * 2 + 2, z);
        glVertex3f(vertices[idx].x, vertices[idx].y, vertices[idx].z);

        glTexCoord2d(0.5, 0.5);
        idx = XYZ2IH(x, y * 2 + 1, z + 1);
        glNormal3f(-1 * normals_x[idx].x, -1 * normals_x[idx].y, -1 * normals_x[idx].z);
        idx = XYZ2IF(x * 2, y * 2 + 1, z + 1);
        glVertex3f(vertices[idx].x, vertices[idx].y, vertices[idx].z);

        glTexCoord2d(0.5, 1);
        idx = XYZ2IH(x, y * 2 + 1, z);
        glNormal3f(-1 * normals_x[idx].x, -1 * normals_x[idx].y, -1 * normals_x[idx].z);
        idx = XYZ2IF(x * 2, y * 2 + 1, z);
        glVertex3f(vertices[idx].x, vertices[idx].y, vertices[idx].z);

        glTexCoord2d(1, 0.5);
        idx = XYZ2IH(x, y * 2 + 2, z + 1);
        glNormal3f(-1 * normals_x[idx].x, -1 * normals_x[idx].y, -1 * normals_x[idx].z);
        idx = XYZ2IF(x * 2, y * 2, z + 1);
        glVertex3f(vertices[idx].x, vertices[idx].y, vertices[idx].z);

        glTexCoord2d(1, 1);
        idx = XYZ2IH(x, y * 2 + 2, z);
        glNormal3f(-1 * normals_x[idx].x, -1 * normals_x[idx].y, -1 * normals_x[idx].z);
        idx = XYZ2IF(x * 2, y * 2, z);
        glVertex3f(vertices[idx].x, vertices[idx].y, vertices[idx].z);
    }
    glEnd();
}

static void draw_north(int x, int y, unsigned int z, DK_Texture texture) {
    unsigned int idx;

    x += DK_MAP_BORDER / 2;
    y += DK_MAP_BORDER / 2;

    set_texture(x, y, z, texture);

    glBegin(GL_QUAD_STRIP);
    {
        glTexCoord2d(0, 0);
        idx = XYZ2IH(x * 2, y, z + 2);
        glNormal3f(normals_y[idx].x, normals_y[idx].y, normals_y[idx].z);
        idx = XYZ2IF(x * 2 + 2, y * 2, z + 2);
        glVertex3f(vertices[idx].x, vertices[idx].y, vertices[idx].z);

        glTexCoord2d(0, 0.5);
        idx = XYZ2IH(x * 2, y, z + 1);
        glNormal3f(normals_y[idx].x, normals_y[idx].y, normals_y[idx].z);
        idx = XYZ2IF(x * 2 + 2, y * 2, z + 1);
        glVertex3f(vertices[idx].x, vertices[idx].y, vertices[idx].z);

        glTexCoord2d(0.5, 0);
        idx = XYZ2IH(x * 2 + 1, y, z + 2);
        glNormal3f(normals_y[idx].x, normals_y[idx].y, normals_y[idx].z);
        idx = XYZ2IF(x * 2 + 1, y * 2, z + 2);
        glVertex3f(vertices[idx].x, vertices[idx].y, vertices[idx].z);

        glTexCoord2d(0.5, 0.5);
        idx = XYZ2IH(x * 2 + 1, y, z + 1);
        glNormal3f(normals_y[idx].x, normals_y[idx].y, normals_y[idx].z);
        idx = XYZ2IF(x * 2 + 1, y * 2, z + 1);
        glVertex3f(vertices[idx].x, vertices[idx].y, vertices[idx].z);

        glTexCoord2d(1, 0);
        idx = XYZ2IH(y * 2 + 2, y, z + 2);
        glNormal3f(normals_y[idx].x, normals_y[idx].y, normals_y[idx].z);
        idx = XYZ2IF(x * 2, y * 2, z + 2);
        glVertex3f(vertices[idx].x, vertices[idx].y, vertices[idx].z);

        glTexCoord2d(1, 0.5);
        idx = XYZ2IH(x * 2 + 2, y, z + 1);
        glNormal3f(normals_y[idx].x, normals_y[idx].y, normals_y[idx].z);
        idx = XYZ2IF(x * 2, y * 2, z + 1);
        glVertex3f(vertices[idx].x, vertices[idx].y, vertices[idx].z);
    }
    glEnd();

    glBegin(GL_QUAD_STRIP);
    {
        glTexCoord2d(0, 0.5);
        idx = XYZ2IH(x * 2, y, z + 1);
        glNormal3f(normals_y[idx].x, normals_y[idx].y, normals_y[idx].z);
        idx = XYZ2IF(x * 2 + 2, y * 2, z + 1);
        glVertex3f(vertices[idx].x, vertices[idx].y, vertices[idx].z);

        glTexCoord2d(0, 1);
        idx = XYZ2IH(x * 2, y, z);
        glNormal3f(normals_y[idx].x, normals_y[idx].y, normals_y[idx].z);
        idx = XYZ2IF(x * 2 + 2, y * 2, z);
        glVertex3f(vertices[idx].x, vertices[idx].y, vertices[idx].z);

        glTexCoord2d(0.5, 0.5);
        idx = XYZ2IH(x * 2 + 1, y, z + 1);
        glNormal3f(normals_y[idx].x, normals_y[idx].y, normals_y[idx].z);
        idx = XYZ2IF(x * 2 + 1, y * 2, z + 1);
        glVertex3f(vertices[idx].x, vertices[idx].y, vertices[idx].z);

        glTexCoord2d(0.5, 1);
        idx = XYZ2IH(x * 2 + 1, y, z);
        glNormal3f(normals_y[idx].x, normals_y[idx].y, normals_y[idx].z);
        idx = XYZ2IF(x * 2 + 1, y * 2, z);
        glVertex3f(vertices[idx].x, vertices[idx].y, vertices[idx].z);

        glTexCoord2d(1, 0.5);
        idx = XYZ2IH(x * 2 + 2, y, z + 1);
        glNormal3f(normals_y[idx].x, normals_y[idx].y, normals_y[idx].z);
        idx = XYZ2IF(x * 2, y * 2, z + 1);
        glVertex3f(vertices[idx].x, vertices[idx].y, vertices[idx].z);

        glTexCoord2d(1, 1);
        idx = XYZ2IH(x * 2 + 2, y, z);
        glNormal3f(normals_y[idx].x, normals_y[idx].y, normals_y[idx].z);
        idx = XYZ2IF(x * 2, y * 2, z);
        glVertex3f(vertices[idx].x, vertices[idx].y, vertices[idx].z);
    }
    glEnd();
}

static void draw_south(int x, int y, unsigned int z, DK_Texture texture) {
    unsigned int idx;

    x += DK_MAP_BORDER / 2;
    y += DK_MAP_BORDER / 2;

    set_texture(x, y, z, texture);

    glBegin(GL_QUAD_STRIP);
    {
        glTexCoord2d(0, 0);
        idx = XYZ2IH(x * 2, y, z + 2);
        glNormal3f(-1 * normals_y[idx].x, -1 * normals_y[idx].y, -1 * normals_y[idx].z);
        idx = XYZ2IF(x * 2, y * 2, z + 2);
        glVertex3f(vertices[idx].x, vertices[idx].y, vertices[idx].z);

        glTexCoord2d(0, 0.5);
        idx = XYZ2IH(x * 2, y, z + 1);
        glNormal3f(-1 * normals_y[idx].x, -1 * normals_y[idx].y, -1 * normals_y[idx].z);
        idx = XYZ2IF(x * 2, y * 2, z + 1);
        glVertex3f(vertices[idx].x, vertices[idx].y, vertices[idx].z);

        glTexCoord2d(0.5, 0);
        idx = XYZ2IH(x * 2 + 1, y, z + 2);
        glNormal3f(-1 * normals_y[idx].x, -1 * normals_y[idx].y, -1 * normals_y[idx].z);
        idx = XYZ2IF(x * 2 + 1, y * 2, z + 2);
        glVertex3f(vertices[idx].x, vertices[idx].y, vertices[idx].z);

        glTexCoord2d(0.5, 0.5);
        idx = XYZ2IH(x * 2 + 1, y, z + 1);
        glNormal3f(-1 * normals_y[idx].x, -1 * normals_y[idx].y, -1 * normals_y[idx].z);
        idx = XYZ2IF(x * 2 + 1, y * 2, z + 1);
        glVertex3f(vertices[idx].x, vertices[idx].y, vertices[idx].z);

        glTexCoord2d(1, 0);
        idx = XYZ2IH(y * 2 + 2, y, z + 2);
        glNormal3f(-1 * normals_y[idx].x, -1 * normals_y[idx].y, -1 * normals_y[idx].z);
        idx = XYZ2IF(x * 2 + 2, y * 2, z + 2);
        glVertex3f(vertices[idx].x, vertices[idx].y, vertices[idx].z);

        glTexCoord2d(1, 0.5);
        idx = XYZ2IH(x * 2 + 2, y, z + 1);
        glNormal3f(-1 * normals_y[idx].x, -1 * normals_y[idx].y, -1 * normals_y[idx].z);
        idx = XYZ2IF(x * 2 + 2, y * 2, z + 1);
        glVertex3f(vertices[idx].x, vertices[idx].y, vertices[idx].z);
    }
    glEnd();

    glBegin(GL_QUAD_STRIP);
    {
        glTexCoord2d(0, 0.5);
        idx = XYZ2IH(x * 2, y, z + 1);
        glNormal3f(-1 * normals_y[idx].x, -1 * normals_y[idx].y, -1 * normals_y[idx].z);
        idx = XYZ2IF(x * 2, y * 2, z + 1);
        glVertex3f(vertices[idx].x, vertices[idx].y, vertices[idx].z);

        glTexCoord2d(0, 1);
        idx = XYZ2IH(x * 2, y, z);
        glNormal3f(-1 * normals_y[idx].x, -1 * normals_y[idx].y, -1 * normals_y[idx].z);
        idx = XYZ2IF(x * 2, y * 2, z);
        glVertex3f(vertices[idx].x, vertices[idx].y, vertices[idx].z);

        glTexCoord2d(0.5, 0.5);
        idx = XYZ2IH(x * 2 + 1, y, z + 1);
        glNormal3f(-1 * normals_y[idx].x, -1 * normals_y[idx].y, -1 * normals_y[idx].z);
        idx = XYZ2IF(x * 2 + 1, y * 2, z + 1);
        glVertex3f(vertices[idx].x, vertices[idx].y, vertices[idx].z);

        glTexCoord2d(0.5, 1);
        idx = XYZ2IH(x * 2 + 1, y, z);
        glNormal3f(-1 * normals_y[idx].x, -1 * normals_y[idx].y, -1 * normals_y[idx].z);
        idx = XYZ2IF(x * 2 + 1, y * 2, z);
        glVertex3f(vertices[idx].x, vertices[idx].y, vertices[idx].z);

        glTexCoord2d(1, 0.5);
        idx = XYZ2IH(x * 2 + 2, y, z + 1);
        glNormal3f(-1 * normals_y[idx].x, -1 * normals_y[idx].y, -1 * normals_y[idx].z);
        idx = XYZ2IF(x * 2 + 2, y * 2, z + 1);
        glVertex3f(vertices[idx].x, vertices[idx].y, vertices[idx].z);

        glTexCoord2d(1, 1);
        idx = XYZ2IH(x * 2 + 2, y, z);
        glNormal3f(-1 * normals_y[idx].x, -1 * normals_y[idx].y, -1 * normals_y[idx].z);
        idx = XYZ2IF(x * 2 + 2, y * 2, z);
        glVertex3f(vertices[idx].x, vertices[idx].y, vertices[idx].z);
    }
    glEnd();
}

static void draw_outline(unsigned int start_x, unsigned int start_y, unsigned int end_x, unsigned int end_y) {
    unsigned int x, y, map_x, map_y, idx;

    start_x += DK_MAP_BORDER / 2;
    start_y += DK_MAP_BORDER / 2;
    end_x += DK_MAP_BORDER / 2;
    end_y += DK_MAP_BORDER / 2;

    // Store state.
    glPushAttrib(GL_ENABLE_BIT | GL_CURRENT_BIT);
    glPushMatrix();

    glBindTexture(GL_TEXTURE_2D, 0);
    glDisable(GL_LIGHTING);

    // Set up for line drawing.
    glLineWidth(3.0f + DK_GetCameraZoom() * 3.0f);
    //glColor4f(DK_MAP_OUTLINE_COLOR, 0.5f);

    for (x = start_x; x <= end_x; ++x) {
        map_x = x - DK_MAP_BORDER / 2;
        for (y = start_y; y <= end_y; ++y) {
            map_y = y - DK_MAP_BORDER / 2;
            if (!DK_block_is_selectable(DK_PLAYER_RED, map_x, map_y)) {
                continue;
            }

            // Draw north outline.
            if (y == end_y || DK_block_is_open(DK_block_at(map_x, map_y + 1))) {
                glPushMatrix();
                glTranslatef(0, DK_MAP_OUTLINE_OFFSET, DK_MAP_OUTLINE_OFFSET);

                glBegin(GL_LINES);
                {
                    idx = XYZ2IF(x * 2, y * 2 + 2, 4);
                    glVertex3f(vertices[idx].x, vertices[idx].y, vertices[idx].z);
                    idx = XYZ2IF(x * 2 + 1, y * 2 + 2, 4);
                    glVertex3f(vertices[idx].x, vertices[idx].y, vertices[idx].z);
                    idx = XYZ2IF(x * 2 + 1, y * 2 + 2, 4);
                    glVertex3f(vertices[idx].x, vertices[idx].y, vertices[idx].z);
                    idx = XYZ2IF(x * 2 + 2, y * 2 + 2, 4);
                    glVertex3f(vertices[idx].x, vertices[idx].y, vertices[idx].z);
                }
                glEnd();

                if (DK_block_is_open(DK_block_at(map_x, map_y + 1))) {
                    glBegin(GL_LINES);
                    {
                        idx = XYZ2IF(x * 2, y * 2 + 2, 2);
                        glVertex3f(vertices[idx].x, vertices[idx].y, vertices[idx].z);
                        idx = XYZ2IF(x * 2 + 1, y * 2 + 2, 2);
                        glVertex3f(vertices[idx].x, vertices[idx].y, vertices[idx].z);
                        idx = XYZ2IF(x * 2 + 1, y * 2 + 2, 2);
                        glVertex3f(vertices[idx].x, vertices[idx].y, vertices[idx].z);
                        idx = XYZ2IF(x * 2 + 2, y * 2 + 2, 2);
                        glVertex3f(vertices[idx].x, vertices[idx].y, vertices[idx].z);
                    }
                    glEnd();
                }

                if (x == start_x ?
                        (DK_block_is_open(DK_block_at(map_x, map_y + 1)) ||
                        DK_block_is_open(DK_block_at(map_x - 1, map_y)))
                        :
                        ((DK_block_is_open(DK_block_at(map_x, map_y + 1)) ^
                        DK_block_is_open(DK_block_at(map_x - 1, map_y + 1))) ||
                        DK_block_is_open(DK_block_at(map_x - 1, map_y)))) {
                    // Draw north west top-to-bottom line.
                    glTranslatef(-DK_MAP_OUTLINE_OFFSET, 0, 0);
                    glBegin(GL_LINES);
                    {
                        idx = XYZ2IF(x * 2, y * 2 + 2, 4);
                        glVertex3f(vertices[idx].x, vertices[idx].y, vertices[idx].z);
                        idx = XYZ2IF(x * 2, y * 2 + 2, 3);
                        glVertex3f(vertices[idx].x, vertices[idx].y, vertices[idx].z);
                        idx = XYZ2IF(x * 2, y * 2 + 2, 3);
                        glVertex3f(vertices[idx].x, vertices[idx].y, vertices[idx].z);
                        idx = XYZ2IF(x * 2, y * 2 + 2, 2);
                        glVertex3f(vertices[idx].x, vertices[idx].y, vertices[idx].z);
                    }
                    glEnd();
                    glTranslatef(DK_MAP_OUTLINE_OFFSET, 0, 0);
                }
                if (x == end_x ?
                        (DK_block_is_open(DK_block_at(map_x, map_y + 1)) ||
                        DK_block_is_open(DK_block_at(map_x + 1, map_y)))
                        :
                        ((DK_block_is_open(DK_block_at(map_x, map_y + 1)) ^
                        DK_block_is_open(DK_block_at(map_x + 1, map_y + 1))) ||
                        DK_block_is_open(DK_block_at(map_x + 1, map_y)))) {
                    // Draw north east top-to-bottom line.
                    glTranslatef(DK_MAP_OUTLINE_OFFSET, 0, 0);
                    glBegin(GL_LINES);
                    {
                        idx = XYZ2IF(x * 2 + 2, y * 2 + 2, 4);
                        glVertex3f(vertices[idx].x, vertices[idx].y, vertices[idx].z);
                        idx = XYZ2IF(x * 2 + 2, y * 2 + 2, 3);
                        glVertex3f(vertices[idx].x, vertices[idx].y, vertices[idx].z);
                        idx = XYZ2IF(x * 2 + 2, y * 2 + 2, 3);
                        glVertex3f(vertices[idx].x, vertices[idx].y, vertices[idx].z);
                        idx = XYZ2IF(x * 2 + 2, y * 2 + 2, 2);
                        glVertex3f(vertices[idx].x, vertices[idx].y, vertices[idx].z);
                    }
                    glEnd();
                    glTranslatef(-DK_MAP_OUTLINE_OFFSET, 0, 0);
                }

                glPopMatrix();
            }

            // Draw south outline.
            if (y == start_y || DK_block_is_open(DK_block_at(map_x, map_y - 1))) {
                glPushMatrix();
                glTranslatef(0, -DK_MAP_OUTLINE_OFFSET, DK_MAP_OUTLINE_OFFSET);

                glBegin(GL_LINES);
                {
                    idx = XYZ2IF(x * 2, y * 2, 4);
                    glVertex3f(vertices[idx].x, vertices[idx].y, vertices[idx].z);
                    idx = XYZ2IF(x * 2 + 1, y * 2, 4);
                    glVertex3f(vertices[idx].x, vertices[idx].y, vertices[idx].z);
                    idx = XYZ2IF(x * 2 + 1, y * 2, 4);
                    glVertex3f(vertices[idx].x, vertices[idx].y, vertices[idx].z);
                    idx = XYZ2IF(x * 2 + 2, y * 2, 4);
                    glVertex3f(vertices[idx].x, vertices[idx].y, vertices[idx].z);
                }
                glEnd();

                if (DK_block_is_open(DK_block_at(map_x, map_y - 1))) {
                    glBegin(GL_LINES);
                    {
                        idx = XYZ2IF(x * 2, y * 2, 2);
                        glVertex3f(vertices[idx].x, vertices[idx].y, vertices[idx].z);
                        idx = XYZ2IF(x * 2 + 1, y * 2, 2);
                        glVertex3f(vertices[idx].x, vertices[idx].y, vertices[idx].z);
                        idx = XYZ2IF(x * 2 + 1, y * 2, 2);
                        glVertex3f(vertices[idx].x, vertices[idx].y, vertices[idx].z);
                        idx = XYZ2IF(x * 2 + 2, y * 2, 2);
                        glVertex3f(vertices[idx].x, vertices[idx].y, vertices[idx].z);
                    }
                    glEnd();
                }

                if (x == start_x ?
                        (DK_block_is_open(DK_block_at(map_x, map_y - 1)) ||
                        DK_block_is_open(DK_block_at(map_x - 1, map_y)))
                        :
                        ((DK_block_is_open(DK_block_at(map_x, map_y - 1)) ^
                        DK_block_is_open(DK_block_at(map_x - 1, map_y - 1))) ||
                        DK_block_is_open(DK_block_at(map_x - 1, map_y)))) {
                    // Draw south west top-to-bottom line.
                    glTranslatef(-DK_MAP_OUTLINE_OFFSET, 0, 0);
                    glBegin(GL_LINES);
                    {
                        idx = XYZ2IF(x * 2, y * 2, 4);
                        glVertex3f(vertices[idx].x, vertices[idx].y, vertices[idx].z);
                        idx = XYZ2IF(x * 2, y * 2, 3);
                        glVertex3f(vertices[idx].x, vertices[idx].y, vertices[idx].z);
                        idx = XYZ2IF(x * 2, y * 2, 3);
                        glVertex3f(vertices[idx].x, vertices[idx].y, vertices[idx].z);
                        idx = XYZ2IF(x * 2, y * 2, 2);
                        glVertex3f(vertices[idx].x, vertices[idx].y, vertices[idx].z);
                    }
                    glEnd();
                    glTranslatef(DK_MAP_OUTLINE_OFFSET, 0, 0);
                }
                if (x == end_x ?
                        (DK_block_is_open(DK_block_at(map_x, map_y - 1)) ||
                        DK_block_is_open(DK_block_at(map_x + 1, map_y)))
                        :
                        ((DK_block_is_open(DK_block_at(map_x, map_y - 1)) ^
                        DK_block_is_open(DK_block_at(map_x + 1, map_y - 1))) ||
                        DK_block_is_open(DK_block_at(map_x + 1, map_y)))) {
                    // Draw south east top-to-bottom line.
                    glTranslatef(DK_MAP_OUTLINE_OFFSET, 0, 0);
                    glBegin(GL_LINES);
                    {
                        idx = XYZ2IF(x * 2 + 2, y * 2, 4);
                        glVertex3f(vertices[idx].x, vertices[idx].y, vertices[idx].z);
                        idx = XYZ2IF(x * 2 + 2, y * 2, 3);
                        glVertex3f(vertices[idx].x, vertices[idx].y, vertices[idx].z);
                        idx = XYZ2IF(x * 2 + 2, y * 2, 3);
                        glVertex3f(vertices[idx].x, vertices[idx].y, vertices[idx].z);
                        idx = XYZ2IF(x * 2 + 2, y * 2, 2);
                        glVertex3f(vertices[idx].x, vertices[idx].y, vertices[idx].z);
                    }
                    glEnd();
                    glTranslatef(-DK_MAP_OUTLINE_OFFSET, 0, 0);
                }

                glPopMatrix();
            }

            // Draw west outline.
            if (x == start_x || DK_block_is_open(DK_block_at(map_x - 1, map_y))) {
                glPushMatrix();
                glTranslatef(-DK_MAP_OUTLINE_OFFSET, 0, DK_MAP_OUTLINE_OFFSET);

                glBegin(GL_LINES);
                {
                    idx = XYZ2IF(x * 2, y * 2, 4);
                    glVertex3f(vertices[idx].x, vertices[idx].y, vertices[idx].z);
                    idx = XYZ2IF(x * 2, y * 2 + 1, 4);
                    glVertex3f(vertices[idx].x, vertices[idx].y, vertices[idx].z);
                    idx = XYZ2IF(x * 2, y * 2 + 1, 4);
                    glVertex3f(vertices[idx].x, vertices[idx].y, vertices[idx].z);
                    idx = XYZ2IF(x * 2, y * 2 + 2, 4);
                    glVertex3f(vertices[idx].x, vertices[idx].y, vertices[idx].z);
                }
                glEnd();

                if (DK_block_is_open(DK_block_at(map_x - 1, map_y))) {
                    glBegin(GL_LINES);
                    {
                        idx = XYZ2IF(x * 2, y * 2, 2);
                        glVertex3f(vertices[idx].x, vertices[idx].y, vertices[idx].z);
                        idx = XYZ2IF(x * 2, y * 2 + 1, 2);
                        glVertex3f(vertices[idx].x, vertices[idx].y, vertices[idx].z);
                        idx = XYZ2IF(x * 2, y * 2 + 1, 2);
                        glVertex3f(vertices[idx].x, vertices[idx].y, vertices[idx].z);
                        idx = XYZ2IF(x * 2, y * 2 + 2, 2);
                        glVertex3f(vertices[idx].x, vertices[idx].y, vertices[idx].z);
                    }
                    glEnd();
                }

                glPopMatrix();
            }

            // Draw east outline.
            if (x == end_x || DK_block_is_open(DK_block_at(map_x + 1, map_y))) {
                glPushMatrix();
                glTranslatef(DK_MAP_OUTLINE_OFFSET, 0, DK_MAP_OUTLINE_OFFSET);

                glBegin(GL_LINES);
                {
                    idx = XYZ2IF(x * 2 + 2, y * 2, 4);
                    glVertex3f(vertices[idx].x, vertices[idx].y, vertices[idx].z);
                    idx = XYZ2IF(x * 2 + 2, y * 2 + 1, 4);
                    glVertex3f(vertices[idx].x, vertices[idx].y, vertices[idx].z);
                    idx = XYZ2IF(x * 2 + 2, y * 2 + 1, 4);
                    glVertex3f(vertices[idx].x, vertices[idx].y, vertices[idx].z);
                    idx = XYZ2IF(x * 2 + 2, y * 2 + 2, 4);
                    glVertex3f(vertices[idx].x, vertices[idx].y, vertices[idx].z);
                }
                glEnd();

                if (DK_block_is_open(DK_block_at(map_x + 1, map_y))) {
                    glBegin(GL_LINES);
                    {
                        idx = XYZ2IF(x * 2 + 2, y * 2, 2);
                        glVertex3f(vertices[idx].x, vertices[idx].y, vertices[idx].z);
                        idx = XYZ2IF(x * 2 + 2, y * 2 + 1, 2);
                        glVertex3f(vertices[idx].x, vertices[idx].y, vertices[idx].z);
                        idx = XYZ2IF(x * 2 + 2, y * 2 + 1, 2);
                        glVertex3f(vertices[idx].x, vertices[idx].y, vertices[idx].z);
                        idx = XYZ2IF(x * 2 + 2, y * 2 + 2, 2);
                        glVertex3f(vertices[idx].x, vertices[idx].y, vertices[idx].z);
                    }
                    glEnd();
                }

                glPopMatrix();
            }

        }
    }

    // Reset stuff.
    glPopMatrix();
    glPopAttrib();
}

///////////////////////////////////////////////////////////////////////////////
// Header implementation
///////////////////////////////////////////////////////////////////////////////

void DK_init_map(unsigned short size) {
    unsigned int x, y, z;

    // Reallocate data only if the size changed.
    if (size != DK_map_size) {
        // Free old map data.
        free(map);

        // Allocate new map data.
        if (!(map = calloc(size * size, sizeof (DK_Block)))) {
            fprintf(stderr, "Out of memory while allocating map data.\n");
            exit(EXIT_FAILURE);
        }

        // Free old map model data.
        free(vertices);
        free(normals_x);
        free(normals_y);
        free(normals_z);

        // Allocate new map model data.
        vertices_count_full = (size + DK_MAP_BORDER) * 2 + 1;
        vertices_count_semi = (size + DK_MAP_BORDER) + 1;
        vertices = calloc(vertices_count_full * vertices_count_full * 5, sizeof (v3f));
        normals_x = calloc(vertices_count_semi * vertices_count_full * 5, sizeof (v3f));
        normals_y = calloc(vertices_count_semi * vertices_count_full * 5, sizeof (v3f));
        normals_z = calloc(vertices_count_full * vertices_count_full * 3, sizeof (v3f));

        if (!vertices || !normals_x || !normals_y || !normals_z) {
            fprintf(stderr, "Out of memory while allocating map model data.\n");
            exit(EXIT_FAILURE);
        }
    }

    // Remember new map size (needed *now* for offset computation).
    DK_map_size = size;

    // Initialize map data.
    for (x = 0; x < size; ++x) {
        for (y = 0; y < size; ++y) {
            DK_Block* block = DK_block_at(x, y);
            block->type = DK_BLOCK_DIRT;
            block->health = DK_BLOCK_DIRT_HEALTH;
            block->strength = DK_BLOCK_DIRT_STRENGTH;
            block->room = DK_ROOM_NONE;
        }
    }

    // Initialize map model data.

    // Pass 1: set vertex positions.
    for (x = 0; x < vertices_count_full; ++x) {
        for (y = 0; y < vertices_count_full; ++y) {
            update_vertices(x, y);

            // Initialize normals to defaults (especially the border cases, i.e.
            // at 0 and max, because those aren't computed dynamically to avoid
            // having to check the border cases).
            for (z = 0; z < 5; ++z) {
                if ((x & 1) == 0) {
                    // Compute x normals.
                    v3f* n = &normals_x[XYZ2IH(x / 2, y, z)];
                    n->x = 1;
                    n->y = 0;
                    n->z = 0;
                }
                if ((y & 1) == 0) {
                    // Compute y normals.
                    v3f* n = &normals_y[XYZ2IH(x, y / 2, z)];
                    n->x = 0;
                    n->y = 1;
                    n->z = 0;
                }
                if ((z & 1) == 0) {
                    v3f* n = &normals_z[XYZ2IF(x, y, z / 2)];
                    n->x = 0;
                    n->y = 0;
                    n->z = 1;
                }
            }
        }
    }

    // Pass 2: compute normals.
    for (x = 1; x < vertices_count_full - 1; ++x) {
        for (y = 1; y < vertices_count_full - 1; ++y) {
            update_normals(x, y);
        }
    }
}

void DK_update_map(void) {
    int mouse_x, mouse_y;
    GLuint selected_name;

    SDL_GetMouseState(&mouse_x, &mouse_y);
    selected_name = pick_block(mouse_x, DK_resolution_y - mouse_y);
    cursor_x = (short) (selected_name & 0xFFFF);
    cursor_y = (short) (selected_name >> 16);
}

#define M_PI 3.14159265358979323846

void DK_render_map(void) {
    const vec2* cam_position = DK_GetCameraPosition();
    int x_begin = (int) (cam_position->v[0] / DK_BLOCK_SIZE) - DK_RENDER_AREA_X / 2;
    int y_begin = (int) (cam_position->v[1] / DK_BLOCK_SIZE) - DK_RENDER_AREA_Y_OFFSET;
    int x_end = x_begin + DK_RENDER_AREA_X;
    int y_end = y_begin + DK_RENDER_AREA_Y;
    int x, y, z;
    DK_Texture texture_top, texture_side, texture_top_wall, texture_top_owner;

    for (x = x_begin; x < x_end; ++x) {
        for (y = y_begin; y < y_end; ++y) {
            // Mark for select mode, coordinates of that block.
            if (picking) {
                glLoadName(((unsigned short) y << 16) | (unsigned short) x);
            } else {
                // Reset tint color.
                //glColorMaterial(GL_FRONT, GL_EMISSION);
                //glColor3f(0.0f, 0.0f, 0.0f);
                //glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);
                //glColor3f(1.0f, 1.0f, 1.0f);
            }

            texture_top_wall = 0;
            texture_top_owner = 0;
            if (x < 0 || y < 0 || x >= DK_map_size || y >= DK_map_size) {
                // Solid rock when out of bounds.
                z = 4;
                texture_top = DK_TEX_ROCK_TOP;
                texture_side = DK_TEX_ROCK_SIDE;
            } else {
                const DK_Block* block = DK_block_at(x, y);

                // Selected by the local player?
                if (!picking && DK_block_is_selected(DK_PLAYER_RED, x, y)) {
                    //const float intensity = 0.6f + sinf(SDL_GetTicks() * M_PI * (DK_MAP_SELECTED_PULSE_FREQUENCY)) * 0.3f;
                    //glColorMaterial(GL_FRONT, GL_EMISSION);
                    //glColor3f(DK_MAP_SELECTED_COLOR(intensity));
                }

                if (block->type == DK_BLOCK_NONE) {
                    // Render floor.
                    z = 2;
                    if (block->owner == DK_PLAYER_NONE) {
                        texture_top = DK_TEX_DIRT_FLOOR;
                    } else {
                        texture_top = DK_TEX_FLOOR;
                        texture_top_owner = DK_TEX_OWNER_RED;
                    }
                } else if (block->type == DK_BLOCK_WATER ||
                        block->type == DK_BLOCK_LAVA) {
                    // Draw fluid floor.
                    z = 0;
                    texture_top = DK_TEX_DIRT_FLOOR;
                } else {
                    z = 4;
                    // Render block top.
                    switch (block->type) {
                        case DK_BLOCK_DIRT:
                            if (block->owner == DK_PLAYER_NONE) {
                                // Normal dirt block.
                                texture_top = DK_TEX_DIRT_TOP;
                                texture_side = DK_TEX_DIRT_SIDE;
                            } else {
                                // It's a wall.
                                texture_top = DK_TEX_DIRT_TOP;
                                texture_top_wall = DK_TEX_WALL_TOP_NESW;
                                texture_top_owner = DK_TEX_OWNER_RED;
                                texture_side = DK_TEX_DIRT_SIDE;
                            }
                            break;
                        case DK_BLOCK_GOLD:
                            texture_top = DK_TEX_GOLD_TOP;
                            texture_side = DK_TEX_GOLD_SIDE;
                            break;
                        case DK_BLOCK_GEM:
                            texture_top = DK_TEX_GEM_TOP;
                            texture_side = DK_TEX_GEM_SIDE;
                            break;
                        case DK_BLOCK_ROCK:
                            // Solid rock, cannot be owned.
                            texture_top = DK_TEX_ROCK_TOP;
                            texture_side = DK_TEX_ROCK_SIDE;
                            break;
                        default:
                            continue;
                    }
                }
            }

            draw_top(x, y, z, texture_top);

            if (!picking && (texture_top_wall || texture_top_owner)) {
                glEnable(GL_BLEND);
                glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

                if (texture_top_wall) {
                    draw_top(x, y, z, texture_top_wall);
                }
                if (texture_top_owner) {
                    draw_top(x, y, z, texture_top_owner);
                }

                glDisable(GL_BLEND);
            }

            // Check if we need to render walls.
            if (z == 4) {
                // North wall.
                if (y + 1 < DK_map_size && DK_block_is_open(DK_block_at(x, y + 1))) {
                    draw_north(x, y + 1, 2, texture_side);
                }

                // South wall.
                if (y > 0 && DK_block_is_open(DK_block_at(x, y - 1))) {
                    draw_south(x, y, 2, texture_side);
                }

                // West wall.
                if (x > 0 && DK_block_is_open(DK_block_at(x - 1, y))) {
                    draw_west(x, y, 2, texture_side);
                }

                // East wall.
                if (x + 1 < DK_map_size && DK_block_is_open(DK_block_at(x + 1, y))) {
                    draw_east(x + 1, y, 2, texture_side);
                }
            }

            // Check if we need to render water walls.
            if (z == 0) {
                // North wall.
                if (y + 1 < DK_map_size && !DK_block_is_fluid(DK_block_at(x, y + 1))) {
                    draw_south(x, y + 1, 0, DK_TEX_FLUID_SIDE);
                }

                // South wall.
                if (y > 0 && !DK_block_is_fluid(DK_block_at(x, y - 1))) {
                    draw_north(x, y, 0, DK_TEX_FLUID_SIDE);
                }

                // West wall.
                if (x > 0 && !DK_block_is_fluid(DK_block_at(x - 1, y))) {
                    draw_east(x, y, 0, DK_TEX_FLUID_SIDE);
                }

                // East wall.
                if (x + 1 < DK_map_size && !DK_block_is_fluid(DK_block_at(x + 1, y))) {
                    draw_west(x + 1, y, 0, DK_TEX_FLUID_SIDE);
                }
            }
        }
    }

    if (!picking) {
        int sx, sy, ex, ey;
        DK_selection(&sx, &sy, &ex, &ey);
        draw_outline(sx, sy, ex, ey);
    }
}

///////////////////////////////////////////////////////////////////////////////
// Save / Load
///////////////////////////////////////////////////////////////////////////////

void DK_map_load(const char* filename) {
    int i, j;

    DK_init_map(128);
    DK_init_selection();
    DK_InitAStar();
    DK_init_units();

    for (i = 0; i < 7; ++i) {
        for (j = 0; j < 7; ++j) {
            if (i <= 1 || j <= 1) {
                DK_block_set_owner(DK_block_at(4 + i, 5 + j), DK_PLAYER_RED);
            }
            if (i > 0 && j > 0 && i < 6 && j < 6) {
                DK_block_set_type(DK_block_at(4 + i, 5 + j), DK_BLOCK_NONE);
            }
        }
    }

    DK_block_set_type(DK_block_at(7, 8), DK_BLOCK_DIRT);
    DK_block_set_type(DK_block_at(8, 8), DK_BLOCK_DIRT);

    DK_block_set_type(DK_block_at(10, 8), DK_BLOCK_WATER);
    DK_block_set_type(DK_block_at(11, 8), DK_BLOCK_WATER);
    DK_block_set_type(DK_block_at(11, 9), DK_BLOCK_WATER);
    //DK_block_at(9, 8)->owner = DK_PLAYER_RED;

    for (i = 0; i < 2; ++i) {
        DK_add_unit(DK_PLAYER_RED, DK_UNIT_IMP, 5, 10);
    }
}

void DK_map_save(const char* filename) {

}

///////////////////////////////////////////////////////////////////////////////
// Accessors
///////////////////////////////////////////////////////////////////////////////

DK_Block* DK_block_at(int x, int y) {
    if (x >= 0 && y >= 0 && x < DK_map_size && y < DK_map_size) {
        return &map[y * DK_map_size + x];
    }
    return NULL;
}

int DK_block_coordinates(unsigned short* x, unsigned short* y, const DK_Block* block) {
    if (block) {
        unsigned int idx = block - map;
        *x = idx % DK_map_size;
        *y = idx / DK_map_size;
        return 1;
    }
    return 0;
}

DK_Block* DK_block_under_cursor(int* block_x, int* block_y) {
    *block_x = cursor_x;
    *block_y = cursor_y;
    return DK_block_at(cursor_x, cursor_y);
}

int DK_block_is_fluid(const DK_Block* block) {
    return block && (block->type == DK_BLOCK_LAVA || block->type == DK_BLOCK_WATER);
}

int DK_block_is_wall(const DK_Block* block, DK_Player player) {
    return block && (block->type == DK_BLOCK_DIRT) && (block->owner == player);
}

int DK_block_is_open(const DK_Block* block) {
    return block && (block->type == DK_BLOCK_NONE || DK_block_is_fluid(block));
}

int DK_block_is_passable(const DK_Block* block, const DK_Unit* unit) {
    if (block) {
        switch (block->type) {
            case DK_BLOCK_WATER:
                return 1;
            case DK_BLOCK_LAVA:
                return DK_unit_immune_to_lava(unit);
            case DK_BLOCK_NONE:
                if (block->room == DK_ROOM_DOOR) {
                    return block->owner == DK_unit_owner(unit) && !block->closed;
                } else {
                    return 1;
                }
            default:
                break;
        }
    }
    return 0;
}

int DK_block_damage(DK_Block* block, unsigned int damage) {
    // Already destroyed (nothing to do)?
    if (block->health == 0) {
        return 1;
    }

    // Check if this is the final blow.
    if (block->health > damage) {
        block->health -= damage;
        return 0;
    }

    // Block is destroyed.
    block->health = 0;
    block->type = DK_BLOCK_NONE;
    block->owner = DK_PLAYER_NONE;
    block->strength = DK_BLOCK_NONE_STRENGTH;

    // Update visual representation of the surroundings.
    update_block(block);

    return 1;
}

int DK_block_convert(DK_Block* block, unsigned int strength, DK_Player player) {
    // Get the actual coordinates.
    unsigned short x = 0, y = 0;
    DK_block_coordinates(&x, &y, block);

    // First reduce any enemy influence.
    if (block->owner != player) {
        // Not this player's, reduce strength.
        if (block->strength > strength) {
            block->strength -= strength;
            return 0;
        }

        // Block is completely converted.
        DK_block_set_owner(block, player);

        // See if we're now cornering a neutral block. In that case we automatically
        // convert that one, too.
        if (block->type == DK_BLOCK_DIRT) {

            enum {
                TOP_LEFT,
                TOP,
                TOP_RIGHT,
                LEFT,
                RIGHT,
                BOTTOM_LEFT,
                BOTTOM,
                BOTTOM_RIGHT
            };

            DK_Block * neighbors[8] = {
                [TOP_LEFT] = DK_block_at(x - 1, y - 1),
                [TOP] = DK_block_at(x, y - 1),
                [TOP_RIGHT] = DK_block_at(x + 1, y - 1),
                [LEFT] = DK_block_at(x - 1, y),
                [RIGHT] = DK_block_at(x + 1, y),
                [BOTTOM_LEFT] = DK_block_at(x - 1, y + 1),
                [BOTTOM] = DK_block_at(x, y + 1),
                [BOTTOM_RIGHT] = DK_block_at(x + 1, y + 1),
            };

            if (neighbors[TOP_LEFT] &&
                    neighbors[TOP_LEFT]->type == DK_BLOCK_DIRT &&
                    neighbors[TOP_LEFT]->owner == player) {
                // Top left already owned.
                if (neighbors[TOP]->type == DK_BLOCK_NONE &&
                        neighbors[TOP]->owner == player &&
                        neighbors[LEFT]->type == DK_BLOCK_DIRT &&
                        neighbors[LEFT]->owner == DK_PLAYER_NONE) {
                    DK_block_set_owner(neighbors[LEFT], player);
                }
                if (neighbors[LEFT]->type == DK_BLOCK_NONE &&
                        neighbors[LEFT]->owner == player &&
                        neighbors[TOP]->type == DK_BLOCK_DIRT &&
                        neighbors[TOP]->owner == DK_PLAYER_NONE) {
                    DK_block_set_owner(neighbors[TOP], player);
                }
            }
            if (neighbors[TOP_RIGHT] &&
                    neighbors[TOP_RIGHT]->type == DK_BLOCK_DIRT &&
                    neighbors[TOP_RIGHT]->owner == player) {
                // Top left already owned.
                if (neighbors[TOP]->type == DK_BLOCK_NONE &&
                        neighbors[TOP]->owner == player &&
                        neighbors[RIGHT]->type == DK_BLOCK_DIRT &&
                        neighbors[RIGHT]->owner == DK_PLAYER_NONE) {
                    DK_block_set_owner(neighbors[RIGHT], player);
                }
                if (neighbors[RIGHT]->type == DK_BLOCK_NONE &&
                        neighbors[RIGHT]->owner == player &&
                        neighbors[TOP]->type == DK_BLOCK_DIRT &&
                        neighbors[TOP]->owner == DK_PLAYER_NONE) {
                    DK_block_set_owner(neighbors[TOP], player);
                }
            }
            if (neighbors[BOTTOM_LEFT] &&
                    neighbors[BOTTOM_LEFT]->type == DK_BLOCK_DIRT &&
                    neighbors[BOTTOM_LEFT]->owner == player) {
                // Top left already owned.
                if (neighbors[BOTTOM]->type == DK_BLOCK_NONE &&
                        neighbors[BOTTOM]->owner == player &&
                        neighbors[LEFT]->type == DK_BLOCK_DIRT &&
                        neighbors[LEFT]->owner == DK_PLAYER_NONE) {
                    DK_block_set_owner(neighbors[LEFT], player);
                }
                if (neighbors[LEFT]->type == DK_BLOCK_NONE &&
                        neighbors[LEFT]->owner == player &&
                        neighbors[BOTTOM]->type == DK_BLOCK_DIRT &&
                        neighbors[BOTTOM]->owner == DK_PLAYER_NONE) {
                    DK_block_set_owner(neighbors[BOTTOM], player);
                }
            }
            if (neighbors[BOTTOM_RIGHT] &&
                    neighbors[BOTTOM_RIGHT]->type == DK_BLOCK_DIRT &&
                    neighbors[BOTTOM_RIGHT]->owner == player) {
                // Top left already owned.
                if (neighbors[BOTTOM]->type == DK_BLOCK_NONE &&
                        neighbors[BOTTOM]->owner == player &&
                        neighbors[RIGHT]->type == DK_BLOCK_DIRT &&
                        neighbors[RIGHT]->owner == DK_PLAYER_NONE) {
                    DK_block_set_owner(neighbors[RIGHT], player);
                }
                if (neighbors[RIGHT]->type == DK_BLOCK_NONE &&
                        neighbors[RIGHT]->owner == player &&
                        neighbors[BOTTOM]->type == DK_BLOCK_DIRT &&
                        neighbors[BOTTOM]->owner == DK_PLAYER_NONE) {
                    DK_block_set_owner(neighbors[BOTTOM], player);
                }
            }
        }
    } else {
        // Owned by this player, repair it.
        const unsigned int max_strength = block->type == DK_BLOCK_DIRT ?
                DK_BLOCK_DIRT_OWNED_STRENGTH :
                DK_BLOCK_NONE_OWNED_STRENGTH;
        if (block->strength + strength < max_strength) {
            block->strength += strength;
            return 0;
        }

        // Completely repaired.
        block->strength = max_strength;

        // Update jobs nearby.
        DK_FindJobs(player, x, y);
    }
    return 1;
}

void DK_block_set_type(DK_Block* block, DK_BlockType type) {
    block->type = type;
    update_block(block);
}

void DK_block_set_owner(DK_Block* block, DK_Player player) {
    block->owner = player;
    update_block(block);
}

#undef XYZD2I
#undef XYZ2IF
#undef XYZ2IH

#undef DK_NEQ
#undef DK_NLT
