#include <math.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glext.h>
#include <string.h>

#include "config.h"
#include "map.h"
#include "simplexnoise.h"
#include "textures.h"

double last_normal[3];
double last_vertex[3];

int DK_block_is_fluid(const DK_Block* block) {
    return block->type == DK_BLOCK_LAVA || block->type == DK_BLOCK_WATER;
}

int DK_block_is_passable(const DK_Block* block) {
    return block->type == DK_BLOCK_NONE || DK_block_is_fluid(block);
}

unsigned int DK_variation(double x, double y, double z) {
    return (unsigned int) ((snoise2(x, y + z) + 1) / 2 * DK_TEX_MAX_VARIATIONS);
}

double DK_noise_factor_block(const DK_Block* block) {
    if (DK_block_is_passable(block) && block->owner == DK_PLAYER_NONE) {
        return 1.5;
    } else if (block->owner != DK_PLAYER_NONE) {
        return 0.5;
    }
    return 1;
}

double DK_noise_factor(double x, double y) {
    const int i = round(x), j = round(y);
    double factor;
    if (fabs(x - (int) x - 0.5f) < 0.01f) {
        // Half way vertex, apply only the two adjacent blocks.
        if (fabs(y - (int) y - 0.5f) < 0.01f) {
            // Half way vertex, apply only the one adjacent block.
            return DK_noise_factor_block(&DK_map[i + j * DK_map_size]);
        } else if (j > 0 && j < DK_map_size) {
            factor = DK_noise_factor_block(&DK_map[i + j * DK_map_size]);
            factor += DK_noise_factor_block(&DK_map[i + (j - 1) * DK_map_size]);
            return factor / 2.0;
        }
    } else if (i > 0 && i < DK_map_size) {
        // Inner vertex, apply normal rules.
        if (fabs(y - (int) y - 0.5f) < 0.01f) {
            // Half way vertex, apply only the two adjacent blocks.
            factor = DK_noise_factor_block(&DK_map[i + j * DK_map_size]);
            factor += DK_noise_factor_block(&DK_map[i - 1 + j * DK_map_size]);
            return factor / 2.0;
        } else if (j > 0 && j < DK_map_size) {
            factor = DK_noise_factor_block(&DK_map[i + j * DK_map_size]);
            factor += DK_noise_factor_block(&DK_map[i - 1 + j * DK_map_size]);
            factor += DK_noise_factor_block(&DK_map[i + (j - 1) * DK_map_size]);
            factor += DK_noise_factor_block(&DK_map[i - 1 + (j - 1) * DK_map_size]);
            return factor / 4.0;
        }
    }
    return 0;
}

void DK_get_vertex(double* v, const double* points, int point) {
    const double* p = &points[point * 3];
    const double f = DK_noise_factor(p[0] / DK_BLOCK_SIZE, p[1] / DK_BLOCK_SIZE);
    v[0] = p[0] + f * snoise4(p[0], p[1], p[2], 0);
    v[1] = p[1] + f * snoise4(p[0], p[1], p[2], 1);
    v[2] = p[2] + snoise4(p[0], p[1], p[2], 2);
}

void DK_set_point(const double* points, int point) {
    DK_get_vertex(last_vertex, points, point);
    glVertex3dv(last_vertex);
}

void DK_cross(double* cross, const double* v0, const double* v1) {
    cross[0] = v0[1] * v1[2] - v0[2] * v1[1];
    cross[1] = v0[2] * v1[0] - v0[0] * v1[2];
    cross[2] = v0[0] * v1[1] - v0[1] * v1[0];
}

void DK_normal2(const double* points, int i0, int i1, int i2) {
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
    DK_cross(n, a, b);

    const double len = sqrt(n[0] * n[0] + n[1] * n[1] + n[2] * n[2]);
    n[0] /= len;
    n[1] /= len;
    n[2] /= len;

    glNormal3dv(n);
}

void DK_normal3(const double* points, int i0, int i1, int i2, int i3) {
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
    DK_cross(t, a, b);
    DK_cross(n, b, c);
    n[0] = (n[0] + t[0]) / 2.0;
    n[1] = (n[1] + t[1]) / 2.0;
    n[2] = (n[2] + t[2]) / 2.0;

    const double len = sqrt(n[0] * n[0] + n[1] * n[1] + n[2] * n[2]);
    n[0] /= len;
    n[1] /= len;
    n[2] /= len;

    glNormal3dv(n);
}

