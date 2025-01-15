
#ifndef RENDER_STRUCTS_H
#define RENDER_STRUCTS_H

#include "../core/common.h"

// TODO: Add defines so as to make this inlcude-able in shaders
#ifndef SHADER_INCLUDE
    #include <stdbool.h>
    #include <stdint.h>
#endif

#include "../scene/transform.h"

//-----------------------------
// RayMarching Settings
#define MAX_STEPS    128
#define RAY_MIN_STEP 0.01
#define RAY_MAX_STEP 100.0
#define EPSILON      0.01

#define MAX_GPU_STACK_SIZE 32
//-----------------------------

// Docs: https://github.com/PsychedelicOrange/byoe/pull/10

STRUCT(color_rgba,
       float r, g, b, a;)

STRUCT(color_rgb,
       float r, g, b;)

STRUCT(bounding_sphere,
       vec3  pos;
       float radius;)

//------------------------
// SDF Renderer Structs
//------------------------

ENUM(SDF_PrimitiveType,
    SDF_PRIM_Sphere,
    SDF_PRIM_Box,
    SDF_PRIM_RoundedBox,
    SDF_PRIM_BoxFrame,
    SDF_PRIM_Torus,
    SDF_PRIM_TorusCapped,

    SDF_PRIM_Capsule,
    SDF_PRIM_Cylinder,
    SDF_PRIM_Ellipsoid,
    SDF_PRIM_Quad,
    SDF_PRIM_HexagonalPrism,
    SDF_PRIM_TriangularPrism,
    SDF_PRIM_Cone,
    SDF_PRIM_ConeSection,
    SDF_PRIM_Plane,
    SDF_PRIM_RoundedCylinder,
    SDF_PRIM_SolidAngle,
    SDF_PRIM_Line,
    SDF_PRIM_RoundedCone,
    SDF_PRIM_VerticalCapsule,
    SDF_PRIM_CappedCone,
    SDF_PRIM_CappedTorus,
    SDF_PRIM_CappedCylinder,
    SDF_PRIM_CappedPlane)

ENUM(SDF_BlendType,
    SDF_BLEND_UNION,
    SDF_BLEND_INTERSECTION,
    SDF_BLEND_SUBTRACTION,
    SDF_BLEND_XOR,
    SDF_BLEND_SMOOTH_UNION,
    SDF_BLEND_SMOOTH_INTERSECTION,
    SDF_BLEND_SMOOTH_SUBTRACTION)

ENUM(SDF_Operation,
    SDF_OP_TRANSLATE,
    SDF_OP_ROTATE,
    SDF_OP_SCALE,
    SDF_OP_DISTORTION,
    SDF_OP_ELONGATION)

ENUM(SDF_NodeType,
    SDF_NODE_PRIMITIVE,
    SDF_NODE_OBJECT)

// TODO: If data memory gets too much, push the material to a separate buffer and use a bindless model
STRUCT(SDF_Material,
       vec4 diffuse;)

//center is set by transform node
STRUCT(sphere_props,
       float radius;)

STRUCT(box_props,
       vec3s dimensions;)

STRUCT(round_box_props,
       vec3s dimensions;
       float roundness;)

STRUCT(box_frame_props,
       vec3s dimensions;
       float thickness;)

STRUCT(torus_props,
       float thickness;)

STRUCT(torus_capped_props,
       float thickness;)

STRUCT(capsule_props,
       float radius;
       float height;)

STRUCT(cylinder_props,
       float radius;
       float height;)

STRUCT(ellipsoid_props,
       vec3s radii;)

STRUCT(quad_props,
       vec2s dimensions;)

STRUCT(hexagonal_prism_props,
       float radius;
       float height;)

STRUCT(triangular_prism_props,
       float radius;
       float height;)

STRUCT(cone_props,
       float radius;
       float height;)

STRUCT(cone_section_props,
       float radiusTop;
       float radiusBottom;
       float height;)

STRUCT(plane_props,
       vec3s normal;
       float distance;)

STRUCT(rounded_cylinder_props,
       float radius;
       float height;
       float roundness;)

STRUCT(solid_angle_props,
       float angle;
       float radius;)

STRUCT(line_props,
       vec3s start;
       vec3s end;
       float radius;)

STRUCT(rounded_cone_props,
       float radiusTop;
       float radiusBottom;
       float height;)

STRUCT(vertical_capsule_props,
       float radius;
       float height;)

STRUCT(capped_cone_props,
       float radiusTop;
       float radiusBottom;
       float height;)

STRUCT(capped_torus_props,
       float radiusMajor;
       float radiusMinor;)

STRUCT(capped_cylinder_props,
       float radius;
       float height;)

STRUCT(capped_plane_props,
       vec2s dimensions;
       float radius;)

STRUCT(
    SDF_Primitive,
    SDF_PrimitiveType type;
    Transform         transform;    // Position, Rotation, Scale
    SDF_Material      material;
    union {
        sphere_props           sphere;
        box_props              box;
        round_box_props        round_box;
        box_frame_props        box_frame;
        torus_props            torus;
        torus_capped_props     torus_capped;
        capsule_props          capsule;
        cylinder_props         cylinder;
        ellipsoid_props        ellipsoid;
        quad_props             quad;
        hexagonal_prism_props  hexagonal_prism;
        triangular_prism_props triangular_prism;
        cone_props             cone;
        cone_section_props     cone_section;
        plane_props            plane;
        rounded_cylinder_props rounded_cylinder;
        solid_angle_props      solid_angle;
        line_props             line;
        rounded_cone_props     rounded_cone;
        vertical_capsule_props vertical_capsule;
        capped_cone_props      capped_cone;
        capped_torus_props     capped_torus;
        capped_cylinder_props  capped_cylinder;
        capped_plane_props     capped_plane;
        // Add more as needed
        // Max of 8 floats are needed to pack all props in a flattened view for GPU, update this as props get larger
        vec4s packed_props[2];
    } props;)

// Blending -> these are blending method b/w two primitives (eg. smooth union, XOR, etc. )  ( again refer iq)
STRUCT(SDF_Object,
       SDF_BlendType type;
       Transform     transform;    // Position, Rotation, Scale
       int           prim_a;       // Index of the left child in the SDF node pool
       int           prim_b;       // Index of the right child in the SDF node pool
)

// This struct cannot be directly translated to the GPU, we need another helper struct to flatten it (defined in sdf_scene.h)
#ifndef SHADER_INCLUDE
STRUCT(
    SDF_Node,
    SDF_NodeType type;
    union {
        SDF_Primitive primitive;
        SDF_Object    object;
    };
    bounding_sphere bounds;
    bool            is_ref_node;
    bool            is_culled;)
#endif
// TODO:
// Operations -> operations can act on primitives and objects alike.
// (eg. transformation, distortion, elongation, rotation, etc.)
//( taken from https://iquilezles.org/articles/distfunctions/ )
#endif