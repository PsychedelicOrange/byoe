#ifndef RENDER_STRUCTS_H
#define RENDER_STRUCTS_H

#include <stdbool.h>
#include <stdint.h>

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

typedef struct sdf_function
{
    char prototype_struct;
}sdf_function;

enum SDF_Primitives
{
    SDF_PR_Cube = 0,    // dummy for testing
    SDF_PR_Sphere,      // aka planet + testing
    // Game Specific Primitives
    SDF_PR_Planet,
    SDF_PR_Spaceship,
    SDF_PR_Bullet,
    SDF_PR_Ghost,
    SDF_PR_COUNT
};

#endif