void DK_normal4(const double* points, int i0, int i1, int i2, int i3, int i4) {
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
    DK_cross(t0, a, b);
    DK_cross(t1, b, c);
    DK_cross(n, c, d);
    n[0] = (n[0] + t0[0] + t1[0]) / 3.0;
    n[1] = (n[1] + t0[1] + t1[0]) / 3.0;
    n[2] = (n[2] + t0[2] + t1[0]) / 3.0;

    const double len = sqrt(n[0] * n[0] + n[1] * n[1] + n[2] * n[2]);
    n[0] /= len;
    n[1] /= len;
    n[2] /= len;

    glNormal3dv(n);
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
        DK_normal2(points, 0, 3, 1);
        glTexCoord2d(0, 0);
        DK_set_point(points, 0);

        DK_D_SAVE_NORMALS();

        DK_normal3(points, 3, 4, 1, 0);
        glTexCoord2d(0, 0.5);
        DK_set_point(points, 3);

        DK_D_SAVE_NORMALS();

        DK_normal4(points, 1, 0, 3, 4, 2);
        glTexCoord2d(0.5, 0);
        DK_set_point(points, 1);

        DK_D_SAVE_NORMALS();

        DK_normal4(points, 4, 5, 2, 1, 3);
        glTexCoord2d(0.5, 0.5);
        DK_set_point(points, 4);

        DK_D_SAVE_NORMALS();

        DK_normal3(points, 2, 1, 4, 5);
        glTexCoord2d(1, 0);
        DK_set_point(points, 2);

        DK_D_SAVE_NORMALS();

        DK_normal2(points, 5, 2, 4);
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
        DK_normal2(points, 3, 6, 4);
        glTexCoord2d(0, 0.5);
        DK_set_point(points, 3);

        DK_D_SAVE_NORMALS();

        DK_normal3(points, 6, 7, 4, 3);
        glTexCoord2d(0, 1);
        DK_set_point(points, 6);

        DK_D_SAVE_NORMALS();

        DK_normal4(points, 4, 3, 6, 7, 5);
        glTexCoord2d(0.5, 0.5);
        DK_set_point(points, 4);

        DK_D_SAVE_NORMALS();

        DK_normal4(points, 7, 8, 5, 4, 6);
        glTexCoord2d(0.5, 1);
        DK_set_point(points, 7);

        DK_D_SAVE_NORMALS();

        DK_normal3(points, 5, 4, 7, 8);
        glTexCoord2d(1, 0.5);
        DK_set_point(points, 5);

        DK_D_SAVE_NORMALS();

        DK_normal2(points, 8, 5, 7);
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

void DK_render_map() {
    unsigned short y, x;
    for (x = 0; x < DK_map_size; ++x) {
        float x_coord = x * (DK_BLOCK_SIZE);

        for (y = 0; y < DK_map_size; ++y) {
            float y_coord = y * (DK_BLOCK_SIZE);

            const DK_Block* block = &DK_map[x + y * DK_map_size];

            DK_Texture texture_top, texture_side;
            int top;
            if (block->type == DK_BLOCK_NONE) {
                // Render floor.
                top = 0;
                texture_top = DK_TEX_DIRT_FLOOR;
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
                            continue;
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

            // Check if we need to render walls.
            if (top > 0) {
                // North wall.
                if (y + 1 < DK_map_size && DK_block_is_passable(&DK_map[x + (y + 1) * DK_map_size])) {
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
                if (y > 0 && DK_block_is_passable(&DK_map[x + (y - 1) * DK_map_size])) {
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
                if (x + 1 < DK_map_size && DK_block_is_passable(&DK_map[(x + 1) + y * DK_map_size])) {
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
                if (x > 0 && DK_block_is_passable(&DK_map[(x - 1) + y * DK_map_size])) {
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
                if (y + 1 < DK_map_size && !DK_block_is_fluid(&DK_map[x + (y + 1) * DK_map_size])) {
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
                if (y > 0 && !DK_block_is_fluid(&DK_map[x + (y - 1) * DK_map_size])) {
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
                if (x + 1 < DK_map_size && !DK_block_is_fluid(&DK_map[(x + 1) + y * DK_map_size])) {
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
                if (x > 0 && !DK_block_is_fluid(&DK_map[(x - 1) + y * DK_map_size])) {
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
