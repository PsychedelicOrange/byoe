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
    SDF_PRIM_Cube,
    SDF_PRIM_Capsule,
    SDF_PRIM_Cylinder,
    SDF_PRIM_Planet,
    SDF_PRIM_Spaceship,
    SDF_PRIM_Bullet,
    SDF_PRIM_Ghost)

ENUM(SDF_BlendType,
    SDF_BLEND_UNION,
    SDF_BLEND_SMOOTH_UNION,
    SDF_BLEND_INTERSECTION,
    SDF_BLEND_SUBTRACTION,
    SDF_BLEND_XOR)

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

STRUCT(SDF_Primitive,
    SDF_PrimitiveType type;
    Transform         transform;    // Position, Rotation, Scale
    SDF_Material      material;
    // add more props here combine them all or use a new struct/union to simplify primitive attributes
    // Or use what psyorange is doing in sdf-editor branch to represent more complex SDF props
    // What if we make them a union for easier user land API and pass packed info the GPU with wrapper functions to intgerpret them?
    // ex.
    // union
    // {
    //     struct { float capsuleRadius; float capsuleHeight; };
    //     struct { float cylinderRadius; float cylinderHeight; };
    //     // Add more shapes as needed.
    // };
)

// Blending -> these are blending method b/w two primitives (eg. smooth union, XOR, etc. )  ( again refer iq)
STRUCT(SDF_Object,
    SDF_BlendType type;
    Transform         transform;    // Position, Rotation, Scale
    int           prim_a;    // Index of the left child in the SDF node pool
    int           prim_b;    // Index of the right child in the SDF node pool
)

// This struct cannot be directly translated to the GPU, we need another helper struct to flatten it (defined in sdf_scene.h)
#ifndef SHADER_INCLUDE
STRUCT(SDF_Node, SDF_NodeType type; union {
        SDF_Primitive primitive;
        SDF_Object    object; }; bounding_sphere bounds; bool is_ref_node; bool is_culled;)
#endif
// TODO:
// Operations -> operations can act on primitives and objects alike.
// (eg. transformation, distortion, elongation, rotation, etc.)
//( taken from https://iquilezles.org/articles/distfunctions/ )
#endif