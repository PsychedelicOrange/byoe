
#ifndef RENDER_STRUCTS_H
#define RENDER_STRUCTS_H

#include "../core/common.h"
#include "../core/uuid/uuid.h"

#include <stdbool.h>
#include <stdint.h>

#include "../scene/transform.h"

// TODO: use the BitSet DS for flags don't waste space on booleans

//-----------------------------
// RayMarching Settings
#define MAX_STEPS    128
#define RAY_MIN_STEP 0.01
#define RAY_MAX_STEP 100.0
#define EPSILON      0.01

#define MAX_GPU_STACK_SIZE 32
//-----------------------------

// Docs: https://github.com/PsychedelicOrange/byoe/pull/10

STRUCT(
    color_rgba,
    union {
        struct
        {
            float r;
            float g;
            float b;
            float a;
        };
        float raw[4];
    };)

STRUCT(
    color_rgb,
    union {
        struct
        {
            float r;
            float g;
            float b;
        };
        float raw[3];
    };)

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

#define MAX_BACKBUFFERS         3
#define MAX_FRAME_INFLIGHT      2
#define MAX_CMD_BUFFS_PER_QUEUE 16
#define MAX_RT                  8

typedef enum gfx_format
{
    GFX_FORMAT_NONE,
    GFX_FORMAT_R8INT,
    GFX_FORMAT_R8UINT,
    GFX_FORMAT_R8F,
    GFX_FORMAT_R16F,
    GFX_FORMAT_R32F,
    GFX_FORMAT_R32UINT,
    GFX_FORMAT_R32UNORM,
    GFX_FORMAT_R32INT,
    GFX_FORMAT_RGBINT,
    GFX_FORMAT_RGBUINT,
    GFX_FORMAT_RGBUNORM,
    GFX_FORMAT_RGB32F,
    GFX_FORMAT_RGBAINT,
    GFX_FORMAT_RGBAUNORM,
    GFX_FORMAT_RGBA32F,
    GFX_FORMAT_DEPTH32F,
    GFX_FORMAT_DEPTH16UNORM,
    GFX_FORMAT_DEPTHSTENCIL,    // d24s8 standard
    GFX_FORMAT_SCREEN           // bgra8_unorm swapchain preferred format
} gfx_format;

typedef enum gfx_shader_stage
{
    GFX_SHADER_STAGE_VS,
    GFX_SHADER_STAGE_PS,
    GFX_SHADER_STAGE_CS,
    GFX_SHADER_STAGE_GS,
    GFX_SHADER_STAGE_TES,
    GFX_SHADER_STAGE_TCS,
    GFX_SHADER_STAGE_MS,
    GFX_SHADER_STAGE_AS,
    GFX_SHADER_STAGE_RAY_HIT,
    GFX_SHADER_STAGE_RAY_GEN
} gfx_shader_stage;

typedef enum gfx_fence_type
{
    GFX_FENCE_TYPE_CPU,
    GFX_FENCE_TYPE_GPU
} gfx_fence_type;

typedef enum gfx_pipeline_type
{
    GFX_PIPELINE_TYPE_GRAPHICS,
    GFX_PIPELINE_TYPE_COMPUTE
} gfx_pipeline_type;

typedef enum gfx_draw_type
{
    GFX_DRAW_TYPE_POINT,
    GFX_DRAW_TYPE_TRIANGLE,
    GFX_DRAW_TYPE_LINE
} gfx_draw_type;

typedef enum gfx_polygon_mode
{
    GFX_POLYGON_MODE_FILL,
    GFX_POLYGON_MODE_WIRE,
    GFX_POLYGON_MODE_POINT
} gfx_polygon_mode;

typedef enum gfx_cull_mode
{
    GFX_CULL_MODE_BACK,
    GFX_CULL_MODE_FRONT,
    GFX_CULL_MODE_BACK_AND_FRONT,
    GFX_CULL_MODE_NO_CULL
} gfx_cull_mode;

