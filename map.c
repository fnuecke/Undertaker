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

double last_normal[3] = {0, 0, 0};
double last_vertex[3] = {0, 0, 0};

DK_Block* map = 0;
double* map_noise = 0;

extern void cross(double* cross, const double* v0, const double* v1);
extern void normal2(const double* points, int i0, int i1, int i2);
extern void normal3(const double* points, int i0, int i1, int i2, int i3);
extern void normal4(const double* points, int i0, int i1, int i2, int i3, int i4);

double* DK_map_noise_at_index(int x, int y, int z, int k) {
    return &map_noise[((k * 3 + z) * 3 + y) * (DK_map_size + 1) + x];
}

double DK_get_noise(double x, double y, double z, int k) {
    int i = (int) round((x * 2) / DK_BLOCK_SIZE),
            j = (int) round((y * 2) / DK_BLOCK_SIZE),
            l = (int) round((z * 2) / DK_BLOCK_HEIGHT);
    return *DK_map_noise_at_index(i, j, l, k);
}

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

#if DK_D_CACHE_NOISE
    free(map_noise);
    map_noise = calloc((size * 2 + 1) * (size * 2 + 1) * 3 * 3, sizeof (double));
    for (i = 0; i < size * 2 + 1; ++i) {
        for (j = 0; j < size * 2 + 1; ++j) {
            for (k = 0; k < 3; ++k) {
                for (l = 0; l < 3; ++l) {
                    *DK_map_noise_at_index(i, j, k, l) = snoise4(i * DK_BLOCK_SIZE / 2.0, j * DK_BLOCK_SIZE / 2.0, k / 2.0 * DK_BLOCK_HEIGHT, l);
                }
            }
        }
    }
#endif
}

typedef enum {
    OFFSET_NONE,
    OFFSET_INCREASE,
    OFFSET_DECREASE
} DK_Offset;

