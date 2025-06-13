
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

#define DISPATCH_LOCAL_DIM 8
//-----------------------------

// Docs: https://github.com/PsychedelicOrange/byoe/pull/10

typedef struct color_rgba
{
    union
    {
        struct
        {
            float r;
            float g;
            float b;
            float a;
        };
        float raw[4];
    };
} color_rgba;

typedef struct color_rgb
{
    union
    {
        struct
        {
            float r;
            float g;
            float b;
        };
        float raw[3];
    };
} color_rgb;

typedef struct bounding_sphere
{
    vec3  pos;
    float radius;
} bounding_sphere;

//------------------------
// SDF Renderer Structs
//------------------------

typedef enum SDF_PrimitiveType
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
} SDF_PrimitiveType;

typedef enum SDF_BlendType
{
    SDF_BLEND_UNION,
    SDF_BLEND_INTERSECTION,
    SDF_BLEND_SUBTRACTION,
    SDF_BLEND_XOR,
    SDF_BLEND_SMOOTH_UNION,
    SDF_BLEND_SMOOTH_INTERSECTION,
    SDF_BLEND_SMOOTH_SUBTRACTION
} SDF_BlendType;

typedef enum SDF_Operation
{
    SDF_OP_DISTORTION,
    SDF_OP_ELONGATION
} SDF_Operation;

typedef enum SDF_NodeType
{
    SDF_NODE_PRIMITIVE,
    SDF_NODE_OBJECT
} SDF_NodeType;

//-----------------------------

// TODO: If data memory gets too much, push the material to a separate buffer and use a bindless model
typedef struct SDF_Material
{
    vec4 diffuse;
} SDF_Material;

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
    vec3s dimensions;
    float roundness;
} round_box_props;

typedef struct box_frame_props
{
    vec3s dimensions;
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

//-----------------------------

// TODO: perf-sweep with and without _pad_to_128_bytes_boundary

typedef struct SDF_Primitive
{
    SDF_PrimitiveType type;
    int               _pad0[3];
    Transform         transform;    // Position, Rotation, Scale
    SDF_Material      material;
    union
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
    } props;
    uint32_t _pad_to_128_bytes_boundary[4];
} SDF_Primitive;

// Blending -> these are blending method b/w two primitives (eg. smooth union, XOR, etc. ) (again refer iq)
// Since it's shared as union it's the same size as of SDF_Primitive
typedef struct SDF_Object
{
    SDF_BlendType type;
    uint32_t      _pad0[3];
    Transform     transform;
    uint32_t      prim_a;
    uint32_t      prim_b;
    uint32_t      _pad_to_128_bytes_boundary[14];
} SDF_Object;

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

//------------------------
// Graphics API
//------------------------

#define MAX_BACKBUFFERS         4
#define MAX_FRAMES_INFLIGHT     2
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
    GFX_SHADER_STAGE_RAY_ANY_HIT,
    GFX_SHADER_STAGE_RAY_CLOSEST_HIT
} gfx_shader_stage;

typedef enum gfx_syncobj_type
{
    GFX_SYNCOBJ_TYPE_CPU,
    GFX_SYNCOBJ_TYPE_GPU,
    GFX_SYNCOBJ_TYPE_TIMELINE
} gfx_syncobj_type;

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

typedef enum gfx_resource_type
{
    GFX_RESOURCE_TYPE_SAMPLER,
    GFX_RESOURCE_TYPE_SAMPLED_IMAGE,
    GFX_RESOURCE_TYPE_STORAGE_IMAGE,
    GFX_RESOURCE_TYPE_UNIFORM_BUFFER,
    GFX_RESOURCE_TYPE_STORAGE_BUFFER,
    GFX_RESOURCE_TYPE_UNIFORM_TEXEL_BUFFER,
    GFX_RESOURCE_TYPE_STORAGE_TEXEL_BUFFER,
    GFX_RESOURCE_TYPE_INPUT_ATTACHMENT
} gfx_resource_type;

