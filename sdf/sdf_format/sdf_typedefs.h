#ifndef SDF_FORMAT_TYPES_H
#define SDF_FORMAT_TYPES_H

#ifndef DONT_USE_CGLM_TYPES
    #include "cglm/types-struct.h"
#endif
#ifdef DONT_USE_CGLM_TYPES
typedef float vec2[2];
typedef float vec3[3];
typedef float vec4[4];
typedef float vec2s[2];
typedef float vec3s[3];
typedef float vec4s[4];
#endif

// all char* are null terminated strings
typedef enum format_sdf_primitive_type
{
    SDF_PRIM_Sphere,
    SDF_PRIM_Box,
    SDF_PRIM_RoundedBox,
    SDF_PRIM_BoxFrame,
    SDF_PRIM_Torus,
    SDF_PRIM_TorusCapped,
    SDF_PRIM_Capsule,
    SDF_PRIM_VerticalCapsule,
    SDF_PRIM_Cylinder,
    SDF_PRIM_RoundedCylinder,
    SDF_PRIM_Ellipsoid,
    SDF_PRIM_HexagonalPrism,
    SDF_PRIM_TriangularPrism,
    SDF_PRIM_Cone,
    SDF_PRIM_CappedCone,
    SDF_PRIM_Plane
} format_sdf_primitive_type;

typedef enum format_sdf_blend_type
{
    SDF_BLEND_UNION,
    SDF_BLEND_INTERSECTION,
    SDF_BLEND_SUBTRACTION,
    SDF_BLEND_XOR,
    SDF_BLEND_SMOOTH_UNION,
    SDF_BLEND_SMOOTH_INTERSECTION,
    SDF_BLEND_SMOOTH_SUBTRACTION
} format_sdf_blend_type;

typedef enum format_sdf_operation_type
{
    SDF_OP_DISTORTION,
    SDF_OP_ELONGATION,
    SDF_OP_TRANSLATION,
    SDF_OP_ROTATION,
    SDF_OP_SCALING
} format_sdf_operation_type;

//-----------------------------

typedef struct sphere_props
{
    float radius;
} sphere_props;

typedef struct box_props
{
    vec3s dimensions;
} box_props;

typedef struct round_box_props
{
    vec3  dimensions;
    float roundness;
} round_box_props;

typedef struct box_frame_props
{
    float dimensions;
    float thickness;
} box_frame_props;

typedef struct torus_props
{
    vec2 thickness;
} torus_props;

typedef struct torus_capped_props
{
    vec2  thickness;
    float radiusA;
    float radiusB;
} torus_capped_props;

typedef struct capsule_props
{
    vec3  start;
    vec3  end;
    float radius;
} capsule_props;

typedef struct vertical_capsule_props
{
    float radius;
    float height;
} vertical_capsule_props;

typedef struct cylinder_props
{
    float radius;
    float height;
} cylinder_props;

typedef struct rounded_cylinder_props
{
    float radiusA;
    float radiusB;
    float height;
} rounded_cylinder_props;

typedef struct ellipsoid_props
{
    vec3s radii;
} ellipsoid_props;

typedef struct hexagonal_prism_props
{
    float radius;
    float height;
} hexagonal_prism_props;

typedef struct triangular_prism_props
{
    float radius;
    float height;
} triangular_prism_props;

typedef struct cone_props
{
    float angle;
    float height;
} cone_props;

typedef struct capped_cone_props
{
    float radiusTop;
    float radiusBottom;
    float height;
} capped_cone_props;

typedef struct plane_props
{
    vec3s normal;
    float distance;
} plane_props;

typedef union format_sdf_primitive_prop
{
    sphere_props           sphere;
    box_props              box;
    round_box_props        round_box;
    box_frame_props        box_frame;
    torus_props            torus;
    torus_capped_props     torus_capped;
    capsule_props          capsule;
    vertical_capsule_props vertical_capsule;
    cylinder_props         cylinder;
    rounded_cylinder_props rounded_cylinder;
    ellipsoid_props        ellipsoid;
    hexagonal_prism_props  hexagonal_prism;
    triangular_prism_props triangular_prism;
    cone_props             cone;
    plane_props            plane;
    capped_cone_props      capped_cone;
    // Add more as needed
    // Max of 8 floats are needed to pack all props in a flattened view for GPU, update this as props get larger
    vec4s packed_data[2];
} format_sdf_primitive_prop;

#endif