double DK_noise_factor_block(DK_Offset* offset, const DK_Block* block) {
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

/**
 * Gets noise factor (amplitude) based on neighboring blocks.
 * Also gets a directed offset based on where the neighboring block are (away
 * from empty blocks).
 */
double DK_noise_factor(double* offset, double x, double y) {
    DK_Offset offset_type;
    const int i = round(x), j = round(y);
    double factor;
    if (fabs(x - (int) x - 0.5f) < 0.01f) {
        // Half way vertex, apply only the two adjacent blocks.
        if (fabs(y - (int) y - 0.5f) < 0.01f) {
            // Half way vertex, apply only the one adjacent block.
            return DK_noise_factor_block(&offset_type, &map[i + j * DK_map_size]);
        } else if (j > 0 && j < DK_map_size) {
            factor = DK_noise_factor_block(&offset_type, &map[i + j * DK_map_size]);
            if (offset_type == OFFSET_INCREASE) {
                offset[1] -= DK_BLOCK_MAX_NOISE;
                if ((i ^ j) & 1) {
                    offset[0] -= 0.5 * DK_BLOCK_MAX_NOISE;
                } else {
                    offset[0] += 0.5 * DK_BLOCK_MAX_NOISE;
                }
            } else if (offset_type == OFFSET_DECREASE) {
                offset[0] *= 0.5;
                offset[1] *= 0.5;
            }

            factor += DK_noise_factor_block(&offset_type, &map[i + (j - 1) * DK_map_size]);
            if (offset_type == OFFSET_INCREASE) {
                offset[1] += DK_BLOCK_MAX_NOISE;
                if ((i ^ j) & 1) {
                    offset[0] += 0.5 * DK_BLOCK_MAX_NOISE;
                } else {
                    offset[0] -= 0.5 * DK_BLOCK_MAX_NOISE;
                }
            } else if (offset_type == OFFSET_DECREASE) {
                offset[0] *= 0.5;
                offset[1] *= 0.5;
            }

            if (offset[0] != 0 || offset[1] != 0) {
                const float len = sqrt(offset[0] * offset[0] + offset[1] * offset[1]);
                if (len > DK_BLOCK_MAX_NOISE) {
                    offset[0] *= 0.5 * DK_BLOCK_MAX_NOISE / len;
                    offset[1] *= 0.5 * DK_BLOCK_MAX_NOISE / len;
                }
            }

            return factor / 2.0;
        }
    } else if (i > 0 && i < DK_map_size) {
        // Inner vertex, apply normal rules.
        if (fabs(y - (int) y - 0.5f) < 0.01f) {
            // Half way vertex, apply only the two adjacent blocks.
            factor = DK_noise_factor_block(&offset_type, &map[i + j * DK_map_size]);
            if (offset_type == OFFSET_INCREASE) {
                offset[0] -= DK_BLOCK_MAX_NOISE;
                if ((i ^ j) & 1) {
                    offset[1] += 0.5 * DK_BLOCK_MAX_NOISE;
                } else {
                    offset[1] -= 0.5 * DK_BLOCK_MAX_NOISE;
                }
            } else if (offset_type == OFFSET_DECREASE) {
                offset[0] *= 0.5;
                offset[1] *= 0.5;
            }

            factor += DK_noise_factor_block(&offset_type, &map[i - 1 + j * DK_map_size]);
            if (offset_type == OFFSET_INCREASE) {
                offset[0] += DK_BLOCK_MAX_NOISE;
                if ((i ^ j) & 1) {
                    offset[1] -= 0.5 * DK_BLOCK_MAX_NOISE;
                } else {
                    offset[1] += 0.5 * DK_BLOCK_MAX_NOISE;
                }
            } else if (offset_type == OFFSET_DECREASE) {
                offset[0] *= 0.5;
                offset[1] *= 0.5;
            }

            if (offset[0] != 0 || offset[1] != 0) {
                const float len = sqrt(offset[0] * offset[0] + offset[1] * offset[1]);
                if (len > DK_BLOCK_MAX_NOISE) {
                    offset[0] *= DK_BLOCK_MAX_NOISE / len;
                    offset[1] *= DK_BLOCK_MAX_NOISE / len;
                }
            }

            return factor / 2.0;
        } else if (j > 0 && j < DK_map_size) {
            factor = DK_noise_factor_block(&offset_type, &map[i + j * DK_map_size]);
            if (offset_type == OFFSET_INCREASE) {
                offset[0] -= 1;
                offset[1] -= 1;
            } else if (offset_type == OFFSET_DECREASE) {
                offset[0] *= 0.5;
                offset[1] *= 0.5;
            }

            factor += DK_noise_factor_block(&offset_type, &map[i - 1 + j * DK_map_size]);
            if (offset_type == OFFSET_INCREASE) {
                offset[0] += 1;
                offset[1] -= 1;
            } else if (offset_type == OFFSET_DECREASE) {
                offset[0] *= 0.5;
                offset[1] *= 0.5;
            }

            factor += DK_noise_factor_block(&offset_type, &map[i + (j - 1) * DK_map_size]);
            if (offset_type == OFFSET_INCREASE) {
                offset[0] -= 1;
                offset[1] += 1;
            } else if (offset_type == OFFSET_DECREASE) {
                offset[0] *= 0.5;
                offset[1] *= 0.5;
            }

            factor += DK_noise_factor_block(&offset_type, &map[i - 1 + (j - 1) * DK_map_size]);
            if (offset_type == OFFSET_INCREASE) {
                offset[0] += 1;
                offset[1] += 1;
            } else if (offset_type == OFFSET_DECREASE) {
                offset[0] *= 0.5;
                offset[1] *= 0.5;
            }

            if (offset[0] != 0 || offset[1] != 0) {
                const float len = sqrt(offset[0] * offset[0] + offset[1] * offset[1]);
                if (len > DK_BLOCK_MAX_NOISE) {
                    offset[0] *= DK_BLOCK_MAX_NOISE / len;
                    offset[1] *= DK_BLOCK_MAX_NOISE / len;
                }
            }

            return factor / 4.0;
        }
    }
    return 0;
}

void DK_get_vertex(double* v, const double* points, int point) {
    const double* p = &points[point * 3];
#if DK_D_TERRAIN_NOISE
    double offset[2] = {0, 0};
#if DK_D_FACTOR_NOISE
    const double factor = DK_noise_factor(offset, p[0] / DK_BLOCK_SIZE, p[1] / DK_BLOCK_SIZE);
#else
    const double factor = 1.0;
#endif
    const double offset_factor = 2 * (0.75 - fabs(p[2] / DK_BLOCK_HEIGHT - 0.5));
    offset[0] *= offset_factor;
    offset[1] *= offset_factor;
#if DK_D_CACHE_NOISE
    v[0] = p[0] + factor * DK_get_noise(p[0], p[1], p[2], 0) + offset[0];
    v[1] = p[1] + factor * DK_get_noise(p[0], p[1], p[2], 1) + offset[1];
    v[2] = p[2] + DK_get_noise(p[0], p[1], p[2], 2);
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

unsigned int DK_variation(double x, double y, double z) {
    return (unsigned int) ((snoise2(x, y + z) + 1) / 2 * DK_TEX_MAX_VARIATIONS);
}

void DK_set_point(const double* points, int point) {
    DK_get_vertex(last_vertex, points, point);
    glVertex3dv(last_vertex);
}

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
void DK_draw_4quad(DK_Texture texture, double* points) {
#ifdef DK_D_DRAW_NORMALS
    double v[3 * 6], n[3 * 6];
    int ni = 0;
#endif

    glBindTexture(GL_TEXTURE_2D, DK_opengl_texture(texture, DK_variation(points[0], points[1], points[2])));
    glBegin(GL_QUAD_STRIP);
    {
#if DK_D_COMPUTE_NORMALS
        normal2(points, 0, 3, 1);
#endif
        glTexCoord2d(0, 0);
        DK_set_point(points, 0);

        DK_D_SAVE_NORMALS();

#if DK_D_COMPUTE_NORMALS
        normal3(points, 3, 4, 1, 0);
#endif
        glTexCoord2d(0, 0.5);
        DK_set_point(points, 3);

        DK_D_SAVE_NORMALS();

#if DK_D_COMPUTE_NORMALS
        normal4(points, 1, 0, 3, 4, 2);
#endif
        glTexCoord2d(0.5, 0);
        DK_set_point(points, 1);

        DK_D_SAVE_NORMALS();

#if DK_D_COMPUTE_NORMALS
        normal4(points, 4, 5, 2, 1, 3);
#endif
        glTexCoord2d(0.5, 0.5);
        DK_set_point(points, 4);

        DK_D_SAVE_NORMALS();

#if DK_D_COMPUTE_NORMALS
        normal3(points, 2, 1, 4, 5);
#endif
        glTexCoord2d(1, 0);
        DK_set_point(points, 2);

        DK_D_SAVE_NORMALS();

#if DK_D_COMPUTE_NORMALS
        normal2(points, 5, 2, 4);
#endif
        glTexCoord2d(1, 0.5);
        DK_set_point(points, 5);

        DK_D_SAVE_NORMALS();
    }
    glEnd();

#if DK_D_DRAW_NORMALS
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_LIGHTING);

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

    glEnable(GL_TEXTURE_2D);
    glEnable(GL_LIGHTING);

    ni = 0;
#endif

    glBegin(GL_QUAD_STRIP);
    {
#if DK_D_COMPUTE_NORMALS
        normal2(points, 3, 6, 4);
#endif
        glTexCoord2d(0, 0.5);
        DK_set_point(points, 3);

        DK_D_SAVE_NORMALS();

#if DK_D_COMPUTE_NORMALS
        normal3(points, 6, 7, 4, 3);
#endif
        glTexCoord2d(0, 1);
        DK_set_point(points, 6);

        DK_D_SAVE_NORMALS();

#if DK_D_COMPUTE_NORMALS
        normal4(points, 4, 3, 6, 7, 5);
#endif
        glTexCoord2d(0.5, 0.5);
        DK_set_point(points, 4);

        DK_D_SAVE_NORMALS();

#if DK_D_COMPUTE_NORMALS
        normal4(points, 7, 8, 5, 4, 6);
#endif
        glTexCoord2d(0.5, 1);
        DK_set_point(points, 7);

        DK_D_SAVE_NORMALS();

#if DK_D_COMPUTE_NORMALS
        normal3(points, 5, 4, 7, 8);
#endif
        glTexCoord2d(1, 0.5);
        DK_set_point(points, 5);

        DK_D_SAVE_NORMALS();

#if DK_D_COMPUTE_NORMALS
        normal2(points, 8, 5, 7);
#endif
        glTexCoord2d(1, 1);
        DK_set_point(points, 8);

        DK_D_SAVE_NORMALS();
    }
    glEnd();

#if DK_D_DRAW_NORMALS
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_LIGHTING);

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

    glEnable(GL_TEXTURE_2D);
    glEnable(GL_LIGHTING);
#endif
}

