#include <math.h>
#include <GL/gl.h>

extern double last_normal[3];

void cross(double* cross, const double* v0, const double* v1) {
    cross[0] = v0[1] * v1[2] - v0[2] * v1[1];
    cross[1] = v0[2] * v1[0] - v0[0] * v1[2];
    cross[2] = v0[0] * v1[1] - v0[1] * v1[0];
}

void normal2(const double* points, int i0, int i1, int i2) {
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

void normal3(const double* points, int i0, int i1, int i2, int i3) {
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

void normal4(const double* points, int i0, int i1, int i2, int i3, int i4) {
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
