
#ifndef RENDER_STRUCTS_H
#define RENDER_STRUCTS_H

#include "../core/common.h"
#include "../core/uuid/uuid.h"

// TODO: Add defines so as to make this include-able in shaders
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
    SDF_PRIM_VerticalCapsule,
    SDF_PRIM_Cylinder,
    SDF_PRIM_RoundedCylinder,
    SDF_PRIM_Ellipsoid,
    SDF_PRIM_HexagonalPrism,
    SDF_PRIM_TriangularPrism,
    SDF_PRIM_Cone,
    SDF_PRIM_CappedCone,
    SDF_PRIM_Plane)

ENUM(SDF_BlendType,
    SDF_BLEND_UNION,
    SDF_BLEND_INTERSECTION,
    SDF_BLEND_SUBTRACTION,
    SDF_BLEND_XOR,
    SDF_BLEND_SMOOTH_UNION,
    SDF_BLEND_SMOOTH_INTERSECTION,
    SDF_BLEND_SMOOTH_SUBTRACTION)

ENUM(SDF_Operation,
    SDF_OP_DISTORTION,
    SDF_OP_ELONGATION)

ENUM(SDF_NodeType,
    SDF_NODE_PRIMITIVE,
    SDF_NODE_OBJECT)

//-----------------------------

// TODO: If data memory gets too much, push the material to a separate buffer and use a bindless model
STRUCT(SDF_Material,
       vec4 diffuse;)

//-----------------------------

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
       vec2 thickness;)

STRUCT(torus_capped_props,
       vec2  thickness;
       float radiusA;
       float radiusB;)

STRUCT(capsule_props,
       vec3  start;
       vec3  end;
       float radius;)

STRUCT(vertical_capsule_props,
       float radius;
       float height;)

STRUCT(cylinder_props,
       float radius;
       float height;)

STRUCT(rounded_cylinder_props,
       float radiusA;
       float radiusB;
       float height;)

STRUCT(ellipsoid_props,
       vec3s radii;)

STRUCT(hexagonal_prism_props,
       float radius;
       float height;)

STRUCT(triangular_prism_props,
       float radius;
       float height;)

STRUCT(cone_props,
       float angle;
       float height;)

STRUCT(capped_cone_props,
       float radiusTop;
       float radiusBottom;
       float height;)

STRUCT(plane_props,
       vec3s normal;
       float distance;)

//-----------------------------

// TODO: perf-sweep with and without _pad_to_128_bytes_boundary

STRUCT(
    SDF_Primitive,
    SDF_PrimitiveType type;
    int               _pad0[3];
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
    } props;
    uint32_t _pad_to_128_bytes_boundary[4];)

// Blending -> these are blending method b/w two primitives (eg. smooth union, XOR, etc. )  ( again refer iq)
// Since it's shared as union it's the same size as of SDF_Primitive
STRUCT(SDF_Object,
       SDF_BlendType type;
       uint32_t      _pad0[3];
       Transform     transform;
       uint32_t      prim_a;
       uint32_t      prim_b;
       uint32_t      _pad_to_128_bytes_boundary[14];)

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

//------------------------
// Graphics API
//------------------------

#define MAX_BACKBUFFERS    3
#define MAX_FRAME_INFLIGHT 2

typedef enum gfx_format
{
    r8int,
    r8uint,
    r8f,
    r16f,
    r32f,
    r32uint,
    r32unorm,
    r32int,
    rgbint,
    rgbuint,
    rgbunorm,
    rgb32f,
    rgbaint,
    rgbauint,
    rgbaunorm,
    rgba32f,
    depth32f,
    depth16f,
    depth16unorm,
    depthstencil,
    screen
} gfx_format;

typedef enum fence_type
{
    cpu,
    gpu
} fence_type;

typedef struct gfx_config
{
    bool use_timeline_semaphores;
} gfx_config;

static gfx_config g_gfxConfig = {
    .use_timeline_semaphores = false,
};

typedef struct gfx_swapchain
{
    random_uuid_t uuid;
    void*         backend;
    uint32_t      width;
    uint32_t      height;
    uint32_t      current_backbuffer_idx;
    uint32_t      _pad0[3];
} gfx_swapchain;

typedef struct gfx_fence
{
    random_uuid_t uuid;
    void*         backend;
    uint64_t      value;    // track backend value
    fence_type    visibility;
    uint32_t      _pad0[3];
} gfx_fence;

typedef struct gfx_vertex_buffer
{
    random_uuid_t uuid;
    uint32_t      num_elements;
    uint32_t      size;
    uint32_t      offset;
    void*         backend;
} gfx_vertex_buffer;

typedef struct gfx_index_buffer
{
    random_uuid_t uuid;
    uint32_t      count;
    uint32_t      offset;
    void*         backend;
} gfx_index_buffer;

typedef struct gfx_uniform_buffer
{
    random_uuid_t uuid;
    uint32_t      size;
    uint32_t      offset;
    void*         backend;
} gfx_uniform_buffer;

typedef struct gfx_texture
{
    random_uuid_t uuid;
    // TODO: Move to texture_props --> indices to texture_prop_pool(s)
    union
    {
        bool is_read_write;
        bool is_read_only;
        bool is_write_only;
    } rw_rules;
    gfx_format format;
    void*      backend;
} gfx_texture;

typedef struct gfx_cmd_pool
{
    random_uuid_t uuid;
    void*         backend;

} gfx_cmd_pool;

typedef struct gfx_cmd_buf
{
    random_uuid_t uuid;
    void*         backend;
} gfx_cmd_buf;

//typedef struct gfx_thread_cmd
//{
//    // TODO: add atomics here
//    gfx_cmd_pool* draw_cmds_pools[MAX_FRAME_INFLIGHT];
//    gfx_cmd_buf*  draw_cmds[MAX_FRAME_INFLIGHT];
//} gfx_thread_cmd;

typedef struct gfx_cmd_queue
{
    gfx_cmd_buf** cmds;
    uint32_t      cmds_count;
    uint32_t      _pad0;
} gfx_cmd_queue;

//-----------------------------------
// High-level structs
//-----------------------------------
// Frame Sync
// Triple Buffering
typedef struct gfx_frame_sync
{
    gfx_fence in_flight;
    gfx_fence image_ready;
    gfx_fence rendering_done;
} gfx_frame_sync;

typedef struct gfx_context
{
    random_uuid_t  uuid;
    void*          backend;
    uint32_t       _pad0;
    uint32_t       frame_idx;
    gfx_swapchain  swapchain;
    gfx_frame_sync frame_sync[MAX_FRAME_INFLIGHT];
    // TODO: Add all the command buffers you want here...Draw, Async etc.
    // 1 per thread, only single threaded for now
    gfx_cmd_pool  draw_cmds_pools[MAX_FRAME_INFLIGHT];
    gfx_cmd_buf   draw_cmds[MAX_FRAME_INFLIGHT];
    gfx_cmd_queue cmd_queue;
} gfx_context;

#endif    // RENDER_STRUCTS_H
