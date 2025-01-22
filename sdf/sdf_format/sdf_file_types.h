#ifndef SDF_TYPE_H
#define SDF_TYPE_H
#include "sdf/sdf_format/sdf_typedefs.h"
#include "stddef.h"
#define SDF_VERSION 0

typedef struct format_sdf_blending
{
    size_t                primitive_a;    // index in the global primitive array
    size_t                primitive_b;
    format_sdf_blend_type function;
    float                 parameters[4];
} format_sdf_blending;

typedef struct format_sdf_material
{
    char* pbr_rough_metal_texture_path;
    char* pbr_normal_texture;
    char* pbr_albedo_texture;
    float color[3];
    float parameters[4];    // shader parameters
} format_sdf_material;

typedef struct format_sdf_operation
{
    format_sdf_operation_type type;
    float                     parameters[4];
} format_sdf_operation;

typedef struct format_sdf_primitive
{
    int                       index;    // index in the global primitve array
    char*                     label;
    format_sdf_primitive_type type;
    format_sdf_primitive_prop props;
    format_sdf_material       material;
    format_sdf_operation*     operations;
    size_t                    operations_count;
} sdf_primitive;

typedef struct format_sdf_file
{
    int                  version;
    sdf_primitive*       primitives;
    size_t               primitives_count;
    format_sdf_blending* blends;
    size_t               blends_count;
} sdf_file;

#endif