void DK_set_points(double* points, int point, double x, double y, double z) {
    points[point * 3] = x;
    points[point * 3 + 1] = y;
    points[point * 3 + 2] = z;
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
    if (x_begin < 0) {
        x_begin = 0;
    }
    if (y_begin < 0) {
        y_begin = 0;
    }
    if (x_end > DK_map_size) {
        x_end = DK_map_size;
    }
    if (y_end > DK_map_size) {
        y_end = DK_map_size;
    }
    int x, y;
    for (x = x_begin; x < x_end; ++x) {
        float x_coord = x * (DK_BLOCK_SIZE);

        for (y = y_begin; y < y_end; ++y) {
            float y_coord = y * (DK_BLOCK_SIZE);

            const DK_Block* block = &map[x + y * DK_map_size];

            DK_Texture texture_top, texture_side, texture_top_wall = 0, texture_top_owner = 0;
            int top;
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

            /*
             * 0 - 1 - 2
             * | / | / |
             * 3 - 4 - 5
             * | / | / |
             * 6 - 7 - 8
             */
            double points[3 * 9];

            DK_set_points(points, 0, x_coord, y_coord + DK_BLOCK_SIZE, top);
            DK_set_points(points, 1, x_coord + DK_BLOCK_SIZE / 2.0, y_coord + DK_BLOCK_SIZE, top);
            DK_set_points(points, 2, x_coord + DK_BLOCK_SIZE, y_coord + DK_BLOCK_SIZE, top);
            DK_set_points(points, 3, x_coord, y_coord + DK_BLOCK_SIZE / 2.0, top);
            DK_set_points(points, 4, x_coord + DK_BLOCK_SIZE / 2.0, y_coord + DK_BLOCK_SIZE / 2.0, top);
            DK_set_points(points, 5, x_coord + DK_BLOCK_SIZE, y_coord + DK_BLOCK_SIZE / 2.0, top);
            DK_set_points(points, 6, x_coord, y_coord, top);
            DK_set_points(points, 7, x_coord + DK_BLOCK_SIZE / 2.0, y_coord, top);
            DK_set_points(points, 8, x_coord + DK_BLOCK_SIZE, y_coord, top);

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
                    DK_set_points(points, 0, x_coord + DK_BLOCK_SIZE, y_coord + DK_BLOCK_SIZE, DK_BLOCK_HEIGHT);
                    DK_set_points(points, 1, x_coord + DK_BLOCK_SIZE / 2.0, y_coord + DK_BLOCK_SIZE, DK_BLOCK_HEIGHT);
                    DK_set_points(points, 2, x_coord, y_coord + DK_BLOCK_SIZE, DK_BLOCK_HEIGHT);
                    DK_set_points(points, 3, x_coord + DK_BLOCK_SIZE, y_coord + DK_BLOCK_SIZE, DK_BLOCK_HEIGHT / 2.0);
                    DK_set_points(points, 4, x_coord + DK_BLOCK_SIZE / 2.0, y_coord + DK_BLOCK_SIZE, DK_BLOCK_HEIGHT / 2.0);
                    DK_set_points(points, 5, x_coord, y_coord + DK_BLOCK_SIZE, DK_BLOCK_HEIGHT / 2.0);
                    DK_set_points(points, 6, x_coord + DK_BLOCK_SIZE, y_coord + DK_BLOCK_SIZE, 0);
                    DK_set_points(points, 7, x_coord + DK_BLOCK_SIZE / 2.0, y_coord + DK_BLOCK_SIZE, 0);
                    DK_set_points(points, 8, x_coord, y_coord + DK_BLOCK_SIZE, 0);

                    DK_draw_4quad(texture_side, points);
                }

                // South wall.
                if (y > 0 && DK_block_is_passable(&map[x + (y - 1) * DK_map_size])) {
                    DK_set_points(points, 0, x_coord, y_coord, DK_BLOCK_HEIGHT);
                    DK_set_points(points, 1, x_coord + DK_BLOCK_SIZE / 2.0, y_coord, DK_BLOCK_HEIGHT);
                    DK_set_points(points, 2, x_coord + DK_BLOCK_SIZE, y_coord, DK_BLOCK_HEIGHT);
                    DK_set_points(points, 3, x_coord, y_coord, DK_BLOCK_HEIGHT / 2.0);
                    DK_set_points(points, 4, x_coord + DK_BLOCK_SIZE / 2.0, y_coord, DK_BLOCK_HEIGHT / 2.0);
                    DK_set_points(points, 5, x_coord + DK_BLOCK_SIZE, y_coord, DK_BLOCK_HEIGHT / 2.0);
                    DK_set_points(points, 6, x_coord, y_coord, 0);
                    DK_set_points(points, 7, x_coord + DK_BLOCK_SIZE / 2.0, y_coord, 0);
                    DK_set_points(points, 8, x_coord + DK_BLOCK_SIZE, y_coord, 0);

                    DK_draw_4quad(texture_side, points);
                }

                // East wall.
                if (x + 1 < DK_map_size && DK_block_is_passable(&map[(x + 1) + y * DK_map_size])) {
                    DK_set_points(points, 0, x_coord + DK_BLOCK_SIZE, y_coord, DK_BLOCK_HEIGHT);
                    DK_set_points(points, 1, x_coord + DK_BLOCK_SIZE, y_coord + DK_BLOCK_SIZE / 2.0, DK_BLOCK_HEIGHT);
                    DK_set_points(points, 2, x_coord + DK_BLOCK_SIZE, y_coord + DK_BLOCK_SIZE, DK_BLOCK_HEIGHT);
                    DK_set_points(points, 3, x_coord + DK_BLOCK_SIZE, y_coord, DK_BLOCK_HEIGHT / 2.0);
                    DK_set_points(points, 4, x_coord + DK_BLOCK_SIZE, y_coord + DK_BLOCK_SIZE / 2.0, DK_BLOCK_HEIGHT / 2.0);
                    DK_set_points(points, 5, x_coord + DK_BLOCK_SIZE, y_coord + DK_BLOCK_SIZE, DK_BLOCK_HEIGHT / 2.0);
                    DK_set_points(points, 6, x_coord + DK_BLOCK_SIZE, y_coord, 0);
                    DK_set_points(points, 7, x_coord + DK_BLOCK_SIZE, y_coord + DK_BLOCK_SIZE / 2.0, 0);
                    DK_set_points(points, 8, x_coord + DK_BLOCK_SIZE, y_coord + DK_BLOCK_SIZE, 0);

                    DK_draw_4quad(texture_side, points);
                }

                // West wall.
                if (x > 0 && DK_block_is_passable(&map[(x - 1) + y * DK_map_size])) {
                    DK_set_points(points, 0, x_coord, y_coord + DK_BLOCK_SIZE, DK_BLOCK_HEIGHT);
                    DK_set_points(points, 1, x_coord, y_coord + DK_BLOCK_SIZE / 2.0, DK_BLOCK_HEIGHT);
                    DK_set_points(points, 2, x_coord, y_coord, DK_BLOCK_HEIGHT);
                    DK_set_points(points, 3, x_coord, y_coord + DK_BLOCK_SIZE, DK_BLOCK_HEIGHT / 2.0);
                    DK_set_points(points, 4, x_coord, y_coord + DK_BLOCK_SIZE / 2.0, DK_BLOCK_HEIGHT / 2.0);
                    DK_set_points(points, 5, x_coord, y_coord, DK_BLOCK_HEIGHT / 2.0);
                    DK_set_points(points, 6, x_coord, y_coord + DK_BLOCK_SIZE, 0);
                    DK_set_points(points, 7, x_coord, y_coord + DK_BLOCK_SIZE / 2.0, 0);
                    DK_set_points(points, 8, x_coord, y_coord, 0);

                    DK_draw_4quad(texture_side, points);
                }
            }

            // Check if we need to render water walls.
            if (top < 0) {
                // North wall.
                if (y + 1 < DK_map_size && !DK_block_is_fluid(&map[x + (y + 1) * DK_map_size])) {
                    DK_set_points(points, 0, x_coord, y_coord + DK_BLOCK_SIZE, 0);
                    DK_set_points(points, 1, x_coord + DK_BLOCK_SIZE / 2.0, y_coord + DK_BLOCK_SIZE, 0);
                    DK_set_points(points, 2, x_coord + DK_BLOCK_SIZE, y_coord + DK_BLOCK_SIZE, 0);
                    DK_set_points(points, 3, x_coord, y_coord + DK_BLOCK_SIZE, top / 2.0);
                    DK_set_points(points, 4, x_coord + DK_BLOCK_SIZE / 2.0, y_coord + DK_BLOCK_SIZE, top / 2.0);
                    DK_set_points(points, 5, x_coord + DK_BLOCK_SIZE, y_coord + DK_BLOCK_SIZE, top / 2.0);
                    DK_set_points(points, 6, x_coord, y_coord + DK_BLOCK_SIZE, top);
                    DK_set_points(points, 7, x_coord + DK_BLOCK_SIZE / 2.0, y_coord + DK_BLOCK_SIZE, top);
                    DK_set_points(points, 8, x_coord + DK_BLOCK_SIZE, y_coord + DK_BLOCK_SIZE, top);

                    DK_draw_4quad(DK_TEX_FLUID_SIDE, points);
                }

                // South wall.
                if (y > 0 && !DK_block_is_fluid(&map[x + (y - 1) * DK_map_size])) {
                    DK_set_points(points, 0, x_coord + DK_BLOCK_SIZE, y_coord, 0);
                    DK_set_points(points, 1, x_coord + DK_BLOCK_SIZE / 2.0, y_coord, 0);
                    DK_set_points(points, 2, x_coord, y_coord, 0);
                    DK_set_points(points, 3, x_coord + DK_BLOCK_SIZE, y_coord, top / 2.0);
                    DK_set_points(points, 4, x_coord + DK_BLOCK_SIZE / 2.0, y_coord, top / 2.0);
                    DK_set_points(points, 5, x_coord, y_coord, top / 2.0);
                    DK_set_points(points, 6, x_coord + DK_BLOCK_SIZE, y_coord, top);
                    DK_set_points(points, 7, x_coord + DK_BLOCK_SIZE / 2.0, y_coord, top);
                    DK_set_points(points, 8, x_coord, y_coord, top);

                    DK_draw_4quad(DK_TEX_FLUID_SIDE, points);
                }

                // East wall.
                if (x + 1 < DK_map_size && !DK_block_is_fluid(&map[(x + 1) + y * DK_map_size])) {
                    DK_set_points(points, 0, x_coord + DK_BLOCK_SIZE, y_coord + DK_BLOCK_SIZE, 0);
                    DK_set_points(points, 1, x_coord + DK_BLOCK_SIZE, y_coord + DK_BLOCK_SIZE / 2.0, 0);
                    DK_set_points(points, 2, x_coord + DK_BLOCK_SIZE, y_coord, 0);
                    DK_set_points(points, 3, x_coord + DK_BLOCK_SIZE, y_coord + DK_BLOCK_SIZE, top / 2.0);
                    DK_set_points(points, 4, x_coord + DK_BLOCK_SIZE, y_coord + DK_BLOCK_SIZE / 2.0, top / 2.0);
                    DK_set_points(points, 5, x_coord + DK_BLOCK_SIZE, y_coord, top / 2.0);
                    DK_set_points(points, 6, x_coord + DK_BLOCK_SIZE, y_coord + DK_BLOCK_SIZE, top);
                    DK_set_points(points, 7, x_coord + DK_BLOCK_SIZE, y_coord + DK_BLOCK_SIZE / 2.0, top);
                    DK_set_points(points, 8, x_coord + DK_BLOCK_SIZE, y_coord, top);

                    DK_draw_4quad(DK_TEX_FLUID_SIDE, points);
                }

                // West wall.
                if (x > 0 && !DK_block_is_fluid(&map[(x - 1) + y * DK_map_size])) {
                    DK_set_points(points, 0, x_coord, y_coord, 0);
                    DK_set_points(points, 1, x_coord, y_coord + DK_BLOCK_SIZE / 2.0, 0);
                    DK_set_points(points, 2, x_coord, y_coord + DK_BLOCK_SIZE, 0);
                    DK_set_points(points, 3, x_coord, y_coord, top / 2.0);
                    DK_set_points(points, 4, x_coord, y_coord + DK_BLOCK_SIZE / 2.0, top / 2.0);
                    DK_set_points(points, 5, x_coord, y_coord + DK_BLOCK_SIZE, top / 2.0);
                    DK_set_points(points, 6, x_coord, y_coord, top);
                    DK_set_points(points, 7, x_coord, y_coord + DK_BLOCK_SIZE / 2.0, top);
                    DK_set_points(points, 8, x_coord, y_coord + DK_BLOCK_SIZE, top);

                    DK_draw_4quad(DK_TEX_FLUID_SIDE, points);
                }
            }
        }
    }
}