typedef enum gfx_texture_type
{
    GFX_TEXTURE_TYPE_1D,
    GFX_TEXTURE_TYPE_2D,
    GFX_TEXTURE_TYPE_3D,
    GFX_TEXTURE_TYPE_CUBEMAP,
    GFX_TEXTURE_TYPE_2D_ARRAY
} gfx_texture_type;

typedef enum gfx_filter_mode
{
    GFX_FILTER_MODE_NEAREST,
    GFX_FILTER_MODE_LINEAR
} gfx_filter_mode;

typedef enum gfx_wrap_mode
{
    GFX_WRAP_MODE_REPEAT,
    GFX_WRAP_MODE_MIRRORED_REPEAT,
    GFX_WRAP_MODE_CLAMP_TO_EDGE,
    GFX_WRAP_MODE_CLAMP_TO_BORDER
} gfx_wrap_mode;

typedef enum gfx_image_layout
{
    GFX_IMAGE_LAYOUT_UNDEFINED,
    GFX_IMAGE_LAYOUT_GENERAL,
    GFX_IMAGE_LAYOUT_COLOR_ATTACHMENT,
    GFX_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT,
    GFX_IMAGE_LAYOUT_TRANSFER_DST,
    GFX_IMAGE_LAYOUT_TRANSFER_SRC,
    GFX_IMAGE_LAYOUT_SHADER_READ_ONLY,
    GFX_IMAGE_LAYOUT_PRESENTATION,
} gfx_image_layout;

// Values defined in frontend.c
typedef struct gfx_config
{
    bool use_timeline_semaphores;
} gfx_config;

// Global GFX config
//------------------------------
extern gfx_config g_gfxConfig;
//------------------------------

// Draft-1 gfx_XXX memory alloc design
// void* backend will be allocated by malloc and manual free (hightly fragmented and not data-oriented)

// [Draft-2]
// TODO: void* backend will be refactored to use a uint32_t index for backend resource pools, frontend will have similar pools
// TODO: frontend_resource_pool_(Type) will have it's own allocators with a index to another backend pool elements

typedef struct gfx_syncobj
{
    random_uuid_t    uuid;
    void*            backend;
    uint64_t         wait_value;
    gfx_syncobj_type type;
    uint32_t         _pad0[3];
} gfx_syncobj;

typedef uint64_t gfx_sync_point;

typedef struct gfx_swapchain
{
    random_uuid_t uuid;
    void*         backend;
    uint32_t      width;
    uint32_t      height;
    uint32_t      image_count;
    uint32_t      current_backbuffer_idx;
    uint32_t      _pad0[2];
} gfx_swapchain;

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

typedef struct gfx_texture_create_info
{
    uint32_t          width;
    uint32_t          height;
    uint32_t          depth;
    gfx_format        format;
    gfx_texture_type  tex_type;
    gfx_resource_type res_type;
} gfx_texture_create_info;

typedef struct gfx_texture
{
    random_uuid_t uuid;
    void*         backend;
} gfx_texture;

typedef struct gfx_sampler_create_info
{
    gfx_filter_mode min_filter;
    gfx_filter_mode mag_filter;
    gfx_wrap_mode   wrap_mode;
    float           min_lod;
    float           max_lod;
    float           max_anisotropy;
} gfx_sampler_create_info;

typedef struct gfx_sampler
{
    random_uuid_t uuid;
    void*         backend;
} gfx_sampler;

typedef struct gfx_resource
{
    union
    {
        gfx_texture*        texture;
        gfx_uniform_buffer* ubo;
        gfx_sampler*        sampler;
    };
} gfx_resource;

typedef struct gfx_resource_view_create_info
{
    const gfx_resource* resource;
    gfx_resource_type   res_type;
    uint32_t            _pad0;
    union
    {
        struct
        {
            gfx_format       format;
            gfx_texture_type texture_type;
            uint32_t         base_mip;
            uint32_t         mip_levels;
            uint32_t         base_layer;
            uint32_t         layer_count;
        } texture;

        struct
        {
            uint32_t offset;
            uint32_t size;
        } buffer;
    };
} gfx_resource_view_create_info;

