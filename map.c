#include <math.h>
#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glext.h>
#include <string.h>
#include <malloc.h>

#include "config.h"
#include "map.h"
#include "simplexnoise.h"
#include "textures.h"
#include "camera.h"

///////////////////////////////////////////////////////////////////////////////
// Internal variables
///////////////////////////////////////////////////////////////////////////////

/** Last normal that was set */
static double last_normal[3] = {0, 0, 0};

/** Last vertex that was set */
static double last_vertex[3] = {0, 0, 0};

/** The current map */
static DK_Block* map = 0;

/** Cache of map noise */
static double* map_noise = 0;

/**
 * Selected blocks, per player.
 * -1 because NONE cannot select (white can!).
 * This is a bitset, where each bit represents a block in the map.
 */
static char* selection[DK_PLAYER_COUNT - 1] = {0};

///////////////////////////////////////////////////////////////////////////////
// Internal rendering stuff
///////////////////////////////////////////////////////////////////////////////

/** Get cached noise for the specified offset */
static inline double* noise_at_index(int x, int y, int z, int k) {
    return &map_noise[((k * 3 + z) * 3 + y) * (DK_map_size + 1) + x];
}

/** Noise at the specified world coordinates (same args as for snoise4) */
static double noise_at(double x, double y, double z, int k) {
    int i = (int) round((x * 2) / DK_BLOCK_SIZE),
            j = (int) round((y * 2) / DK_BLOCK_SIZE),
            l = (int) round((z * 2) / DK_BLOCK_HEIGHT);

    // Wrap for off-the-grid terrain (rocks outside actual map)
    while (i < 0) {
        i += DK_map_size;
    }
    while (i > DK_map_size * 2 + 1) {
        i -= DK_map_size * 2;
    }
    while (j < 0) {
        j += DK_map_size;
    }
    while (j > DK_map_size * 2 + 1) {
        j -= DK_map_size * 2;
    }

    // Lookup noise.
    return *noise_at_index(i, j, l, k);
}

/** Used to differentiate how blocks effect offsetting */
typedef enum {
    OFFSET_NONE,
    OFFSET_INCREASE,
    OFFSET_DECREASE
} DK_Offset;

/** Get influence of a specific block on noise */
static double noise_factor_block(DK_Offset* offset, const DK_Block* block) {
    *offset = OFFSET_NONE;
    if (DK_block_is_passable(block) && block->owner == DK_PLAYER_NONE) {
        *offset = OFFSET_INCREASE;
        return 1.5;
    } else if (block->owner != DK_PLAYER_NONE) {
        *offset = OFFSET_DECREASE;
        return 0.5;
    }
    return 1;
}

#define DK_NEQ(a, b) (fabs(b - (a + 0.5)) < 0.01f)
#define DK_NLT(a, b) (b - (a + 0.5) > 0)

/**
 * Gets noise factor (amplitude) based on neighboring blocks.
 * Also gets a directed offset based on where the neighboring block are (away
 * from empty blocks).
 */
