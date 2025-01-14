#ifndef RENDER_STRUCTS_H
#define RENDER_STRUCTS_H

// TODO: Add defines so as to make this inlcude-able in shaders

#include <stdbool.h>
#include <stdint.h>

#include "../scene/transform.h"

// Docs: https://github.com/PsychedelicOrange/byoe/pull/10

typedef struct color_rgba
{
    float r, g, b, a;
} color_rgba;

typedef struct color_rgb
{
    float r, g, b;
} color_rgb;

typedef struct bounding_sphere
{
    vec3  pos;
    float radius;
} bounding_sphere;

//------------------------
// SDF Renderer Structs
//------------------------

// Add new types here as you need
typedef enum SDF_PrimitiveType
{
    SDF_PRIM_Sphere = 0,
    SDF_PRIM_Cube,
    SDF_PRIM_Capsule,
    SDF_PRIM_Cylinder,
    // Game Specific Primitives presets!
    SDF_PRIM_Planet,
    SDF_PRIM_Spaceship,
    SDF_PRIM_Bullet,
    SDF_PRIM_Ghost
} SDF_PrimitiveType;

typedef enum SDF_BlendType
{
    SDF_BLEND_UNION,
    SDF_BLEND_SMOOTH_UNION,
    SDF_BLEND_INTERSECTION,
    SDF_BLEND_SUBTRACTION,
    SDF_BLEND_XOR
} SDF_BlendType;

typedef enum SDF_Operation
{
    SDF_OP_TRANSLATE,
    SDF_OP_ROTATE,
    SDF_OP_SCALE,
    SDF_OP_DISTORTION,
    SDF_OP_ELONGATION
} SDF_Operation;

// TODO: If data memory gets too much, push the material to a separate buffer and use a bindless model
typedef struct SDF_Material
{
    vec4 diffuse;
} SDF_Material;

typedef struct SDF_Primitive
{
    SDF_PrimitiveType type;
    Transform         transform;    // Position, Rotation, Scale
    SDF_Material      material;
    // add more props here combine them all or use a new struct/union to simplify primitive attributes
    // Or use what psyorange is doing in sdf-editor branch to represent more complex SDF props
    // What if we make them a union for easier user land API and pass packed info the GPU with wrapper functions to intgerpret them?
} SDF_Primitive;

// Blending -> these are blending method b/w two primitives (eg. smooth union, XOR, etc. )  ( again refer iq)
typedef struct SDF_Object
{
    SDF_BlendType type;
    int           prim_a;    // Index of the left child in the SDF node pool
    int           prim_b;    // Index of the right child in the SDF node pool
} SDF_Object;

typedef enum SDF_NodeType
{
    SDF_NODE_PRIMITIVE,
    SDF_NODE_OBJECT
} SDF_NodeType;

typedef struct SDF_Node
{
    SDF_NodeType type;
    union
    {
        SDF_Primitive primitive;
        SDF_Object    object;
    };
    bounding_sphere bounds;
    bool            is_ref_node;
    bool            is_culled;
} SDF_Node;

// TODO:
// Operations -> operations can act on primitives and objects alike.
// (eg. transformation, distortion, elongation, rotation, etc.)
//( taken from https://iquilezles.org/articles/distfunctions/ )

#endif