typedef enum gfx_blend_op
{
    GFX_BLEND_OP_ADD,
    GFX_BLEND_OP_SUBTRACT,
    GFX_BLEND_OP_REVERSE_SUBTRACT,
    GFX_BLEND_OP_MIN,
    GFX_BLEND_OP_MAX
} gfx_blend_op;

typedef enum gfx_blend_factor
{
    GFX_BLEND_FACTOR_ZERO,
    GFX_BLEND_FACTOR_ONE,
    GFX_BLEND_FACTOR_SRC_COLOR,
    GFX_BLEND_FACTOR_ONE_MINUS_SRC_COLOR,
    GFX_BLEND_FACTOR_DST_COLOR,
    GFX_BLEND_FACTOR_ONE_MINUS_DST_COLOR,
    GFX_BLEND_FACTOR_SRC_ALPHA,
    GFX_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
    GFX_BLEND_FACTOR_DST_ALPHA,
    GFX_BLEND_FACTOR_ONE_MINUS_DST_ALPHA,
    GFX_BLEND_FACTOR_CONSTANT_COLOR,
    GFX_BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR,
    GFX_BLEND_FACTOR_CONSTANT_ALPHA,
    GFX_BLEND_FACTOR_ONE_MINUS_CONSTANT_ALPHA,
    GFX_BLEND_FACTOR_SRC_ALPHA_SATURATE
} gfx_blend_factor;

typedef enum gfx_compare_op
{
    GFX_COMPARE_OP_NEVER,
    GFX_COMPARE_OP_LESS,
    GFX_COMPARE_OP_EQUAL,
    GFX_COMPARE_OP_LESS_OR_EQUAL,
    GFX_COMPARE_OP_GREATER,
    GFX_COMPARE_OP_NOT_EQUAL,
    GFX_COMPARE_OP_GREATER_OR_EQUAL,
    GFX_COMPARE_OP_ALWAYS
} gfx_compare_op;

typedef enum gfx_descriptor_type
{
    GFX_DESCRIPTOR_TYPE_SAMPLER,
    GFX_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
    GFX_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
    GFX_DESCRIPTOR_TYPE_STORAGE_IMAGE,
    GFX_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
    GFX_DESCRIPTOR_TYPE_STORAGE_BUFFER,
    GFX_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER,
    GFX_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER,
    GFX_DESCRIPTOR_TYPE_INPUT_ATTACHMENT
} gfx_descriptor_type;

typedef struct gfx_config
{
    bool use_timeline_semaphores;
} gfx_config;

static gfx_config g_gfxConfig = {
    .use_timeline_semaphores = false,
};

// TODO: gfx_XXX memory alloc design
// void* backend will be allocated by their own backend_resource_pool_(Type)
// frontend_resource_pool_(Type) will have it's own allocators with a pointer to another backend pool elements

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
    random_uuid_t  uuid;
    void*          backend;
    uint64_t       value;    // track backend value
    gfx_fence_type visibility;
    uint32_t       _pad0[3];
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
    void*         _pad0;
    // TODO: store refs to all the command buffers allocated
} gfx_cmd_pool;

typedef struct gfx_cmd_buf
{
    random_uuid_t uuid;
    void*         backend;
    void*         _pad0;
} gfx_cmd_buf;

// Example:
//typedef struct gfx_thread_cmd
//{
//    // TODO: add atomics here
//    gfx_cmd_pool draw_cmds_pool;
//    gfx_cmd_buf*  draw_cmds[MAX_FRAME_INFLIGHT];
//} gfx_thread_cmd;

typedef struct gfx_cmd_queue
{
    const gfx_cmd_buf** cmds;
    uint32_t            cmds_count;
    uint32_t            _pad0;
} gfx_cmd_queue;

typedef struct gfx_shader
{
    random_uuid_t uuid;
    union
    {
        void* CS;
        struct
        {
            void* VS;
            void* PS;
        };
    } stages;
} gfx_shader;

typedef struct gfx_descriptor_binding
{
    uint32_t            binding;
    uint32_t            set;
    gfx_descriptor_type type;
    uint32_t            count;
    gfx_shader_stage    stage_flags;
} gfx_descriptor_binding;

