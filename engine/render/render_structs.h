#ifndef RENDER_STRUCTS_H
#define RENDER_STRUCTS_H

#include "../core/common.h"

// TODO: Add defines so as to make this include-able in shaders
#ifndef SHADER_INCLUDE
    #include <stdbool.h>
    #include <stdint.h>
#endif
#define USE_CGLM_TYPES
#include "../../sdf/sdf_format/sdf_typedefs.h"
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

ENUM(SDF_NodeType,
    SDF_NODE_PRIMITIVE,
    SDF_NODE_OBJECT)

//-----------------------------

// TODO: If data memory gets too much, push the material to a separate buffer and use a bindless model
STRUCT(SDF_Material,
    vec4 diffuse;)

//-----------------------------

// TODO: perf-sweep with and without _pad_to_128_bytes_boundary

STRUCT(
    SDF_Primitive,
    format_sdf_primitive_type type;
    int                       _pad0[3];
    Transform                 transform;    // Position, Rotation, Scale
    SDF_Material              material;
    format_sdf_primitive_prop props;
    uint32_t                  _pad_to_128_bytes_boundary[4];)

// Blending -> these are blending method b/w two primitives (eg. smooth union, XOR, etc. )  ( again refer iq)
// Since it's shared as union it's the same size as of SDF_Primitive
STRUCT(SDF_Object,
    format_sdf_blend_type type;
    uint32_t              _pad0[3];
    Transform             transform;
    uint32_t              prim_a;
    uint32_t              prim_b;
    uint32_t              _pad_to_128_bytes_boundary[14];)

#ifndef SHADER_INCLUDE
// This struct cannot be directly translated to the GPU, we need another helper struct to flatten it (defined in sdf_scene.h)
typedef struct SDF_Node
{
    SDF_NodeType type;
    uint32_t     _pad0[3];
    union
    {
        SDF_Primitive primitive;
        SDF_Object    object;
    };
    bounding_sphere bounds;
    bool            is_ref_node;
    bool            is_culled;
    bool            is_dirty;
    bool            _pad1[13];
} SDF_Node;
#endif    // SHADER_INCLUDE

#endif    // RENDER_STRUCTS_H