static double noise_factor(double* offset, double x, double y) {
    DK_Offset offset_type;
    double offset_reduction[2] = {1, 1};

    // Get full coordinate for our doubles (which might be at half coordinates.
    //const int i = ((int) (x * 2)) >> 1, j = ((int) (y * 2)) >> 1;
    const int i = (int) (x + 0.1), j = (int) (y + 0.1);

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
    double normalizer;
    if (fabs(x - i - 0.5f) < 0.01f) {
        // X is halfway, so we're at 1 or 3.
        x_end = i + 1;
        if (fabs(y - j - 0.5f) < 0.01f) {
            // Y is halfway, so we're at 3, the most expensive case.
            y_end = j + 1;
            normalizer = 9.0;
        } else {
            // Y is full, so we're at 1.
            y_end = j;
            normalizer = 6.0;
        }
    } else {
        // X is full, so we're at 0 or 2.
        x_end = i;
        if (fabs(y - j - 0.5f) < 0.01f) {
            // Y is halfway, so we're at 2.
            y_end = j + 1;
            normalizer = 6.0;
        } else {
            // Y is full, so we're at 0.
            y_end = j;
            normalizer = 4.0;
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
    double factor = 0;
    int k, l;
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

    if (offset[0] != 0 || offset[1] != 0) {
        offset[0] *= offset_reduction[0] / normalizer;
        offset[1] *= offset_reduction[1] / normalizer;
        const float len = sqrt(offset[0] * offset[0] + offset[1] * offset[1]);
        if (len > 1) {
            offset[0] /= len;
            offset[1] *= len;
        }
    }

    return factor / normalizer;
}

#undef DK_NEQ
#undef DK_NLT

/** Get the actual vertex for a "clean" world coordinate (point) */
static void DK_get_vertex(double* v, const double* points, int point) {
    const double* p = &points[point * 3];
#if DK_D_TERRAIN_NOISE
    double offset[2] = {0, 0};
#if DK_D_USE_NOISE_OFFSET
    const double factor = noise_factor(offset, p[0] / DK_BLOCK_SIZE, p[1] / DK_BLOCK_SIZE);
#else
    const double factor = 1.0;
#endif
    const double offset_factor = 2 * (0.75 - fabs(p[2] / DK_BLOCK_HEIGHT - 0.5));
    offset[0] *= offset_factor;
    offset[1] *= offset_factor;
#if DK_D_DRAW_NOISE_FACTOR
    const double len = sqrt(offset[0] * offset[0] + offset[1] * offset[1]);
    glColor3d(len, 0.25, 0.25);
#endif
    offset[0] *= DK_BLOCK_MAX_NOISE_OFFSET;
    offset[1] *= DK_BLOCK_MAX_NOISE_OFFSET;
#if DK_D_CACHE_NOISE
    v[0] = p[0] + factor * noise_at(p[0], p[1], p[2], 0) + offset[0];
    v[1] = p[1] + factor * noise_at(p[0], p[1], p[2], 1) + offset[1];
    v[2] = p[2] + noise_at(p[0], p[1], p[2], 2);
#else
    v[0] = p[0] + factor * snoise4(p[0], p[1], p[2], 0) + offset[0];
    v[1] = p[1] + factor * snoise4(p[0], p[1], p[2], 1) + offset[1];
    v[2] = p[2] + snoise4(p[0], p[1], p[2], 2);
#endif
#else
    v[0] = p[0];
    v[1] = p[1];
    v[2] = p[2];
#endif
}

/** Set a vertex in opengl */
inline static void gl_set_vertex(const double* points, int point) {
    DK_get_vertex(last_vertex, points, point);
    glVertex3dv(last_vertex);
}

/** Computes the cross product of two 3d vectors */
inline static void cross(double* cross, const double* v0, const double* v1) {
    cross[0] = v0[1] * v1[2] - v0[2] * v1[1];
    cross[1] = v0[2] * v1[0] - v0[0] * v1[2];
    cross[2] = v0[0] * v1[1] - v0[1] * v1[0];
}

/** Gets normal for a vertex (two neighboring vertices) */
static void normal2(const double* points, int i0, int i1, int i2) {
    double p0[3], p1[3], p2[3];
    DK_get_vertex(p0, points, i0);
    DK_get_vertex(p1, points, i1);
    DK_get_vertex(p2, points, i2);
    const double a[] = {
        p1[0] - p0[0],
        p1[1] - p0[1],
        p1[2] - p0[2]
    };
    const double b[] = {
        p2[0] - p0[0],
        p2[1] - p0[1],
        p2[2] - p0[2]
    };

    double* n = last_normal;
    cross(n, a, b);

    const double len = sqrt(n[0] * n[0] + n[1] * n[1] + n[2] * n[2]);
    n[0] /= len;
    n[1] /= len;
    n[2] /= len;

    glNormal3dv(n);
}

/** Gets normal for a vertex (three neighboring vertices) */
static void normal3(const double* points, int i0, int i1, int i2, int i3) {
    double p0[3], p1[3], p2[3], p3[3];
    DK_get_vertex(p0, points, i0);
    DK_get_vertex(p1, points, i1);
    DK_get_vertex(p2, points, i2);
    DK_get_vertex(p3, points, i3);
    const double a[] = {
        p1[0] - p0[0],
        p1[1] - p0[1],
        p1[2] - p0[2]
    };
    const double b[] = {
        p2[0] - p0[0],
        p2[1] - p0[1],
        p2[2] - p0[2]
    };
    const double c[] = {
        p3[0] - p0[0],
        p3[1] - p0[1],
        p3[2] - p0[2]
    };

    double* n = last_normal;
    double t[3];
    cross(t, a, b);
    cross(n, b, c);
    n[0] = (n[0] + t[0]) / 2.0;
    n[1] = (n[1] + t[1]) / 2.0;
    n[2] = (n[2] + t[2]) / 2.0;

    const double len = sqrt(n[0] * n[0] + n[1] * n[1] + n[2] * n[2]);
    n[0] /= len;
    n[1] /= len;
    n[2] /= len;

    glNormal3dv(n);
}

/** Gets normal for a vertex (four neighboring vertices) */
static void normal4(const double* points, int i0, int i1, int i2, int i3, int i4) {
    double p0[3], p1[3], p2[3], p3[3], p4[3];
    DK_get_vertex(p0, points, i0);
    DK_get_vertex(p1, points, i1);
    DK_get_vertex(p2, points, i2);
    DK_get_vertex(p3, points, i3);
    DK_get_vertex(p4, points, i4);
    const double a[] = {
        p1[0] - p0[0],
        p1[1] - p0[1],
        p1[2] - p0[2]
    };
    const double b[] = {
        p2[0] - p0[0],
        p2[1] - p0[1],
        p2[2] - p0[2]
    };
    const double c[] = {
        p3[0] - p0[0],
        p3[1] - p0[1],
        p3[2] - p0[2]
    };
    const double d[] = {
        p4[0] - p0[0],
        p4[1] - p0[1],
        p4[2] - p0[2]
    };

    double* n = last_normal;
    double t0[3], t1[3];
    cross(t0, a, b);
    cross(t1, b, c);
    cross(n, c, d);
    n[0] = (n[0] + t0[0] + t1[0]) / 3.0;
    n[1] = (n[1] + t0[1] + t1[0]) / 3.0;
    n[2] = (n[2] + t0[2] + t1[0]) / 3.0;

    const double len = sqrt(n[0] * n[0] + n[1] * n[1] + n[2] * n[2]);
    n[0] /= len;
    n[1] /= len;
    n[2] /= len;

    glNormal3dv(n);
}

/** Nesting glBegin() is not possible, so we need to store the normals to draw them */
#if DK_D_DRAW_NORMALS
#define DK_D_SAVE_NORMALS()\
        memcpy(&v[3 * ni], last_vertex, sizeof (last_vertex));\
        memcpy(&n[3 * ni++], last_normal, sizeof (last_normal))
#else
#define DK_D_SAVE_NORMALS()
#endif

/**
 * Draw a quad subdivided into 4 quads like so:
 * 
 * 0 - 1 - 2
 * | / | / |
 * 3 - 4 - 5
 * | / | / |
 * 6 - 7 - 8
 * 
 * Where the points argument must contain these 3d positions in the specified
 * order.
 */
static void DK_draw_4quad(DK_Texture texture, double* points) {
#ifdef DK_D_DRAW_NORMALS
    double v[3 * 6], n[3 * 6];
    int ni = 0;
#endif

    const unsigned int variation = (unsigned int) ((snoise2(points[0], points[1] + points[2]) + 1) / 2 * DK_TEX_MAX_VARIATIONS);
    const GLuint tex = DK_opengl_texture(texture, variation);
    glBindTexture(GL_TEXTURE_2D, tex);
    glBegin(GL_QUAD_STRIP);
    {
#if DK_D_COMPUTE_NORMALS
        normal2(points, 0, 3, 1);
#endif
        glTexCoord2d(0, 0);
        gl_set_vertex(points, 0);

        DK_D_SAVE_NORMALS();

#if DK_D_COMPUTE_NORMALS
        normal3(points, 3, 4, 1, 0);
#endif
        glTexCoord2d(0, 0.5);
        gl_set_vertex(points, 3);

        DK_D_SAVE_NORMALS();

#if DK_D_COMPUTE_NORMALS
        normal4(points, 1, 0, 3, 4, 2);
#endif
        glTexCoord2d(0.5, 0);
        gl_set_vertex(points, 1);

        DK_D_SAVE_NORMALS();

#if DK_D_COMPUTE_NORMALS
        normal4(points, 4, 5, 2, 1, 3);
#endif
        glTexCoord2d(0.5, 0.5);
        gl_set_vertex(points, 4);

        DK_D_SAVE_NORMALS();

#if DK_D_COMPUTE_NORMALS
        normal3(points, 2, 1, 4, 5);
#endif
        glTexCoord2d(1, 0);
        gl_set_vertex(points, 2);

        DK_D_SAVE_NORMALS();

#if DK_D_COMPUTE_NORMALS
        normal2(points, 5, 2, 4);
#endif
        glTexCoord2d(1, 0.5);
        gl_set_vertex(points, 5);

        DK_D_SAVE_NORMALS();
    }
    glEnd();

#if DK_D_DRAW_NORMALS
    const GLboolean lighting = glIsEnabled(GL_LIGHTING);
    glDisable(GL_LIGHTING);

    glBindTexture(GL_TEXTURE_2D, 0);
    glBegin(GL_LINES);
    {
        glColor3f(1.0, 0.0, 0.0);
        for (ni--; ni >= 0; --ni) {
            glVertex3f(
                    v[3 * ni] + n[3 * ni] * 2,
                    v[3 * ni + 1] + n[3 * ni + 1] * 2,
                    v[3 * ni + 2] + n[3 * ni + 2] * 2);
            glVertex3dv(&v[3 * ni]);
        }
    }
    glColor3f(1.0, 1.0, 1.0);
    glEnd();

    if (lighting) {
        glEnable(GL_LIGHTING);
    }

    ni = 0;
#endif

    glBindTexture(GL_TEXTURE_2D, tex);
    glBegin(GL_QUAD_STRIP);
    {
#if DK_D_COMPUTE_NORMALS
        normal2(points, 3, 6, 4);
#endif
        glTexCoord2d(0, 0.5);
        gl_set_vertex(points, 3);

        DK_D_SAVE_NORMALS();

#if DK_D_COMPUTE_NORMALS
        normal3(points, 6, 7, 4, 3);
#endif
        glTexCoord2d(0, 1);
        gl_set_vertex(points, 6);

        DK_D_SAVE_NORMALS();

#if DK_D_COMPUTE_NORMALS
        normal4(points, 4, 3, 6, 7, 5);
#endif
        glTexCoord2d(0.5, 0.5);
        gl_set_vertex(points, 4);

        DK_D_SAVE_NORMALS();

#if DK_D_COMPUTE_NORMALS
        normal4(points, 7, 8, 5, 4, 6);
#endif
        glTexCoord2d(0.5, 1);
        gl_set_vertex(points, 7);

        DK_D_SAVE_NORMALS();

#if DK_D_COMPUTE_NORMALS
        normal3(points, 5, 4, 7, 8);
#endif
        glTexCoord2d(1, 0.5);
        gl_set_vertex(points, 5);

        DK_D_SAVE_NORMALS();

#if DK_D_COMPUTE_NORMALS
        normal2(points, 8, 5, 7);
#endif
        glTexCoord2d(1, 1);
        gl_set_vertex(points, 8);

        DK_D_SAVE_NORMALS();
    }
    glEnd();

#if DK_D_DRAW_NORMALS
    glDisable(GL_LIGHTING);

    glBindTexture(GL_TEXTURE_2D, 0);
    glBegin(GL_LINES);
    {
        glColor3f(1.0, 0.0, 0.0);
        for (ni--; ni >= 0; --ni) {
            glVertex3f(
                    v[3 * ni] + n[3 * ni] * 2,
                    v[3 * ni + 1] + n[3 * ni + 1] * 2,
                    v[3 * ni + 2] + n[3 * ni + 2] * 2);
            glVertex3dv(&v[3 * ni]);
        }
    }
    glColor3f(1.0, 1.0, 1.0);
    glEnd();

    if (lighting) {
        glEnable(GL_LIGHTING);
    }
#endif
}

#undef DK_D_SAVE_NORMALS

/** Set a point in a point list to the specified values */
inline static void set_points(double* points, int point, double x, double y, double z) {
    points[point * 3] = x;
    points[point * 3 + 1] = y;
    points[point * 3 + 2] = z;
}

///////////////////////////////////////////////////////////////////////////////
// Header implementation
///////////////////////////////////////////////////////////////////////////////

void DK_init_map(unsigned int size) {
    DK_map_size = size;

    free(map);
    map = calloc(size * size, sizeof (DK_Block));

    int i, j, k, l;
    for (i = 0; i < size; ++i) {
        for (j = 0; j < size; ++j) {
            if (i == 0 || j == 0 || i == size - 1 || j == size - 1) {
                map[i + j * size].type = DK_BLOCK_ROCK;
            } else {
                map[i + j * size].type = DK_BLOCK_DIRT;
            }
        }
    }

    for (i = 0; i < DK_PLAYER_COUNT - 1; ++i) {
        free(selection[i]);
        selection[i] = calloc((DK_map_size * DK_map_size) / 8 + 1, sizeof (char));
    }

#if DK_D_CACHE_NOISE
    free(map_noise);
    map_noise = calloc((size * 2 + 1) * (size * 2 + 1) * 3 * 3, sizeof (double));
    for (i = 0; i < size * 2 + 1; ++i) {
        for (j = 0; j < size * 2 + 1; ++j) {
            for (k = 0; k < 3; ++k) {
                for (l = 0; l < 3; ++l) {
                    *noise_at_index(i, j, k, l) = snoise4(i * DK_BLOCK_SIZE / 2.0, j * DK_BLOCK_SIZE / 2.0, k / 2.0 * DK_BLOCK_HEIGHT, l);
                }
            }
        }
    }
#endif
}

DK_Block* DK_block_at(unsigned int x, unsigned int y) {
    return &map[y * DK_map_size + x];
}

int DK_block_is_fluid(const DK_Block* block) {
    return block->type == DK_BLOCK_LAVA || block->type == DK_BLOCK_WATER;
}

int DK_block_is_passable(const DK_Block* block) {
    return block->type == DK_BLOCK_NONE || DK_block_is_fluid(block);
}

void DK_render_map() {
    int x_begin = (int) (DK_camera_position()[0] / DK_BLOCK_SIZE) - DK_RENDER_AREA / 2;
    int y_begin = (int) (DK_camera_position()[1] / DK_BLOCK_SIZE) - DK_RENDER_AREA / 2 + DK_RENDER_OFFSET;
    int x_end = x_begin + DK_RENDER_AREA;
    int y_end = y_begin + DK_RENDER_AREA;
    int x, y;
    for (x = x_begin; x < x_end; ++x) {
        float x_coord = x * (DK_BLOCK_SIZE);

        for (y = y_begin; y < y_end; ++y) {
            float y_coord = y * (DK_BLOCK_SIZE);

            // Reset tint color.
            glColor3f(1.0f, 1.0f, 1.0f);

            DK_Texture texture_top, texture_side, texture_top_wall = 0, texture_top_owner = 0;
            int top;
            if (x < 0 || y < 0 || x >= DK_map_size || y >= DK_map_size) {

                // Invalidate this in select mode.
                glLoadName(0);

                // Solid rock when out of bounds.
                top = DK_BLOCK_HEIGHT;
                texture_top = DK_TEX_ROCK_TOP;
                texture_side = DK_TEX_ROCK_SIDE;
            } else {
                const DK_Block* block = &map[x + y * DK_map_size];

                // Mark for select mode, push pointer to that block.
                glLoadName((GLuint) block);

                // Selected by the local player?
                if (DK_block_selected(DK_PLAYER_RED, x, y)) {
                    glColor3f(0.8f, 0.8f, 1.0f);
                }

                if (block->type == DK_BLOCK_NONE) {
                    // Render floor.
                    top = 0;
                    if (block->owner == DK_PLAYER_NONE) {
                        texture_top = DK_TEX_DIRT_FLOOR;
                    } else {
                        texture_top = DK_TEX_FLOOR;
                        texture_top_owner = DK_TEX_OWNER_RED;
                    }
                } else if (block->type == DK_BLOCK_WATER ||
                        block->type == DK_BLOCK_LAVA) {
                    top = -DK_WATER_LEVEL;
                    texture_top = DK_TEX_DIRT_FLOOR;
                } else {
                    // Render block top.
                    top = DK_BLOCK_HEIGHT;
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
                            continue;
                            break;
                        case DK_BLOCK_GEM:
                            continue;
                            break;
                        case DK_BLOCK_ROCK:
                            // Solid rock, cannot be owned.
                            texture_top = DK_TEX_ROCK_TOP;
                            texture_side = DK_TEX_ROCK_SIDE;
                            break;
                    }
                }
            }

            /*
             * 0 - 1 - 2
             * | / | / |
             * 3 - 4 - 5
             * | / | / |
             * 6 - 7 - 8
             */
            double points[3 * 9];

            set_points(points, 0, x_coord, y_coord + DK_BLOCK_SIZE, top);
            set_points(points, 1, x_coord + DK_BLOCK_SIZE / 2.0, y_coord + DK_BLOCK_SIZE, top);
            set_points(points, 2, x_coord + DK_BLOCK_SIZE, y_coord + DK_BLOCK_SIZE, top);
            set_points(points, 3, x_coord, y_coord + DK_BLOCK_SIZE / 2.0, top);
            set_points(points, 4, x_coord + DK_BLOCK_SIZE / 2.0, y_coord + DK_BLOCK_SIZE / 2.0, top);
            set_points(points, 5, x_coord + DK_BLOCK_SIZE, y_coord + DK_BLOCK_SIZE / 2.0, top);
            set_points(points, 6, x_coord, y_coord, top);
            set_points(points, 7, x_coord + DK_BLOCK_SIZE / 2.0, y_coord, top);
            set_points(points, 8, x_coord + DK_BLOCK_SIZE, y_coord, top);

            DK_draw_4quad(texture_top, points);

            if (texture_top_wall || texture_top_owner) {
                glEnable(GL_BLEND);
                //glDisable(GL_DEPTH_TEST);
                glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

                if (texture_top_wall) {
                    DK_draw_4quad(texture_top_wall, points);
                }
                if (texture_top_owner) {
                    DK_draw_4quad(texture_top_owner, points);
                }

                glDisable(GL_BLEND);
                //glEnable(GL_DEPTH_TEST);
            }

            // Check if we need to render walls.
            if (top > 0) {
                // North wall.
                if (y + 1 < DK_map_size && DK_block_is_passable(&map[x + (y + 1) * DK_map_size])) {
                    set_points(points, 0, x_coord + DK_BLOCK_SIZE, y_coord + DK_BLOCK_SIZE, DK_BLOCK_HEIGHT);
                    set_points(points, 1, x_coord + DK_BLOCK_SIZE / 2.0, y_coord + DK_BLOCK_SIZE, DK_BLOCK_HEIGHT);
                    set_points(points, 2, x_coord, y_coord + DK_BLOCK_SIZE, DK_BLOCK_HEIGHT);
                    set_points(points, 3, x_coord + DK_BLOCK_SIZE, y_coord + DK_BLOCK_SIZE, DK_BLOCK_HEIGHT / 2.0);
                    set_points(points, 4, x_coord + DK_BLOCK_SIZE / 2.0, y_coord + DK_BLOCK_SIZE, DK_BLOCK_HEIGHT / 2.0);
                    set_points(points, 5, x_coord, y_coord + DK_BLOCK_SIZE, DK_BLOCK_HEIGHT / 2.0);
                    set_points(points, 6, x_coord + DK_BLOCK_SIZE, y_coord + DK_BLOCK_SIZE, 0);
                    set_points(points, 7, x_coord + DK_BLOCK_SIZE / 2.0, y_coord + DK_BLOCK_SIZE, 0);
                    set_points(points, 8, x_coord, y_coord + DK_BLOCK_SIZE, 0);

                    DK_draw_4quad(texture_side, points);
                }

                // South wall.
                if (y > 0 && DK_block_is_passable(&map[x + (y - 1) * DK_map_size])) {
                    set_points(points, 0, x_coord, y_coord, DK_BLOCK_HEIGHT);
                    set_points(points, 1, x_coord + DK_BLOCK_SIZE / 2.0, y_coord, DK_BLOCK_HEIGHT);
                    set_points(points, 2, x_coord + DK_BLOCK_SIZE, y_coord, DK_BLOCK_HEIGHT);
                    set_points(points, 3, x_coord, y_coord, DK_BLOCK_HEIGHT / 2.0);
                    set_points(points, 4, x_coord + DK_BLOCK_SIZE / 2.0, y_coord, DK_BLOCK_HEIGHT / 2.0);
                    set_points(points, 5, x_coord + DK_BLOCK_SIZE, y_coord, DK_BLOCK_HEIGHT / 2.0);
                    set_points(points, 6, x_coord, y_coord, 0);
                    set_points(points, 7, x_coord + DK_BLOCK_SIZE / 2.0, y_coord, 0);
                    set_points(points, 8, x_coord + DK_BLOCK_SIZE, y_coord, 0);

                    DK_draw_4quad(texture_side, points);
                }

                // East wall.
                if (x + 1 < DK_map_size && DK_block_is_passable(&map[(x + 1) + y * DK_map_size])) {
                    set_points(points, 0, x_coord + DK_BLOCK_SIZE, y_coord, DK_BLOCK_HEIGHT);
                    set_points(points, 1, x_coord + DK_BLOCK_SIZE, y_coord + DK_BLOCK_SIZE / 2.0, DK_BLOCK_HEIGHT);
                    set_points(points, 2, x_coord + DK_BLOCK_SIZE, y_coord + DK_BLOCK_SIZE, DK_BLOCK_HEIGHT);
                    set_points(points, 3, x_coord + DK_BLOCK_SIZE, y_coord, DK_BLOCK_HEIGHT / 2.0);
                    set_points(points, 4, x_coord + DK_BLOCK_SIZE, y_coord + DK_BLOCK_SIZE / 2.0, DK_BLOCK_HEIGHT / 2.0);
                    set_points(points, 5, x_coord + DK_BLOCK_SIZE, y_coord + DK_BLOCK_SIZE, DK_BLOCK_HEIGHT / 2.0);
                    set_points(points, 6, x_coord + DK_BLOCK_SIZE, y_coord, 0);
                    set_points(points, 7, x_coord + DK_BLOCK_SIZE, y_coord + DK_BLOCK_SIZE / 2.0, 0);
                    set_points(points, 8, x_coord + DK_BLOCK_SIZE, y_coord + DK_BLOCK_SIZE, 0);

                    DK_draw_4quad(texture_side, points);
                }

                // West wall.
                if (x > 0 && DK_block_is_passable(&map[(x - 1) + y * DK_map_size])) {
                    set_points(points, 0, x_coord, y_coord + DK_BLOCK_SIZE, DK_BLOCK_HEIGHT);
                    set_points(points, 1, x_coord, y_coord + DK_BLOCK_SIZE / 2.0, DK_BLOCK_HEIGHT);
                    set_points(points, 2, x_coord, y_coord, DK_BLOCK_HEIGHT);
                    set_points(points, 3, x_coord, y_coord + DK_BLOCK_SIZE, DK_BLOCK_HEIGHT / 2.0);
                    set_points(points, 4, x_coord, y_coord + DK_BLOCK_SIZE / 2.0, DK_BLOCK_HEIGHT / 2.0);
                    set_points(points, 5, x_coord, y_coord, DK_BLOCK_HEIGHT / 2.0);
                    set_points(points, 6, x_coord, y_coord + DK_BLOCK_SIZE, 0);
                    set_points(points, 7, x_coord, y_coord + DK_BLOCK_SIZE / 2.0, 0);
                    set_points(points, 8, x_coord, y_coord, 0);

                    DK_draw_4quad(texture_side, points);
                }
            }

            // Check if we need to render water walls.
            if (top < 0) {
                // North wall.
                if (y + 1 < DK_map_size && !DK_block_is_fluid(&map[x + (y + 1) * DK_map_size])) {
                    set_points(points, 0, x_coord, y_coord + DK_BLOCK_SIZE, 0);
                    set_points(points, 1, x_coord + DK_BLOCK_SIZE / 2.0, y_coord + DK_BLOCK_SIZE, 0);
                    set_points(points, 2, x_coord + DK_BLOCK_SIZE, y_coord + DK_BLOCK_SIZE, 0);
                    set_points(points, 3, x_coord, y_coord + DK_BLOCK_SIZE, top / 2.0);
                    set_points(points, 4, x_coord + DK_BLOCK_SIZE / 2.0, y_coord + DK_BLOCK_SIZE, top / 2.0);
                    set_points(points, 5, x_coord + DK_BLOCK_SIZE, y_coord + DK_BLOCK_SIZE, top / 2.0);
                    set_points(points, 6, x_coord, y_coord + DK_BLOCK_SIZE, top);
                    set_points(points, 7, x_coord + DK_BLOCK_SIZE / 2.0, y_coord + DK_BLOCK_SIZE, top);
                    set_points(points, 8, x_coord + DK_BLOCK_SIZE, y_coord + DK_BLOCK_SIZE, top);

                    DK_draw_4quad(DK_TEX_FLUID_SIDE, points);
                }

                // South wall.
                if (y > 0 && !DK_block_is_fluid(&map[x + (y - 1) * DK_map_size])) {
                    set_points(points, 0, x_coord + DK_BLOCK_SIZE, y_coord, 0);
                    set_points(points, 1, x_coord + DK_BLOCK_SIZE / 2.0, y_coord, 0);
                    set_points(points, 2, x_coord, y_coord, 0);
                    set_points(points, 3, x_coord + DK_BLOCK_SIZE, y_coord, top / 2.0);
                    set_points(points, 4, x_coord + DK_BLOCK_SIZE / 2.0, y_coord, top / 2.0);
                    set_points(points, 5, x_coord, y_coord, top / 2.0);
                    set_points(points, 6, x_coord + DK_BLOCK_SIZE, y_coord, top);
                    set_points(points, 7, x_coord + DK_BLOCK_SIZE / 2.0, y_coord, top);
                    set_points(points, 8, x_coord, y_coord, top);

                    DK_draw_4quad(DK_TEX_FLUID_SIDE, points);
                }

                // East wall.
                if (x + 1 < DK_map_size && !DK_block_is_fluid(&map[(x + 1) + y * DK_map_size])) {
                    set_points(points, 0, x_coord + DK_BLOCK_SIZE, y_coord + DK_BLOCK_SIZE, 0);
                    set_points(points, 1, x_coord + DK_BLOCK_SIZE, y_coord + DK_BLOCK_SIZE / 2.0, 0);
                    set_points(points, 2, x_coord + DK_BLOCK_SIZE, y_coord, 0);
                    set_points(points, 3, x_coord + DK_BLOCK_SIZE, y_coord + DK_BLOCK_SIZE, top / 2.0);
                    set_points(points, 4, x_coord + DK_BLOCK_SIZE, y_coord + DK_BLOCK_SIZE / 2.0, top / 2.0);
                    set_points(points, 5, x_coord + DK_BLOCK_SIZE, y_coord, top / 2.0);
                    set_points(points, 6, x_coord + DK_BLOCK_SIZE, y_coord + DK_BLOCK_SIZE, top);
                    set_points(points, 7, x_coord + DK_BLOCK_SIZE, y_coord + DK_BLOCK_SIZE / 2.0, top);
                    set_points(points, 8, x_coord + DK_BLOCK_SIZE, y_coord, top);

                    DK_draw_4quad(DK_TEX_FLUID_SIDE, points);
                }

                // West wall.
                if (x > 0 && !DK_block_is_fluid(&map[(x - 1) + y * DK_map_size])) {
                    set_points(points, 0, x_coord, y_coord, 0);
                    set_points(points, 1, x_coord, y_coord + DK_BLOCK_SIZE / 2.0, 0);
                    set_points(points, 2, x_coord, y_coord + DK_BLOCK_SIZE, 0);
                    set_points(points, 3, x_coord, y_coord, top / 2.0);
                    set_points(points, 4, x_coord, y_coord + DK_BLOCK_SIZE / 2.0, top / 2.0);
                    set_points(points, 5, x_coord, y_coord + DK_BLOCK_SIZE, top / 2.0);
                    set_points(points, 6, x_coord, y_coord, top);
                    set_points(points, 7, x_coord, y_coord + DK_BLOCK_SIZE / 2.0, top);
                    set_points(points, 8, x_coord, y_coord + DK_BLOCK_SIZE, top);

                    DK_draw_4quad(DK_TEX_FLUID_SIDE, points);
                }
            }
        }
    }
}

DK_Block* DK_as_block(void* ptr, int* x, int* y) {
    if (ptr >= (void*) map && ptr < (void*) (map + DK_map_size * DK_map_size)) {
        unsigned int offset = ptr - (void*) map;
        if (offset % sizeof (DK_Block) == 0) {
            offset = (DK_Block*) ptr - map;
            *x = offset % DK_map_size;
            *y = offset / DK_map_size;
        }
        return (DK_Block*) ptr;
    }
    return NULL;
}

void DK_block_select(DK_Player player, unsigned int x, unsigned int y) {
    const DK_Block* block = DK_block_at(x, y);
    if (block->type != DK_BLOCK_DIRT ||
            (block->owner != player && block->owner != DK_PLAYER_NONE)) {
        return;
    }
    const unsigned int idx = y * DK_map_size + x;
    selection[player][idx >> 3] |= (1 << (idx & 7));
}

void DK_block_deselect(DK_Player player, unsigned int x, unsigned int y) {
    const unsigned int idx = y * DK_map_size + x;
    selection[player][idx >> 3] &= ~(1 << (idx & 7));
}

int DK_block_selected(DK_Player player, unsigned int x, unsigned int y) {
    const unsigned int idx = y * DK_map_size + x;
    return (selection[player][idx >> 3] & (1 << (idx & 7))) != 0;
}
