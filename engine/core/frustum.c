#include "frustum.h"
#include "game_state.h"
#include <string.h>
// taken from https://www.gamedevs.org/uploads/fast-extraction-viewing-frustum-planes-from-world-view-projection-matrix.pdf
typedef struct Matrix4x4
{
    // The elements of the 4x4 matrix are stored in
    // column-major order (see "OpenGL Programming Guide",
    // 3rd edition, pp 106, glLoadMatrix).
    float _11, _21, _31, _41;
    float _12, _22, _32, _42;
    float _13, _23, _33, _43;
    float _14, _24, _34, _44;
} Matrix4x4;

typedef struct Plane
{
    float a, b, c, d;
} Plane;

void NormalizePlane(Plane plane)
{
    float mag;
    mag     = (float) sqrt(plane.a * plane.a + plane.b * plane.b + plane.c * plane.c);
    plane.a = plane.a / mag;
    plane.b = plane.b / mag;
    plane.c = plane.c / mag;
    plane.d = plane.d / mag;
}

void ExtractPlanesGL(
    Plane*          p_planes,
    const Matrix4x4 comboMatrix,
    int             normalize)
{
    // Left clipping plane
    p_planes[0].a = comboMatrix._41 + comboMatrix._11;
    p_planes[0].b = comboMatrix._42 + comboMatrix._12;
    p_planes[0].c = comboMatrix._43 + comboMatrix._13;
    p_planes[0].d = comboMatrix._44 + comboMatrix._14;
    // Right clipping plane
    p_planes[1].a = comboMatrix._41 - comboMatrix._11;
    p_planes[1].b = comboMatrix._42 - comboMatrix._12;
    p_planes[1].c = comboMatrix._43 - comboMatrix._13;
    p_planes[1].d = comboMatrix._44 - comboMatrix._14;
    // Top clipping plane
    p_planes[2].a = comboMatrix._41 - comboMatrix._21;
    p_planes[2].b = comboMatrix._42 - comboMatrix._22;
    p_planes[2].c = comboMatrix._43 - comboMatrix._23;
    p_planes[2].d = comboMatrix._44 - comboMatrix._24;
    // Bottom clipping plane
    p_planes[3].a = comboMatrix._41 + comboMatrix._21;
    p_planes[3].b = comboMatrix._42 + comboMatrix._22;
    p_planes[3].c = comboMatrix._43 + comboMatrix._23;
    p_planes[3].d = comboMatrix._44 + comboMatrix._24;
    // Near clipping plane
    p_planes[4].a = comboMatrix._41 + comboMatrix._31;
    p_planes[4].b = comboMatrix._42 + comboMatrix._32;
    p_planes[4].c = comboMatrix._43 + comboMatrix._33;
    p_planes[4].d = comboMatrix._44 + comboMatrix._34;
    // Far clipping plane
    p_planes[5].a = comboMatrix._41 - comboMatrix._31;
    p_planes[5].b = comboMatrix._42 - comboMatrix._32;
    p_planes[5].c = comboMatrix._43 - comboMatrix._33;
    p_planes[5].d = comboMatrix._44 - comboMatrix._34;
    // Normalize the plane equations, if requested
    if (normalize) {
        NormalizePlane(p_planes[0]);
        NormalizePlane(p_planes[1]);
        NormalizePlane(p_planes[2]);
        NormalizePlane(p_planes[3]);
        NormalizePlane(p_planes[4]);
        NormalizePlane(p_planes[5]);
    }
}

int ClassifySphere(const Plane plane, vec4 pt)
{
    float d;
    d = plane.a * pt[0] + plane.b * pt[1] + plane.c * pt[2] + plane.d;
    return d > -pt[3] - 1.25f;
}

int cull_nodes(int count, vec4* nodes, vec4* out_visible_nodes, mat4 viewproj)
{
    typedef struct Frustum
    {
        Plane planes[6];
    } Frustum;

    int rocks_visible_count = 0;
    // construct frustum
    Frustum   f;
    Matrix4x4 mat;
    memcpy(&mat, viewproj, sizeof(float) * 16);
    ExtractPlanesGL(f.planes, mat, true);

    for (int i = 0; i < count; i++) {
        int visible =
            ClassifySphere(f.planes[0], nodes[i]) &&
            ClassifySphere(f.planes[1], nodes[i]) &&
            ClassifySphere(f.planes[2], nodes[i]) &&
            ClassifySphere(f.planes[3], nodes[i]) &&
            ClassifySphere(f.planes[4], nodes[i]) &&
            ClassifySphere(f.planes[5], nodes[i]);
        if (visible) {
            memcpy(out_visible_nodes[rocks_visible_count++], nodes[i], sizeof(float) * 4);
        }
    }
    return rocks_visible_count;
}