typedef struct gfx_binding_location
{
    uint32_t set;
    uint32_t binding;
} gfx_binding_location;

typedef struct gfx_resource_view
{
    random_uuid_t        uuid;
    void*                backend;
    gfx_resource_type    type;
    gfx_binding_location location;
    uint32_t             _pad0[3];
} gfx_resource_view;

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
    gfx_cmd_buf* cmds[MAX_CMD_BUFFS_PER_QUEUE];
    uint32_t     cmds_count;
    uint32_t     _pad0[3];
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
    gfx_binding_location location;
    gfx_resource_type    type;    // can we remove this from here?
    uint32_t             count;
    gfx_shader_stage     stage_flags;
} gfx_descriptor_binding;

typedef struct gfx_descriptor_set_layout
{
    gfx_descriptor_binding* bindings;
    uint32_t                binding_count;
} gfx_descriptor_set_layout;

typedef struct gfx_push_constant
{
    uint32_t         size;
    uint32_t         offset;
    gfx_shader_stage stage;
    uint32_t         _pad0;
    void*            data;
} gfx_push_constant;

typedef struct gfx_push_constant_range
{
    uint32_t         size;
    uint32_t         offset;
    gfx_shader_stage stage;
} gfx_push_constant_range;

typedef struct gfx_descriptor_table
{
    random_uuid_t uuid;
    void*         backend;
    uint32_t      set_count;
    uint32_t      _pad0;
} gfx_descriptor_table;

typedef struct gfx_descriptor_table_entry
{
    const gfx_resource*      resource;
    const gfx_resource_view* resource_view;
    gfx_binding_location     location;
} gfx_descriptor_table_entry;

typedef struct gfx_root_signature
{
    random_uuid_t                    uuid;
    const gfx_descriptor_set_layout* descriptor_set_layouts;
    const gfx_push_constant_range*   push_constants;
    uint32_t                         descriptor_layout_count;
    uint32_t                         push_constant_count;
    void*                            backend;
} gfx_root_signature;

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

typedef struct gfx_submit_syncobj
{
    const gfx_syncobj*  wait_synobjs;
    const gfx_syncobj*  signal_synobjs;
    uint32_t            wait_syncobjs_count;
    uint32_t            signal_syncobjs_count;
    // CPU sync primitve to wait on: Fence or Timeline Semaphore
    gfx_syncobj*        inflight_syncobj;
    // Global timeline synnc point that will be signalled when the submit operation is completed
    // Workloads can wait on this or intermediate points, 
    // this is sued to increment Values to singnal queue submits
    // this is tracked per-inflight farme, gfx_context owns This
    uint64_t*           timeline_syncpoint;
} gfx_submit_syncobj;

typedef struct gfx_context
{
    random_uuid_t uuid;
    void*         backend;
    uint32_t      current_syncobj_idx;
    uint32_t      inflight_frame_idx;
    gfx_swapchain swapchain;
    gfx_syncobj   inflight_syncobj[MAX_FRAMES_INFLIGHT];
    gfx_syncobj   image_ready[MAX_BACKBUFFERS];
    gfx_syncobj   rendering_done[MAX_BACKBUFFERS];
    // NOTE: Add all the command buffers you want here...Draw, Async etc.
    // 1 per thread, only single threaded for now
    gfx_cmd_pool draw_cmds_pool;
    // FIXME: Do we really need 2 of these draw_cmds and queue? can't we collapse and use 1?
    gfx_cmd_buf   draw_cmds[MAX_FRAMES_INFLIGHT];
    gfx_cmd_queue cmd_queue;
    uint64_t      timeline_syncpoint[MAX_FRAMES_INFLIGHT];    // last timeline value signaled
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

typedef struct gfx_texture_readback
{
    alignas(16) uint32_t width;
    uint32_t height;
    uint32_t bits_per_pixel;
    uint32_t _pad0;
    char*    pixels;
    bool     _pad1[8];
} gfx_texture_readback;

#endif    // RENDER_STRUCTS_H
