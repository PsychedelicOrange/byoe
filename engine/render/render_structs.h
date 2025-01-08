#ifndef RENDER_STRUCTS_H
#define RENDER_STRUCTS_H

#include <stdbool.h>
#include <stdint.h>

#include "../scene/transform.h"

typedef struct vertex_buffer
{
    uint32_t count;
    uint32_t size;
    void* data;
}vertex_buffer;

typedef struct index_buffer
{
    uint32_t count;
    uint16_t* data; // We are using 16-bit indices
}index_buffer;

typedef struct color_rgba {
    float r, g, b, a;
}color_rgba;

typedef struct color_rgb {
    float r, g, b;
}color_rgb;

//------------------------
// SDF Renderer Structs
//------------------------

// Add new types here as you need
typedef enum SDF_PrimitiveType
{
    SDF_PRIM_Cube,        // dummy for testing
    SDF_PRIM_Sphere,      // aka planet + testing
    SDF_PRIM_Capsule,
    SDF_PRIM_Cylinder,
    // Game Specific Primitives
    SDF_PRIM_Planet,
    SDF_PRIM_Spaceship,
    SDF_PRIM_Bullet,
    SDF_PRIM_Ghost
} SDF_PrimitiveType;

typedef enum SDF_OperationType {
    SDF_OP_UNION,
    SDF_OP_INTERSECTION,
    SDF_OP_DIFFERENCE
} SDF_OperationType;

typedef struct SDF_Primitive {
    SDF_PrimitiveType type;
    Transform transform; // Position, Rotation, Scale
    vec3 size;           // Dimensions (for box, cylinder, etc.)
    float radius;        // Radius (for sphere, capsule, etc.)
    vec3 start;          // Start point (for capsule or line segment)
    vec3 end;            // End point (for capsule or line segment)
    // add more props here combine them all or use a new struct/union to simplify primitive attributes 
} SDF_Primitive;

typedef struct SDF_Operation {
    SDF_OperationType type;
    int left;                // Index of the left child in the SDF node pool
    int right;               // Index of the right child in the SDF node pool
} SDF_Operation;

#endif