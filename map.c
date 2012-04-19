#include <math.h>
#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glext.h>
#include <string.h>
#include <malloc.h>

#include "bitset.h"
#include "config.h"
#include "map.h"
#include "simplexnoise.h"
#include "textures.h"
#include "camera.h"
#include "selection.h"
#include "jobs.h"

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
static void get_vertex(double* v, const double* points, int point) {
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
inline static void set_vertex(const double* points, int point) {
    get_vertex(last_vertex, points, point);
    glVertex3dv(last_vertex);
}

/** Computes the cross product of two 3d vectors */
inline static void cross(double* cross, const double* v0, const double* v1) {
    cross[0] = v0[1] * v1[2] - v0[2] * v1[1];
    cross[1] = v0[2] * v1[0] - v0[0] * v1[2];
    cross[2] = v0[0] * v1[1] - v0[1] * v1[0];
}

/** Sets normal for a vertex (base don four neighboring vertices) */
static void set_normal(const double* points, int i0, int i1, int i2, int i3, int i4, int snap) {
    double p0[3], p1[3], p2[3], p3[3], p4[3];
    get_vertex(p0, points, i0);
    get_vertex(p1, points, i1);
    get_vertex(p2, points, i2);
    get_vertex(p3, points, i3);
    get_vertex(p4, points, i4);
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

    if (snap) {
        // Determine dominant axis.
        if (fabs(n[0]) > fabs(n[1]) && fabs(n[0]) > fabs(n[2])) {
            n[0] = n[0] > 0.0 ? 1.0 : -1.0;
            n[1] = 0.0;
            n[2] = 0.0;
        } else if (fabs(n[1]) > fabs(n[2])) {
            n[0] = 0.0;
            n[1] = n[1] > 0.0 ? 1.0 : -1.0;
            n[2] = 0.0;
        } else {
            n[0] = 0.0;
            n[1] = 0.0;
            n[2] = n[2] > 0.0 ? 1.0 : -1.0;
        }
    } else {
        // Normalize it.
        const double len = sqrt(n[0] * n[0] + n[1] * n[1] + n[2] * n[2]);
        n[0] /= len;
        n[1] /= len;
        n[2] /= len;
    }

    glNormal3dv(n);
}

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
static void draw_4quad(DK_Texture texture, double* points) {
    // Set surface texture.
    const unsigned int variation = (unsigned int) ((snoise2(points[0], points[1] + points[2]) + 1) / 2 * DK_TEX_MAX_VARIATIONS);
    glBindTexture(GL_TEXTURE_2D, DK_opengl_texture(texture, variation));

    // Top two quads (0 through 5).
    glBegin(GL_QUAD_STRIP);
    {
        // Set normal to predominant axis.
        set_normal(points, 4, 0, 6, 8, 2, 1);

        glTexCoord2d(0, 0);
        set_vertex(points, 0);

        glTexCoord2d(0, 0.5);
        set_vertex(points, 3);

        glTexCoord2d(0.5, 0);
        set_vertex(points, 1);

        // Center vertex, set average normal.
        set_normal(points, 4, 0, 6, 8, 2, 0);

        glTexCoord2d(0.5, 0.5);
        set_vertex(points, 4);

        // And back to predominant one.
        set_normal(points, 4, 0, 6, 8, 2, 1);

        glTexCoord2d(1, 0);
        set_vertex(points, 2);

        glTexCoord2d(1, 0.5);
        set_vertex(points, 5);

    }
    glEnd();

    // Bottom two quads (3 through 8).
    glBegin(GL_QUAD_STRIP);
    {
        glTexCoord2d(0, 0.5);
        set_vertex(points, 3);

        glTexCoord2d(0, 1);
        set_vertex(points, 6);

        // Center vertex, set average normal.
        set_normal(points, 4, 0, 6, 8, 2, 0);

        glTexCoord2d(0.5, 0.5);
        set_vertex(points, 4);

        // And back to predominant one.
        set_normal(points, 4, 0, 6, 8, 2, 1);

        glTexCoord2d(0.5, 1);
        set_vertex(points, 7);

        glTexCoord2d(1, 0.5);
        set_vertex(points, 5);

        glTexCoord2d(1, 1);
        set_vertex(points, 8);
    }
    glEnd();
}

#undef DK_D_SAVE_NORMALS

static void draw_outline(double* points) {
    // Store state.
    glPushAttrib(GL_ENABLE_BIT | GL_CURRENT_BIT);
    glPushMatrix();

    // Set up for line drawing.
    glLineWidth(3.0f);
    glTranslatef(0, 0, 0.1f);
    glColor4f(0.5f, 0.5f, 1.0f, 0.5f);
    glDisable(GL_LIGHTING);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glBindTexture(GL_TEXTURE_2D, 0);

    glBegin(GL_LINES);
    {
        set_vertex(points, 0);
        set_vertex(points, 1);
        set_vertex(points, 1);
        set_vertex(points, 2);
        set_vertex(points, 2);
        set_vertex(points, 5);
        set_vertex(points, 5);
        set_vertex(points, 8);
        set_vertex(points, 8);
        set_vertex(points, 7);
        set_vertex(points, 7);
        set_vertex(points, 6);
        set_vertex(points, 6);
        set_vertex(points, 3);
        set_vertex(points, 3);
        set_vertex(points, 0);
    }
    glEnd();

    // Reset stuff.
    glPopMatrix();
    glPopAttrib();
}

/** Set a point in a point list to the specified values */
inline static void set_points(double* points, int point, double x, double y, double z) {
    points[point * 3] = x;
    points[point * 3 + 1] = y;
    points[point * 3 + 2] = z;
}

///////////////////////////////////////////////////////////////////////////////
// Picking
///////////////////////////////////////////////////////////////////////////////

inline static GLuint pick_block(int x, int y) {
    return DK_pick(x, y, &DK_render_map);
}

///////////////////////////////////////////////////////////////////////////////
// Header implementation
///////////////////////////////////////////////////////////////////////////////

void DK_init_map(unsigned short size) {
    DK_map_size = size;

    free(map);
    map = calloc(size * size, sizeof (DK_Block));

    int i, j, k, l;
    for (i = 0; i < size; ++i) {
        for (j = 0; j < size; ++j) {
            DK_Block* block = &map[i + j * size];
            block->type = DK_BLOCK_DIRT;
            block->health = DK_BLOCK_DIRT_HEALTH;
            block->strength = DK_BLOCK_DIRT_STRENGTH;
            block->room = DK_ROOM_NONE;
        }
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

inline static int block_index_valid(int x, int y) {
    return x >= 0 && y >= 0 && x < DK_map_size && y < DK_map_size;
}

DK_Block* DK_block_at(int x, int y) {
    if (block_index_valid(x, y)) {
        return &map[y * DK_map_size + x];
    }
    return NULL;
}

int DK_block_coordinates(unsigned short* x, unsigned short* y, const DK_Block* block) {
    if (!block) {
        return 0;
    }
    unsigned int idx = block - map;
    *x = idx % DK_map_size;
    *y = idx / DK_map_size;
}

DK_Block* DK_block_under_cursor(int* block_x, int* block_y, int mouse_x, int mouse_y) {
    GLuint selected_name = pick_block(mouse_x, mouse_y);
    *block_x = selected_name % DK_map_size;
    *block_y = selected_name / DK_map_size;
    return DK_block_at(*block_x, *block_y);
}

int DK_block_is_fluid(const DK_Block* block) {
    return block && (block->type == DK_BLOCK_LAVA || block->type == DK_BLOCK_WATER);
}

int DK_block_is_passable(const DK_Block* block) {
    return block && (block->type == DK_BLOCK_NONE || DK_block_is_fluid(block));
}

void DK_render_map() {
    // Get hovered block.
    int cursor_x = 0, cursor_y = 0;
    int current_mode = GL_SELECT;
    glGetIntegerv(GL_RENDER_MODE, &current_mode);
    if (current_mode != GL_SELECT) {
        int mouse_x, mouse_y;
        SDL_GetMouseState(&mouse_x, &mouse_y);
        DK_block_under_cursor(&cursor_x, &cursor_y, mouse_x, DK_RESOLUTION_Y - mouse_y);
    }

    int x_begin = (int) (DK_camera_position()[0] / DK_BLOCK_SIZE) - DK_RENDER_AREA / 2;
    int y_begin = (int) (DK_camera_position()[1] / DK_BLOCK_SIZE) - DK_RENDER_AREA / 2 + DK_RENDER_OFFSET;
    int x_end = x_begin + DK_RENDER_AREA;
    int y_end = y_begin + DK_RENDER_AREA;
    int x, y;
    for (x = x_begin; x < x_end; ++x) {
        float x_coord = x * (DK_BLOCK_SIZE);

        for (y = y_begin; y < y_end; ++y) {
            float y_coord = y * (DK_BLOCK_SIZE);

            // Remember whether this is the current block.
            const int should_outline =
                    x == cursor_x && y == cursor_y &&
                    DK_block_is_selectable(DK_PLAYER_RED, x, y);

            // Reset tint color.
            glColorMaterial(GL_FRONT, GL_EMISSION);
            glColor3f(0.0f, 0.0f, 0.0f);
            glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);
            glColor3f(1.0f, 1.0f, 1.0f);

            // Mark for select mode, coordinates of that block.
            glLoadName(y * DK_map_size + x);

            DK_Texture texture_top, texture_side, texture_top_wall = 0, texture_top_owner = 0;
            int top;
            if (x < 0 || y < 0 || x >= DK_map_size || y >= DK_map_size) {

                // Solid rock when out of bounds.
                top = DK_BLOCK_HEIGHT;
                texture_top = DK_TEX_ROCK_TOP;
                texture_side = DK_TEX_ROCK_SIDE;
            } else {
                const DK_Block* block = &map[x + y * DK_map_size];

                // Selected by the local player?
                if (DK_block_is_selected(DK_PLAYER_RED, x, y)) {
                    glColorMaterial(GL_FRONT, GL_EMISSION);
                    glColor3f(0.4f, 0.4f, 0.35f);
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

            // Outline hovered block.
            if (should_outline) {
                draw_outline(points);
            }

            draw_4quad(texture_top, points);

            if (texture_top_wall || texture_top_owner) {
                glEnable(GL_BLEND);
                glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

                if (texture_top_wall) {
                    draw_4quad(texture_top_wall, points);
                }
                if (texture_top_owner) {
                    draw_4quad(texture_top_owner, points);
                }

                glDisable(GL_BLEND);
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

                    // Outline hovered block.
                    if (should_outline) {
                        draw_outline(points);
                    }

                    draw_4quad(texture_side, points);
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

                    // Outline hovered block.
                    if (should_outline) {
                        draw_outline(points);
                    }

                    draw_4quad(texture_side, points);
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

                    // Outline hovered block.
                    if (should_outline) {
                        draw_outline(points);
                    }

                    draw_4quad(texture_side, points);
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

                    // Outline hovered block.
                    if (should_outline) {
                        draw_outline(points);
                    }

                    draw_4quad(texture_side, points);
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

                    draw_4quad(DK_TEX_FLUID_SIDE, points);
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

                    draw_4quad(DK_TEX_FLUID_SIDE, points);
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

                    draw_4quad(DK_TEX_FLUID_SIDE, points);
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

                    draw_4quad(DK_TEX_FLUID_SIDE, points);
                }
            }
        }
    }
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

    unsigned short x, y;
    DK_block_coordinates(&x, &y, block);

    int i;
    for (i = 0; i < DK_PLAYER_COUNT; ++i) {
        DK_block_deselect(i, x, y);
    }

    return 1;
}

int DK_block_convert(DK_Block* block, unsigned int strength, DK_Player player) {
    unsigned short x, y;

    // First reduce any enemy influence.
    if (block->owner != player) {
        if (block->strength > strength) {
            block->strength -= strength;
            return 0;
        }
    } else {
        // Owned by this player, repair it.
        if (block->strength + strength < DK_BLOCK_OWNED_STRENGTH) {
            block->strength += strength;
            return 0;
        } else {
            block->strength = DK_BLOCK_OWNED_STRENGTH;
            DK_block_coordinates(&x, &y, block);
            DK_jobs_update(player, x, y);
            return 1;
        }
    }

    // Block is completely converted.
    block->strength = DK_BLOCK_OWNED_STRENGTH;
    block->owner = player;
    DK_block_coordinates(&x, &y, block);

    int i;
    for (i = 0; i < DK_PLAYER_COUNT; ++i) {
        DK_block_deselect(i, x, y);
    }

    return 1;
}
