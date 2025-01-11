#ifndef SDF_TYPE_H
#define SDF_TYPE_H
#include <stddef.h>

/* primitive types */
typedef enum sdf_type{
    plane,
    sphere,
    box,
    rounded_box,
    box_frame,
    capsule,
    torus,
    capped_torus,
    link,
    cylinder,
    rounded_cylinder,
    cone,
    round_cone,
    frustum_cone,
    hexagonal_prism,
    triangular_prism,
    solid_angle,
    triangle,
    quad
}sdf_type;

typedef struct sdf_plane_props{
    float normal[3];
    float distance_from_origin;
}sdf_plane_props;

typedef struct sdf_sphere_props{
    float radius;
}sdf_sphere_props;

typedef struct sdf_torus_props{
    float inner_radius;
    float outer_radius;
}sdf_torus_props;

typedef union sdf_props{
    sdf_plane_props plane;
    sdf_sphere_props sphere;
    sdf_torus_props torus;
}sdf_props;

/* Operations */

typedef enum sdf_operation{
    rotate,
    translate,
    scale
}sdf_operation;

typedef struct sdf_scale_operation{
    float scale[3];
}sdf_scale_operation;

typedef struct sdf_translation_operation{
    float translation[3];
}sdf_translation_operation;

typedef union sdf_rotation_operation{
    float quaternion[4];
    float axis_angle[4];
}sdf_rotation_operation;

typedef union sdf_operation_props{
    sdf_scale_operation scale;
    sdf_rotation_operation rotation;
    sdf_translation_operation translation;
}sdf_operation_props;

/* sdf file and primitive */

typedef struct sdf{
    char* label;   /* Null terminated */
    sdf_type type;
    union sdf_props sdf_props; /* sdf specific data, will be read according to sdf_type specified*/
    sdf_operation* sdf_operations;
    sdf_operation_props* sdf_operation_props;
    size_t operation_count;
}sdf;

typedef struct sdf_primtive{
    char* label;   /* Null terminated */
    sdf* sdfs;
    size_t sdf_count;

}sdf_primitive;

typedef struct sdf_file{
    sdf_primitive* primitives;
    size_t primitive_count;
    
}sdf_file;

#endif 