typedef struct gfx_descriptor_set_layout
{
    random_uuid_t           uuid;
    gfx_descriptor_binding* bindings;
    void*                   backend;
    uint32_t                binding_count;
} gfx_descriptor_set_layout;

typedef struct gfx_descriptor_set
{
    random_uuid_t uuid;
    uint32_t      table_idx;
    uint32_t      num_descriptors;
    void*         backend;
} gfx_descriptor_set;

typedef struct gfx_push_constant
{
    random_uuid_t uuid;
    uint32_t      table_idx;
    uint32_t      _pad0;
    void*         data;
    uint32_t      size;
    uint32_t      offset;
    void*         backend;
} gfx_push_constant;

typedef struct gfx_push_constant_range
{
    uint32_t         size;
    uint32_t         offset;
    gfx_shader_stage stage;
} gfx_push_constant_range;

typedef struct gfx_descriptor_table
{
    random_uuid_t       uuid;
    uint32_t            set_count;
    uint32_t            push_constants_count;
    gfx_descriptor_set* sets;
    gfx_push_constant*  push_constants;
    uint64_t            _pad0;
} gfx_descriptor_table;

typedef struct gfx_root_signature
{
    random_uuid_t              uuid;
    gfx_descriptor_set_layout* descriptor_set_layouts;
    gfx_push_constant_range*   push_constants;
    uint32_t                   descriptor_layout_count;
    uint32_t                   push_constant_count;
    void*                      backend;
} gfx_root_signature;

typedef struct gfx_resource
{
    void*               handle;
    gfx_descriptor_type type;
    uint32_t            set;
    uint32_t            binding;
    uint32_t            _pad0;
} gfx_resource;

typedef struct gfx_pipeline_create_info
{
    gfx_shader         shader;
    gfx_root_signature root_sig;
    gfx_format         color_formats[MAX_RT];
    gfx_draw_type      draw_type;
    uint32_t           color_formats_count;
    gfx_format         depth_format;
    gfx_pipeline_type  type;
    bool               enable_depth_test;
    bool               enable_depth_write;
    bool               enable_transparency;
    bool               _pad0;
    gfx_polygon_mode   polygon_mode;
    gfx_cull_mode      cull_mode;
    gfx_compare_op     depth_compare;
    gfx_blend_factor   src_color_blend_factor;
    gfx_blend_factor   dst_color_blend_factor;
    gfx_blend_op       color_blend_op;
    gfx_blend_factor   src_alpha_blend_factor;
    gfx_blend_factor   dst_alpha_blend_factor;
    gfx_blend_op       alpha_blend_op;
    uint64_t           _pad1;
} gfx_pipeline_create_info;

typedef struct gfx_pipeline
{
    random_uuid_t uuid;
    void*         backend;
    uint64_t      _pad0;
} gfx_pipeline;

typedef struct gfx_viewport
{
    uint32_t x;
    uint32_t y;
    uint32_t width;
    uint32_t height;
    uint32_t min_depth;
    uint32_t max_depth;
} gfx_viewport;

typedef struct gfx_scissor
{
    uint32_t x;
    uint32_t y;
    uint32_t width;
    uint32_t height;
} gfx_scissor;

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
    gfx_cmd_pool  draw_cmds_pool;
    gfx_cmd_buf   draw_cmds[MAX_FRAME_INFLIGHT];
    gfx_cmd_queue cmd_queue;
} gfx_context;

typedef struct gfx_attachment
{
    color_rgba   clear_color;
    gfx_texture* attachment;
    bool         clear;
    bool         _pad0[7];
} gfx_attachment;

typedef struct gfx_render_pass
{
    alignas(16) vec2s extents;
    uint32_t       _pad0;
    uint32_t       color_attachments_count;
    gfx_attachment color_attachments[MAX_RT];
    gfx_attachment depth_attachment;
    gfx_swapchain* swapchain;
    bool           is_compute_pass;
    bool           is_swap_pass;
    bool           _pad1[6];
} gfx_render_pass;

#endif    // RENDER_STRUCTS_H
