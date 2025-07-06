#include "backend_dx12.h"

#ifdef _WIN32

    #include "../core/common.h"
    #include "../core/containers/typed_growable_array.h"
    #include "../core/logging/log.h"

    #include "../core/memory/memalign.h"
    #include "../core/shader.h"
    #include <stdint.h>
// clang-format off
#ifdef _WIN32 // careful of the order
    #include <windows.h>
#define COBJMACROS // C Object Macros for DX12
#ifdef _DEBUG
#include <dxgidebug.h>
#endif
#include <d3d12.h>
#include <d3d12sdklayers.h>
#include <dxgi1_6.h>
#include <d3dcompiler.h>
#endif

#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

// clang-format on

    #include <inttypes.h>
    #include <stdlib.h>
    #include <string.h>    // memset

    #define COLOR_ACQUIRE     0xFF8800    // Orange
    #define COLOR_WAIT        0xFF4444    // Red
    #define COLOR_PRESENT     0x00CCFF    // Light Blue
    #define COLOR_CMD_SUBMIT  0x00FF00    // Green
    #define COLOR_CMD_RECORD  0xCCCC00    // Yellow
    #define COLOR_RENDER_PASS 0xAA88FF    // Violet

    #include <tracy/TracyC.h>

//--------------------------------------------------------
// TODO:
// - [x] Use the IDXGIFactory7 to query for the best GPUa and create the IDXGIAdapter4 or use AgilitySDK with DeviceFactory
//   - [x] Figure out how to replicate COM model in C, use lpVtbl and manually release memory for ID3DXXXX/iDXGIXXXX objects
// - [x] Create the device
// - [x] cache the device Features
// - [x] Debug Layers (D3D12 + DXGI + LiveObjectTracking)
// - [x] Create swapchain and RTV heaps and extract back buffer
// - [x] gfx_syncobj primitives
// - [x] Create command allocators
// - [x] Create command lists from allocator
// - [x] Rendering Loop: Basic submit/present flow using Fence counting etc. for Clear color
//   - [x] frame_begin & end/wait_on_cmds/submit/present
//   - [x] Create in context! + decide on per-in flight fence or just one global fence
//   - [x] Test and resolve issues
//   - [x] Swapchain resize
//   - [x] **SUBMIT** API
// - [x] Hello Triangle without VB/IB in single shader quad
//   - [x] Begin/End RenderpPass API (dynamic rendering VK) (setup RT/DRT etc. clear them...)
//   - [x] Drawing API
//   - [x] Shaders/Loading (re-use the SPIRV compiled, and GLSL as source for DX no HLSL!)
//   - [x] Root Signature
//   - [x] Pipeline API
//   - [x] Hello Triangle Shader + Draw loop
// - [x] Create Texture/Buffer/Sampler resources
//   - [x] Textures
//   - [x] Buffers
//   - [x] Sampler
// - [x] Resource Views API
//   - [x] Textures
//   - [x] Buffers
//   - [x] Sampler
// - [x] Descriptor Heaps/tables API + root signature hookup
// - [x] Barriers API
// - [x] Root Constants
// - [ ] Restore SDF renderer
//   - [ ] Clear Image
//   - [ ] Bindings stuff etc. and whatever needs to be done to not crash it
// - [ ] Single time command buffer
// - [ ] Swapchain read back
// - [ ] Restore tests!

//--------------------------------------------------------
const rhi_jumptable dx12_jumptable = {
    dx12_ctx_init,
    dx12_ctx_destroy,
    dx12_flush_gpu_work,

    dx12_create_swapchain,
    dx12_destroy_swapchain,
    dx12_create_syncobj,
    dx12_destroy_syncobj,
    dx12_create_gfx_cmd_allocator,
    dx12_destroy_gfx_cmd_allocator,
    dx12_create_gfx_cmd_buf,
    dx12_free_gfx_cmd_buf,
    dx12_create_compute_shader,
    dx12_destroy_compute_shader,
    dx12_create_vs_ps_shader,
    dx12_destroy_vs_ps_shader,
    dx12_create_root_signature,
    dx12_destroy_root_signature,
    dx12_create_pipeline,
    dx12_destroy_pipeline,
    dx12_create_descriptor_table,
    dx12_destroy_descriptor_table,
    dx12_update_descriptor_table,
    dx12_create_texture_resource,
    dx12_destroy_texture_resource,
    dx12_create_sampler,
    dx12_destroy_sampler,
    dx12_create_uniform_buffer_resource,
    dx12_destroy_uniform_buffer_resource,
    dx12_update_uniform_buffer,
    dx12_create_texture_resource_view,
    dx12_destroy_texture_resource_view,
    dx12d_create_sampler_resource_view,
    dx12d_destroy_sampler_resource_view,
    dx12_create_uniform_buffer_resource_view,
    dx12_destroy_uniform_buffer_resource_view,
    NULL,    // create single time cmd buffer
    NULL,    // destroy single time cmd buffer
    NULL,    // read back swapchain
    dx12_frame_begin,
    dx12_frame_end,
    dx12_wait_on_previous_cmds,
    dx12_acquire_image,
    dx12_gfx_cmd_enque_submit,
    dx12_gfx_cmd_submit_queue,
    dx12_gfx_cmd_submit_for_rendering,
    dx12_present,
    dx12_resize_swapchain,
    dx12_begin_gfx_cmd_recording,
    dx12_end_gfx_cmd_recording,
    dx12_begin_render_pass,
    dx12_end_render_pass,
    dx12_set_viewport,
    dx12_set_scissor,
    dx12_bind_gfx_pipeline,
    dx12_bind_compute_pipeline,
    dx12_device_bind_root_signature,
    dx12_bind_descriptor_table,
    dx12_bind_push_constant,
    dx12_draw,
    dx12_dispatch,
    dx12_transition_image_layout,
    dx12_transition_swapchain_layout,
    NULL,    // clear image
};
//--------------------------------------------------------

    #define DX_CHECK_HR(x)                                                                                                           \
        do {                                                                                                                         \
            HRESULT hr__ = (x);                                                                                                      \
            if (FAILED(hr__)) {                                                                                                      \
                LOG_ERROR("[D3D12] [DX12] %s:%d\n ==> %s failed (HRESULT = 0x%08X)\n", __FILE__, __LINE__, #x, (unsigned int) hr__); \
                abort();                                                                                                             \
            }                                                                                                                        \
        } while (0)

    #define MAX_DX_SWAPCHAIN_BUFFERS 3

    #define DXDevice s_DXCtx.device

    #define SIZE_DWORD                 4
    #define MAX_ROOT_PARAMS            16
    #define MAX_DESCRIPTOR_RANGES      32
    #define SHADER_BINARY_EXT          "cso"
    #define DX_SWAPCHAIN_FORMAT        DXGI_FORMAT_B8G8R8A8_UNORM
    #define DX12_DESCRIPTOR_SIZE(type) s_DXCtx.device->lpVtbl->GetDescriptorHandleIncrementSize(s_DXCtx.device, type)

//--------------------------------------------------------
// Internal Types
//--------------------------------------------------------

typedef struct D3D12FeatureCache
{
    D3D12_FEATURE_DATA_D3D12_OPTIONS   options;
    D3D12_FEATURE_DATA_D3D12_OPTIONS1  options1;
    D3D12_FEATURE_DATA_D3D12_OPTIONS5  options5;
    D3D12_FEATURE_DATA_D3D12_OPTIONS10 options10;
    D3D12_FEATURE_DATA_D3D12_OPTIONS12 options12;

    D3D12_FEATURE_DATA_ARCHITECTURE1  architecture;
    D3D12_FEATURE_DATA_SHADER_MODEL   shaderModel;
    D3D12_FEATURE_DATA_ROOT_SIGNATURE rootSig;

    D3D12_FEATURE_DATA_GPU_VIRTUAL_ADDRESS_SUPPORT vaSupport;

    UINT nodeCount;
    BOOL isUMA;
    BOOL isCacheCoherentUMA;
} D3D12FeatureCache;

/**
 * Direct3D feature levels like 11_0, 12_0, 12_1, etc. define a baseline set of guaranteed features.
 * They determine the minimum functionality that the device must support, but additional features 
 * can still be queried independently using CheckFeatureSupport() — even beyond what the feature level guarantees.
*/

// TODO: Make this a global static context like vulkan, to avoid passing it around sometimes!
// we still pass around gfx_context which has some high-level structs but internally it will
// pollute the global functions with gfx_context, so we can keep it as a global state.
typedef struct context_backend
{
    GLFWwindow*         glfwWindow;
    IDXGIFactory7*      factory;
    IDXGIAdapter4*      gpu;
    ID3D12Device10*     device;    // Win 11 latest, other latest version needs Agility SDK
    HWND                hwnd;
    D3D_FEATURE_LEVEL   feat_level;
    D3D12FeatureCache   features;
    ID3D12CommandQueue* direct_queue;
    #ifdef _DEBUG
    ID3D12Debug3*    d3d12_debug;
    ID3D12InfoQueue* d3d12_info_queue;
    IDXGIInfoQueue*  dxgi_info_queue;
    IDXGIDebug*      dxgi_debug;
    #endif
} context_backend;

static context_backend s_DXCtx;

typedef struct swapchain_backend
{
    IDXGISwapChain4*            swapchain;
    ID3D12DescriptorHeap*       rtv_heap;
    D3D12_CPU_DESCRIPTOR_HANDLE rtv_handle_start;
    ID3D12Resource*             backbuffers[MAX_BACKBUFFERS];
    uint32_t                    image_count;
} swapchain_backend;

typedef struct syncobj_backend
{
    ID3D12Fence* fence;         // for both CPU/GPU signaling
    HANDLE       fenceEvent;    // CPU event for wait operations
} syncobj_backend;

typedef struct shader_backend
{
    gfx_shader_stage stage;
    ID3DBlob*        bytecode;
} shader_backend;

typedef struct pipeline_backend
{
    ID3D12PipelineState*   pso;
    D3D_PRIMITIVE_TOPOLOGY topology;
} pipeline_backend;

typedef struct resource_view_backend
{
    union
    {
        D3D12_SHADER_RESOURCE_VIEW_DESC  srv_desc;
        D3D12_UNORDERED_ACCESS_VIEW_DESC uav_desc;
        D3D12_RENDER_TARGET_VIEW_DESC    rtv_desc;
        D3D12_DEPTH_STENCIL_VIEW_DESC    dsv_desc;
        D3D12_CONSTANT_BUFFER_VIEW_DESC  cbv_desc;
    };
} resource_view_backend;

// TODO: Make this a global ring buffer like descriptor allocator
typedef struct descriptor_table_backend
{
    ID3D12DescriptorHeap* cbv_uav_srv_heap;
    ID3D12DescriptorHeap* sampler_heap;
    ID3D12DescriptorHeap* rtv_heap;
    ID3D12DescriptorHeap* dsv_heap;
    uint32_t              heap_count;
} descriptor_table_backend;

//--------------------------------------------------------

static D3D12_DESCRIPTOR_RANGE_TYPE dx12_util_descriptor_range_type_translate(gfx_resource_type res_type)
{
    switch (res_type) {
        case GFX_RESOURCE_TYPE_SAMPLER:
            return D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER;
        case GFX_RESOURCE_TYPE_SAMPLED_IMAGE:
        case GFX_RESOURCE_TYPE_UNIFORM_TEXEL_BUFFER:
            return D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
        case GFX_RESOURCE_TYPE_STORAGE_IMAGE:
        case GFX_RESOURCE_TYPE_STORAGE_TEXEL_BUFFER:
        case GFX_RESOURCE_TYPE_STORAGE_BUFFER:
            return D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
        case GFX_RESOURCE_TYPE_UNIFORM_BUFFER:
            return D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
        // Note: INPUT_ATTACHMENT doesn't have a direct DX12 equivalent;
        case GFX_RESOURCE_TYPE_COLOR_ATTACHMENT:
        case GFX_RESOURCE_TYPE_DEPTH_STENCIL_ATTACHMENT:
        default:
            return (D3D12_DESCRIPTOR_RANGE_TYPE) -1;    // Invalid/unsupported
    }
}

static D3D12_RESOURCE_STATES dx12_util_translate_image_layout_to_res_state(gfx_image_layout layout)
{
    switch (layout) {
        case GFX_IMAGE_LAYOUT_UNDEFINED:
            // No access intended, map to COMMON
            return D3D12_RESOURCE_STATE_COMMON;

        case GFX_IMAGE_LAYOUT_GENERAL:
            // Most general usage
            return D3D12_RESOURCE_STATE_GENERIC_READ;

        case GFX_IMAGE_LAYOUT_COLOR_ATTACHMENT:
            return D3D12_RESOURCE_STATE_RENDER_TARGET;

        case GFX_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT:
            return D3D12_RESOURCE_STATE_DEPTH_WRITE;

        case GFX_IMAGE_LAYOUT_TRANSFER_DST:
            return D3D12_RESOURCE_STATE_COPY_DEST;

        case GFX_IMAGE_LAYOUT_TRANSFER_SRC:
            return D3D12_RESOURCE_STATE_COPY_SOURCE;

        case GFX_IMAGE_LAYOUT_SHADER_READ_ONLY:
            // For textures bound as SRV in shaders
            return D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;

        case GFX_IMAGE_LAYOUT_PRESENTATION:
            return D3D12_RESOURCE_STATE_PRESENT;

        default:
            LOG_ERROR("[D3D12] Unsupported gfx_image_layout!");
            return D3D12_RESOURCE_STATE_COMMON;
    }
}

static D3D12_PRIMITIVE_TOPOLOGY_TYPE dx12_util_draw_type_prim_topology_type_translate(gfx_draw_type draw_type)
{
    switch (draw_type) {
        case GFX_DRAW_TYPE_POINT: return D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
        case GFX_DRAW_TYPE_LINE: return D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
        case GFX_DRAW_TYPE_TRIANGLE: return D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        default: return D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    }
}

static D3D_PRIMITIVE_TOPOLOGY dx12_util_draw_type_translate(gfx_draw_type draw_type)
{
    switch (draw_type) {
        case GFX_DRAW_TYPE_POINT: return D3D_PRIMITIVE_TOPOLOGY_POINTLIST;
        case GFX_DRAW_TYPE_LINE: return D3D_PRIMITIVE_TOPOLOGY_LINELIST;
        case GFX_DRAW_TYPE_TRIANGLE: return D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
        default: return D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
    }
}

static DXGI_FORMAT dx12_util_format_translate(gfx_format format)
{
    switch (format) {
        case GFX_FORMAT_NONE: return DXGI_FORMAT_UNKNOWN;
        case GFX_FORMAT_R8INT: return DXGI_FORMAT_R8_SINT;
        case GFX_FORMAT_R8UINT: return DXGI_FORMAT_R8_UINT;
        case GFX_FORMAT_R8F: return DXGI_FORMAT_R8_UNORM;    // no R8 float, unorm is closest
        case GFX_FORMAT_R16F: return DXGI_FORMAT_R16_FLOAT;
        case GFX_FORMAT_R32F: return DXGI_FORMAT_R32_FLOAT;
        case GFX_FORMAT_R32UINT: return DXGI_FORMAT_R32_UINT;
        case GFX_FORMAT_R32INT: return DXGI_FORMAT_R32_SINT;

        case GFX_FORMAT_RGBINT: return DXGI_FORMAT_R8G8B8A8_SINT;    // RGB not supported directly
        case GFX_FORMAT_RGBUINT: return DXGI_FORMAT_R8G8B8A8_UINT;
        case GFX_FORMAT_RGBUNORM: return DXGI_FORMAT_R8G8B8A8_UNORM;    // emulate RGB with RGBA
        case GFX_FORMAT_RGB32F: return DXGI_FORMAT_R32G32B32_FLOAT;

        case GFX_FORMAT_RGBAINT: return DXGI_FORMAT_R8G8B8A8_SINT;
        case GFX_FORMAT_RGBAUNORM: return DXGI_FORMAT_R8G8B8A8_UNORM;
        case GFX_FORMAT_RGBA32F: return DXGI_FORMAT_R32G32B32A32_FLOAT;

        case GFX_FORMAT_DEPTH32F: return DXGI_FORMAT_D32_FLOAT;
        case GFX_FORMAT_DEPTH16UNORM: return DXGI_FORMAT_D16_UNORM;
        case GFX_FORMAT_DEPTHSTENCIL: return DXGI_FORMAT_D24_UNORM_S8_UINT;

        case GFX_FORMAT_SCREEN: return DX_SWAPCHAIN_FORMAT;

        default: return DXGI_FORMAT_UNKNOWN;
    }
}

static D3D12_FILL_MODE dx12_util_polygon_mode_translate(gfx_polygon_mode polygon_mode)
{
    switch (polygon_mode) {
        case GFX_POLYGON_MODE_FILL: return D3D12_FILL_MODE_SOLID;
        case GFX_POLYGON_MODE_WIRE: return D3D12_FILL_MODE_WIREFRAME;
        case GFX_POLYGON_MODE_POINT: return D3D12_FILL_MODE_SOLID;
        default: return D3D12_FILL_MODE_SOLID;
    }
}

static D3D12_CULL_MODE dx12_util_cull_mode_translate(gfx_cull_mode cull_mode)
{
    switch (cull_mode) {
        case GFX_CULL_MODE_BACK: return D3D12_CULL_MODE_BACK;
        case GFX_CULL_MODE_FRONT: return D3D12_CULL_MODE_FRONT;
        case GFX_CULL_MODE_BACK_AND_FRONT: return D3D12_CULL_MODE_FRONT | D3D12_CULL_MODE_BACK;
        case GFX_CULL_MODE_NO_CULL: return D3D12_CULL_MODE_NONE;
        default: return D3D12_CULL_MODE_NONE;
    }
}

static D3D12_BLEND dx12_util_blend_factor_translate(gfx_blend_factor factor)
{
    switch (factor) {
        case GFX_BLEND_FACTOR_ZERO: return D3D12_BLEND_ZERO;
        case GFX_BLEND_FACTOR_ONE: return D3D12_BLEND_ONE;
        case GFX_BLEND_FACTOR_SRC_COLOR: return D3D12_BLEND_SRC_COLOR;
        case GFX_BLEND_FACTOR_ONE_MINUS_SRC_COLOR: return D3D12_BLEND_INV_SRC_COLOR;
        case GFX_BLEND_FACTOR_DST_COLOR: return D3D12_BLEND_DEST_COLOR;
        case GFX_BLEND_FACTOR_ONE_MINUS_DST_COLOR: return D3D12_BLEND_INV_DEST_COLOR;
        case GFX_BLEND_FACTOR_SRC_ALPHA: return D3D12_BLEND_SRC_ALPHA;
        case GFX_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA: return D3D12_BLEND_INV_SRC_ALPHA;
        case GFX_BLEND_FACTOR_DST_ALPHA: return D3D12_BLEND_DEST_ALPHA;
        case GFX_BLEND_FACTOR_ONE_MINUS_DST_ALPHA: return D3D12_BLEND_INV_DEST_ALPHA;
        case GFX_BLEND_FACTOR_CONSTANT_COLOR: return D3D12_BLEND_BLEND_FACTOR;
        case GFX_BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR: return D3D12_BLEND_INV_BLEND_FACTOR;
        case GFX_BLEND_FACTOR_CONSTANT_ALPHA: return D3D12_BLEND_BLEND_FACTOR;                  // No separate constant alpha in D3D12
        case GFX_BLEND_FACTOR_ONE_MINUS_CONSTANT_ALPHA: return D3D12_BLEND_INV_BLEND_FACTOR;    // Same as above
        case GFX_BLEND_FACTOR_SRC_ALPHA_SATURATE: return D3D12_BLEND_SRC_ALPHA_SAT;
        default: return D3D12_BLEND_ONE;
    }
}

static D3D12_BLEND_OP dx12_util_blend_op_translate(gfx_blend_op op)
{
    switch (op) {
        case GFX_BLEND_OP_ADD: return D3D12_BLEND_OP_ADD;
        case GFX_BLEND_OP_SUBTRACT: return D3D12_BLEND_OP_SUBTRACT;
        case GFX_BLEND_OP_REVERSE_SUBTRACT: return D3D12_BLEND_OP_REV_SUBTRACT;
        case GFX_BLEND_OP_MIN: return D3D12_BLEND_OP_MIN;
        case GFX_BLEND_OP_MAX: return D3D12_BLEND_OP_MAX;
        default: return D3D12_BLEND_OP_ADD;
    }
}

static D3D12_COMPARISON_FUNC dx12_util_compare_op_translate(gfx_compare_op compare_op)
{
    switch (compare_op) {
        case GFX_COMPARE_OP_NEVER: return D3D12_COMPARISON_FUNC_NEVER;
        case GFX_COMPARE_OP_LESS: return D3D12_COMPARISON_FUNC_LESS;
        case GFX_COMPARE_OP_EQUAL: return D3D12_COMPARISON_FUNC_EQUAL;
        case GFX_COMPARE_OP_LESS_OR_EQUAL: return D3D12_COMPARISON_FUNC_LESS_EQUAL;
        case GFX_COMPARE_OP_GREATER: return D3D12_COMPARISON_FUNC_GREATER;
        case GFX_COMPARE_OP_NOT_EQUAL: return D3D12_COMPARISON_FUNC_NOT_EQUAL;
        case GFX_COMPARE_OP_GREATER_OR_EQUAL: return D3D12_COMPARISON_FUNC_GREATER_EQUAL;
        case GFX_COMPARE_OP_ALWAYS: return D3D12_COMPARISON_FUNC_ALWAYS;
        default: return D3D12_COMPARISON_FUNC_ALWAYS;
    }
}

static D3D12_RESOURCE_DIMENSION dx12_util_tex_type_dimensions_translate(gfx_texture_type type)
{
    switch (type) {
        case GFX_TEXTURE_TYPE_1D:
            return D3D12_RESOURCE_DIMENSION_TEXTURE1D;
        case GFX_TEXTURE_TYPE_2D:
        case GFX_TEXTURE_TYPE_2D_ARRAY:
        case GFX_TEXTURE_TYPE_CUBEMAP:
            return D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        case GFX_TEXTURE_TYPE_3D:
            return D3D12_RESOURCE_DIMENSION_TEXTURE3D;
        default:
            return D3D12_RESOURCE_DIMENSION_UNKNOWN;
            break;
    }
}

static D3D12_FILTER_TYPE dx12_util_filter_type_translate(gfx_filter_mode filter)
{
    switch (filter) {
        case GFX_FILTER_MODE_NEAREST: return D3D12_FILTER_TYPE_POINT;
        case GFX_FILTER_MODE_LINEAR: return D3D12_FILTER_TYPE_LINEAR;
        default: return D3D12_FILTER_TYPE_LINEAR;
    }
}

static D3D12_FILTER dx12_util_filter_translate(gfx_filter_mode min_filter,
    gfx_filter_mode                                            mag_filter)
{
    bool min_linear = (min_filter == GFX_FILTER_MODE_LINEAR);
    bool mag_linear = (mag_filter == GFX_FILTER_MODE_LINEAR);

    if (!min_linear && !mag_linear)
        return D3D12_FILTER_MIN_MAG_MIP_POINT;

    if (!min_linear && mag_linear)
        return D3D12_FILTER_MIN_POINT_MAG_LINEAR_MIP_POINT;

    if (min_linear && !mag_linear)
        return D3D12_FILTER_MIN_LINEAR_MAG_MIP_POINT;

    // Both linear
    return D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT;
}

static D3D12_TEXTURE_ADDRESS_MODE dx12_util_wrap_mode_translate(gfx_wrap_mode mode)
{
    switch (mode) {
        case GFX_WRAP_MODE_REPEAT: return D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        case GFX_WRAP_MODE_MIRRORED_REPEAT: return D3D12_TEXTURE_ADDRESS_MODE_MIRROR;
        case GFX_WRAP_MODE_CLAMP_TO_EDGE: return D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        case GFX_WRAP_MODE_CLAMP_TO_BORDER: return D3D12_TEXTURE_ADDRESS_MODE_BORDER;
        default: return D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    }
}

//--------------------------------------------------------

static IDXGIAdapter4* dx12_internal_select_best_adapter(IDXGIFactory7* factory, D3D_FEATURE_LEVEL min_feat_level)
{
    // We prefer the first one if we cannot find any Discrete GPUs
    IDXGIAdapter4* best_adapter     = NULL;
    size_t         maxDedicatedVRAM = 0;
    for (uint32_t i = 0;; ++i) {
        // DXGI factory returns IDXGIAdapter1* via EnumAdapters1.
        // To access modern features (like GetDesc3), we must QueryInterface
        // the IDXGIAdapter1 to IDXGIAdapter4, which exposes DXGI_ADAPTER_DESC3.
        // We use that to inspect the adapter and decide if it's suitable.
        IDXGIAdapter1* adapter1 = NULL;

        HRESULT hr = IDXGIFactory7_EnumAdapters1(factory, i, &adapter1);
        if (hr == DXGI_ERROR_NOT_FOUND)
            break;
        if (FAILED(hr))
            continue;

        IDXGIAdapter4* adapter4 = NULL;
        hr                      = IDXGIAdapter1_QueryInterface(adapter1, &IID_IDXGIAdapter4, &adapter4);
        if (FAILED(hr)) {
            LOG_ERROR("[D3D12] [DX12] Failed to query IDXGIAdapter4 from IDXGIAdapter1 (HRESULT = 0x%08X)", (unsigned int) hr);
            IDXGIAdapter1_Release(adapter1);
            continue;
        }
        IDXGIAdapter1_Release(adapter1);

        DXGI_ADAPTER_DESC3 desc = {0};
        hr                      = IDXGIAdapter4_GetDesc3(adapter4, &desc);
        if (FAILED(hr)) {
            LOG_ERROR("[D3D12] [DX12] Failed to get DXGI_ADAPTER_DESC3 from IDXGIAdapter4 (HRESULT = 0x%08X)", (unsigned int) hr);
            IDXGIAdapter4_Release(adapter4);
            continue;
        }

        if (desc.Flags & DXGI_ADAPTER_FLAG3_SOFTWARE) {
            IDXGIAdapter4_Release(adapter4);
            continue;
        }

        if (desc.DedicatedVideoMemory > maxDedicatedVRAM) {
            if (best_adapter)
                IDXGIAdapter4_Release(best_adapter);

            maxDedicatedVRAM = desc.DedicatedVideoMemory;
            // ++ Also check if the adapter supports min feature level
            DX_CHECK_HR(D3D12CreateDevice((IUnknown*) adapter4, min_feat_level, &IID_ID3D12Device, NULL));
            best_adapter = adapter4;
            break;
        }

        if (!best_adapter) {
            best_adapter = adapter4;
        } else {
            IDXGIAdapter4_Release(adapter4);
        }
    }

    if (!best_adapter) {
        LOG_ERROR("[D3D12] [DX12] No suitable GPU found for D3D12");
        return NULL;
    } else {
        LOG_INFO("Selected Adapter Info: ");
        DXGI_ADAPTER_DESC3 adapterDesc = {0};
        IDXGIAdapter4_GetDesc3(best_adapter, &adapterDesc);
        LOG_INFO("\t -> Name                    : %ls", adapterDesc.Description);
        LOG_INFO("\t -> VendorID                : %u", adapterDesc.VendorId);
        LOG_INFO("\t -> DeviceId                : %u", adapterDesc.DeviceId);
        LOG_INFO("\t -> SubSysId                : %u", adapterDesc.SubSysId);
        LOG_INFO("\t -> Revision                : %u", adapterDesc.Revision);
        LOG_INFO("\t -> DedicatedVideoMemory    : %zu", adapterDesc.DedicatedVideoMemory);
        LOG_INFO("\t -> DedicatedSystemMemory   : %zu", adapterDesc.DedicatedSystemMemory);
        LOG_INFO("\t -> SharedSystemMemory      : %zu", adapterDesc.SharedSystemMemory);
        return best_adapter;
    }
}

static void dx12_internal_cache_features(context_backend* backend)
{
    ID3D12Device10*    device = backend->device;
    D3D12FeatureCache* f      = &backend->features;

    ID3D12Device10_CheckFeatureSupport(device, D3D12_FEATURE_D3D12_OPTIONS, &f->options, sizeof(f->options));
    ID3D12Device10_CheckFeatureSupport(device, D3D12_FEATURE_D3D12_OPTIONS1, &f->options1, sizeof(f->options1));
    ID3D12Device10_CheckFeatureSupport(device, D3D12_FEATURE_D3D12_OPTIONS5, &f->options5, sizeof(f->options5));
    ID3D12Device10_CheckFeatureSupport(device, D3D12_FEATURE_D3D12_OPTIONS10, &f->options10, sizeof(f->options10));
    ID3D12Device10_CheckFeatureSupport(device, D3D12_FEATURE_D3D12_OPTIONS12, &f->options12, sizeof(f->options12));

    f->architecture.NodeIndex = 0;
    ID3D12Device10_CheckFeatureSupport(device, D3D12_FEATURE_ARCHITECTURE1, &f->architecture, sizeof(f->architecture));
    f->isUMA              = f->architecture.UMA;
    f->isCacheCoherentUMA = f->architecture.CacheCoherentUMA;

    f->shaderModel.HighestShaderModel = D3D_SHADER_MODEL_6_7;
    if (FAILED(ID3D12Device10_CheckFeatureSupport(device, D3D12_FEATURE_SHADER_MODEL, &f->shaderModel, sizeof(f->shaderModel)))) {
        f->shaderModel.HighestShaderModel = D3D_SHADER_MODEL_6_0;
    }

    f->rootSig.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;
    if (FAILED(ID3D12Device10_CheckFeatureSupport(device, D3D12_FEATURE_ROOT_SIGNATURE, &f->rootSig, sizeof(f->rootSig)))) {
        f->rootSig.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
    }

    ID3D12Device10_CheckFeatureSupport(device, D3D12_FEATURE_GPU_VIRTUAL_ADDRESS_SUPPORT, &f->vaSupport, sizeof(f->vaSupport));

    f->nodeCount = ID3D12Device10_GetNodeCount(device);
}

static void dx12_internal_print_features(const D3D12FeatureCache* f)
{
    LOG_INFO("============ D3D12 Device Feature Info ============");

    LOG_INFO("Shader Model          : %u.%u",
        (f->shaderModel.HighestShaderModel >> 4) & 0xF,
        f->shaderModel.HighestShaderModel & 0xF);

    LOG_INFO("Root Signature        : v1.%d",
        f->rootSig.HighestVersion == D3D_ROOT_SIGNATURE_VERSION_1_1 ? 1 : 0);

    LOG_INFO("GPU Nodes             : %u", f->nodeCount);
    LOG_INFO("UMA                   : %s", f->isUMA ? "Yes" : "No");
    LOG_INFO("Cache-Coherent UMA    : %s", f->isCacheCoherentUMA ? "Yes" : "No");

    // D3D12_OPTIONS
    LOG_INFO("Conservative Raster   : %s", f->options.ConservativeRasterizationTier != D3D12_CONSERVATIVE_RASTERIZATION_TIER_NOT_SUPPORTED ? "Yes" : "No");
    LOG_INFO("Standard Swizzle      : %s", f->options.StandardSwizzle64KBSupported ? "Yes" : "No");
    LOG_INFO("Typed UAV Load ADDR64 : %s", f->options.TypedUAVLoadAdditionalFormats ? "Yes" : "No");

    // D3D12_OPTIONS1
    LOG_INFO("Wave Ops              : %s", f->options1.WaveOps ? "Yes" : "No");
    LOG_INFO("Wave Lane Count       : Min %u / Max %u", f->options1.WaveLaneCountMin, f->options1.WaveLaneCountMax);
    LOG_INFO("Int64 Shader Ops      : %s", f->options1.Int64ShaderOps ? "Yes" : "No");

    // D3D12_OPTIONS5
    LOG_INFO("Raytracing Tier       : Tier %d", f->options5.RaytracingTier);

    // D3D12_OPTIONS12
    LOG_INFO("Enhanced Barriers     : %s", f->options12.EnhancedBarriersSupported ? "Yes" : "No");

    // VA support
    LOG_INFO("VA 64-bit Support     : %s", f->vaSupport.MaxGPUVirtualAddressBitsPerResource >= 44 ? "Yes" : "Partial/No");

    LOG_INFO("===================================================");
}

    #ifdef _DEBUG
static void dx12_internal_register_debug_interface(context_backend* backend)
{
    if (SUCCEEDED(D3D12GetDebugInterface(&IID_ID3D12Debug3, (void**) &backend->d3d12_debug))) {
        ID3D12Debug3_EnableDebugLayer(backend->d3d12_debug);
        ID3D12Debug3_SetEnableGPUBasedValidation(backend->d3d12_debug, TRUE);
        LOG_INFO("D3D12 debug layer and GPU-based validation enabled");
    } else {
        LOG_WARN("D3D12 debug interface not available. Debug layer not enabled.");
    }
}

static void dx12_internal_d3d12_register_info_queue(context_backend* backend)
{
    if (!backend->device) {
        LOG_ERROR("[D3D12] D3D12 device is NULL; can't register info queue.");
        return;
    }

    // This increases the device refcount.
    if (SUCCEEDED(ID3D12InfoQueue_QueryInterface(backend->device, &IID_ID3D12InfoQueue, (void**) &backend->d3d12_info_queue))) {
        ID3D12InfoQueue* q = backend->d3d12_info_queue;

        ID3D12InfoQueue_SetBreakOnSeverity(q, D3D12_MESSAGE_SEVERITY_ERROR, TRUE);
        ID3D12InfoQueue_SetBreakOnSeverity(q, D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE);

        LOG_INFO("D3D12 info queue registered and debug message filters installed");
    } else {
        LOG_WARN("Failed to query ID3D12InfoQueue interface. Validation messages will not be captured.");
    }
}

static void dx12_internal_dxgi_register_info_queue(context_backend* backend)
{
    if (SUCCEEDED(DXGIGetDebugInterface1(0, &IID_IDXGIInfoQueue, (void**) &backend->dxgi_info_queue))) {
        IDXGIInfoQueue* q = backend->dxgi_info_queue;

        IDXGIInfoQueue_SetBreakOnSeverity(q, DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_ERROR, TRUE);
        IDXGIInfoQueue_SetBreakOnSeverity(q, DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_CORRUPTION, TRUE);

        LOG_INFO("DXGI debug info queue registered");
    } else {
        LOG_WARN("DXGI info queue not available. No message filtering.");
    }
}

static void dx12_internal_track_dxgi_liveobjects(context_backend* backend)
{
    LOG_WARN("Tracking live DXGI objects. This will report all live objects at the end of the program.");
    if (SUCCEEDED(DXGIGetDebugInterface1(0, &IID_IDXGIDebug, (void**) &backend->dxgi_debug))) {
        IDXGIDebug_ReportLiveObjects(
            backend->dxgi_debug,
            DXGI_DEBUG_ALL,
            (DXGI_DEBUG_RLO_FLAGS) (DXGI_DEBUG_RLO_DETAIL | DXGI_DEBUG_RLO_SUMMARY | DXGI_DEBUG_RLO_IGNORE_INTERNAL));
        LOG_INFO("DXGI live object report completed");
        IDXGIDebug_Release(backend->dxgi_debug);
    }
}

static void dx12_internal_destroy_debug_handles(context_backend* backend)
{
    if (backend->d3d12_debug)
        ID3D12Debug3_Release(backend->d3d12_debug);

    if (backend->d3d12_info_queue)
        ID3D12InfoQueue_Release(backend->d3d12_info_queue);

    if (backend->dxgi_info_queue)
        IDXGIInfoQueue_Release(backend->dxgi_info_queue);
}
    #endif

static gfx_sync_point dx12_internal_signal(gfx_context* context)
{
    gfx_syncobj*   timeline_syncobj = &context->frame_sync.timeline_syncobj;
    gfx_sync_point signal_syncpoint = ++(context->frame_sync.global_syncpoint);
    #if ENABLE_SYNC_LOGGING
    LOG_WARN("[SIGNAL] signaling new global syncpoint: %llu | global_syncpoint: %llu", signal_syncpoint, context->frame_sync.global_syncpoint);
    #endif
    HRESULT hr = ID3D12CommandQueue_Signal(((context_backend*) context->backend)->direct_queue, ((syncobj_backend*) (timeline_syncobj->backend))->fence, signal_syncpoint);
    if (FAILED(hr)) {
        LOG_ERROR("[D3D12] failed to signal syncpoint: %llu  (HRESULT = 0x%08X)", signal_syncpoint, (unsigned int) hr);
        return signal_syncpoint;
    }

    return signal_syncpoint;
}

static void dx12_internal_destroy_res_view(gfx_resource_view* view)
{
    uuid_destroy(&view->uuid);
    if (view->backend) {
        free(view->backend);
        view->backend = NULL;
        view          = NULL;
    }
}

//--------------------------------------------------------

gfx_context dx12_ctx_init(GLFWwindow* window)
{
    gfx_context ctx = {0};
    uuid_generate(&ctx.uuid);

    memset(&s_DXCtx, 0, sizeof(context_backend));
    ctx.backend = &s_DXCtx;
    // For readability, we cast ctx.backend to context_backend*
    context_backend* backend = (context_backend*) ctx.backend;
    backend->glfwWindow      = window;
    backend->hwnd            = (HWND) glfwGetWin32Window(window);

    LOG_INFO("Creating DXGI Factory7");
    UINT createFactoryFlags = 0;
    #if defined(_DEBUG)
    createFactoryFlags = DXGI_CREATE_FACTORY_DEBUG;
    #endif
    HRESULT hr = CreateDXGIFactory2(createFactoryFlags, &IID_IDXGIFactory7, &backend->factory);
    if (FAILED(hr)) {
        LOG_ERROR("[D3D12] Failed to create DXGI Factory7 (HRESULT = 0x%08X)", (unsigned int) hr);
        uuid_destroy(&ctx.uuid);
        free(backend);
        return ctx;
    }

    backend->feat_level = D3D_FEATURE_LEVEL_12_0;

    backend->gpu = dx12_internal_select_best_adapter(backend->factory, backend->feat_level);

    // Now create the logical device from the Adapter
    if (!backend->gpu) {
        LOG_ERROR("[D3D12] Failed to select a suitable GPU for D3D12");
        uuid_destroy(&ctx.uuid);
        free(backend);
        return ctx;
    }

    // Enable DX12 debug layer if available
    #ifdef _DEBUG
    dx12_internal_register_debug_interface(backend);
    #endif

    LOG_INFO("Creating D3D12 Device...");
    hr = D3D12CreateDevice((IUnknown*) backend->gpu, backend->feat_level, &IID_ID3D12Device10, &backend->device);
    if (FAILED(hr)) {
        LOG_ERROR("[D3D12] Failed to create D3D12 Device (HRESULT = 0x%08X)", (unsigned int) hr);
        IDXGIAdapter4_Release(backend->gpu);
        uuid_destroy(&ctx.uuid);
        free(backend);
        return ctx;
    }
    LOG_SUCCESS("Created D3D12 Device!");

    #ifdef _DEBUG
    dx12_internal_d3d12_register_info_queue(backend);
    dx12_internal_dxgi_register_info_queue(backend);
    #endif

    dx12_internal_cache_features(backend);
    dx12_internal_print_features(&backend->features);

    // Create global command queues
    D3D12_COMMAND_QUEUE_DESC desc = {0};
    desc.Type                     = D3D12_COMMAND_LIST_TYPE_DIRECT;
    desc.Priority                 = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
    desc.Flags                    = D3D12_COMMAND_QUEUE_FLAG_NONE;
    desc.NodeMask                 = 0;
    ID3D12Device10_CreateCommandQueue(DXDevice, &desc, &IID_ID3D12CommandQueue, &backend->direct_queue);
    if (!backend->direct_queue) {
        LOG_ERROR("[D3D12] Failed to create D3D12 Direct Command Queue");
        dx12_ctx_destroy(&ctx);
        return ctx;
    }

    // Create frame sync primitives
    ctx.frame_sync.timeline_syncobj = dx12_create_syncobj(GFX_SYNCOBJ_TYPE_TIMELINE);

    // Cache CommandListSubmission commands

    return ctx;
}

void dx12_ctx_destroy(gfx_context* ctx)
{
    if (!ctx || !ctx->backend) {
        LOG_ERROR("[D3D12] Context or backend is NULL, cannot destroy D3D12 context");
        return;
    }

    dx12_destroy_syncobj(&ctx->frame_sync.timeline_syncobj);

    context_backend* backend = (context_backend*) ctx->backend;

    if (backend->direct_queue) {
        ID3D12CommandQueue_Release(backend->direct_queue);
        backend->direct_queue = NULL;
    }

    if (backend->device) {
        ID3D12Device10_Release(backend->device);
        backend->device = NULL;
    }

    if (backend->gpu) {
        IDXGIAdapter4_Release(backend->gpu);
        backend->gpu = NULL;
    }

    if (backend->factory) {
        IDXGIFactory7_Release(backend->factory);
        backend->factory = NULL;
    }

    #ifdef _DEBUG
    dx12_internal_destroy_debug_handles(backend);
    dx12_internal_track_dxgi_liveobjects(backend);
    #endif

    uuid_destroy(&ctx->uuid);
}

void dx12_flush_gpu_work(gfx_context* context)
{
    UNUSED(context);
    gfx_syncobj* timeline_syncobj = &context->frame_sync.timeline_syncobj;

    uint64_t signal_value = ++(context->frame_sync.global_syncpoint);
    #if ENABLE_SYNC_LOGGING
    LOG_WARN("[GPU Flush] [SIGNAL] signaling new global syncpoint: %llu", context->frame_sync.global_syncpoint);
    #endif
    ID3D12CommandQueue_Signal(((context_backend*) context->backend)->direct_queue, ((syncobj_backend*) (timeline_syncobj->backend))->fence, signal_value);

    dx12_wait_on_previous_cmds(timeline_syncobj, context->frame_sync.global_syncpoint);
    context->frame_sync.frame_syncpoint[context->swapchain.current_backbuffer_idx] = signal_value;
    #if ENABLE_SYNC_LOGGING
    LOG_WARN("[GPU Flush] [Update Wait] updating frame_idx: %d | new_sync_point: %llu", context->swapchain.current_backbuffer_idx, signal_value);
    #endif

    for (int i = 0; i < MAX_DX_SWAPCHAIN_BUFFERS; ++i) {
        context->frame_sync.frame_syncpoint[i] = context->frame_sync.global_syncpoint;
    }
}

static void dx12_internal_update_swapchain_rtv(gfx_swapchain* sc)
{
    swapchain_backend* backend = (swapchain_backend*) sc->backend;

    backend->image_count = MAX_DX_SWAPCHAIN_BUFFERS;
    ID3D12DescriptorHeap_GetCPUDescriptorHandleForHeapStart(backend->rtv_heap, &backend->rtv_handle_start);

    for (uint32_t i = 0; i < MAX_DX_SWAPCHAIN_BUFFERS; i++) {
        HRESULT hr = IDXGISwapChain4_GetBuffer(backend->swapchain, i, &IID_ID3D12Resource, &backend->backbuffers[i]);
        if (FAILED(hr)) {
            LOG_ERROR("[D3D12] Failed to get backbuffer %u from swapchain (HRESULT = 0x%08X)", i, (unsigned int) hr);
            for (uint32_t j = 0; j < i; j++) {
                if (backend->backbuffers[j]) {
                    ID3D12Resource_Release(backend->backbuffers[j]);
                }
            }
            ID3D12DescriptorHeap_Release(backend->rtv_heap);
            IDXGISwapChain4_Release(backend->swapchain);
            uuid_destroy(&sc->uuid);
            free(backend);
            return;
        }

        // Create RTV for the back buffer
        D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = backend->rtv_handle_start;
        rtvHandle.ptr += i * ID3D12Device10_GetDescriptorHandleIncrementSize(DXDevice, D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
        ID3D12Device10_CreateRenderTargetView(DXDevice, backend->backbuffers[i], NULL, rtvHandle);
    }
}

static void dx12_internal_create_backbuffers(gfx_swapchain* sc)
{
    swapchain_backend* backend = (swapchain_backend*) sc->backend;

    // Create RTV heap and extract backbuffers
    D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {0};
    rtvHeapDesc.Type                       = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    rtvHeapDesc.NumDescriptors             = MAX_DX_SWAPCHAIN_BUFFERS;
    rtvHeapDesc.Flags                      = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    rtvHeapDesc.NodeMask                   = 0;
    HRESULT hr                             = ID3D12Device10_CreateDescriptorHeap(DXDevice, &rtvHeapDesc, &IID_ID3D12DescriptorHeap, &backend->rtv_heap);
    if (FAILED(hr)) {
        LOG_ERROR("[D3D12] Failed to create RTV Descriptor Heap (HRESULT = 0x%08X)", (unsigned int) hr);
        IDXGISwapChain4_Release(backend->swapchain);
        uuid_destroy(&sc->uuid);
        free(backend);
        return;
    }

    dx12_internal_update_swapchain_rtv(sc);
}

static void dx12_internal_destroy_backbuffers(gfx_swapchain* sc)
{
    swapchain_backend* backend = (swapchain_backend*) sc->backend;

    for (uint32_t i = 0; i < backend->image_count; i++) {
        if (backend->backbuffers[i]) {
            ID3D12Resource_Release(backend->backbuffers[i]);
            backend->backbuffers[i] = NULL;
        }
    }

    if (backend->rtv_heap) {
        ID3D12DescriptorHeap_Release(backend->rtv_heap);
        backend->rtv_heap = NULL;
    }

    backend->image_count = 0;
}

gfx_swapchain dx12_create_swapchain(uint32_t width, uint32_t height)
{
    gfx_swapchain swapchain = {0};
    uuid_generate(&swapchain.uuid);

    swapchain.width  = width;
    swapchain.height = height;

    swapchain_backend* backend = malloc(sizeof(swapchain_backend));
    if (!backend) {
        LOG_ERROR("[D3D12] error malloc swapchain_backend");
        uuid_destroy(&swapchain.uuid);
        return swapchain;
    }
    memset(backend, 0, sizeof(swapchain_backend));
    swapchain.backend = backend;

    DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {0};
    swapChainDesc.Width                 = width;
    swapChainDesc.Height                = height;
    swapChainDesc.Format                = DX_SWAPCHAIN_FORMAT;
    swapChainDesc.Stereo                = FALSE;
    swapChainDesc.SampleDesc            = (DXGI_SAMPLE_DESC){1, 0};
    swapChainDesc.BufferUsage           = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.BufferCount           = MAX_DX_SWAPCHAIN_BUFFERS;
    swapChainDesc.Scaling               = DXGI_SCALING_STRETCH;
    swapChainDesc.SwapEffect            = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swapChainDesc.AlphaMode             = DXGI_ALPHA_MODE_UNSPECIFIED;
    // It is recommended to always allow tearing if tearing support is available.
    swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;

    IDXGISwapChain1* swapchain1 = NULL;
    HRESULT          hr         = IDXGIFactory7_CreateSwapChainForHwnd(s_DXCtx.factory, (IUnknown*) s_DXCtx.direct_queue, s_DXCtx.hwnd, &swapChainDesc, NULL, NULL, &swapchain1);
    if (FAILED(hr)) {
        LOG_ERROR("[D3D12] Failed to create DXGI Swapchain (HRESULT = 0x%08X)", (unsigned int) hr);
        uuid_destroy(&swapchain.uuid);
        free(backend);
        return swapchain;
    }
    IDXGISwapChain1_QueryInterface(swapchain1, &IID_IDXGISwapChain4, &backend->swapchain);
    if (!backend->swapchain) {
        LOG_ERROR("[D3D12] Failed to query IDXGISwapChain4 from IDXGISwapChain1");
        IDXGISwapChain1_Release(swapchain1);
        uuid_destroy(&swapchain.uuid);
        free(backend);
        return swapchain;
    }
    IDXGISwapChain1_Release(swapchain1);    // Release IDXGISwapChain1, we only need IDXGISwapChain4 now

    dx12_internal_create_backbuffers(&swapchain);

    return swapchain;
}

void dx12_destroy_swapchain(gfx_swapchain* sc)
{
    if (!uuid_is_null(&sc->uuid)) {
        uuid_destroy(&sc->uuid);
        swapchain_backend* backend = sc->backend;

        dx12_internal_destroy_backbuffers(sc);

        if (backend->swapchain) {
            IDXGISwapChain4_Release(backend->swapchain);
            backend->swapchain = NULL;
            free(backend);
            sc->backend = NULL;
        }
    }
}

gfx_syncobj dx12_create_syncobj(gfx_syncobj_type type)
{
    gfx_syncobj syncobj = {0};
    uuid_generate(&syncobj.uuid);

    syncobj_backend* backend = malloc(sizeof(syncobj_backend));
    syncobj.type             = type;
    syncobj.backend          = backend;

    HRESULT hr = ID3D12Device10_CreateFence(DXDevice, 0, D3D12_FENCE_FLAG_NONE, &IID_ID3D12Fence, &backend->fence);
    if (FAILED(hr)) {
        LOG_ERROR("[D3D12] Failed to create ID3D12Fence (HRESULT = 0x%08X)", (unsigned int) hr);
        uuid_destroy(&syncobj.uuid);
        free(backend);
        return syncobj;
    }

    if (type == GFX_SYNCOBJ_TYPE_CPU || GFX_SYNCOBJ_TYPE_TIMELINE) {
        // create a CPU event to wait on
        backend->fenceEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
        if (backend->fenceEvent == NULL) {
            DWORD err = GetLastError();
            LOG_ERROR("CreateEvent failed -> 0x%08X", err);
        }
    }

    return syncobj;
}

void dx12_destroy_syncobj(gfx_syncobj* syncobj)
{
    if (!uuid_is_null(&syncobj->uuid)) {
        uuid_destroy(&syncobj->uuid);
        syncobj_backend* backend = syncobj->backend;

        if (backend->fence) {
            ID3D12Fence_Release(backend->fence);
            backend->fence = NULL;

            if (syncobj->type == GFX_SYNCOBJ_TYPE_CPU || GFX_SYNCOBJ_TYPE_TIMELINE)
                CloseHandle(backend->fenceEvent);

            free(backend);
            syncobj->backend = NULL;
        }
    }
}

gfx_cmd_pool dx12_create_gfx_cmd_allocator(void)
{
    gfx_cmd_pool pool = {0};
    uuid_generate(&pool.uuid);

    pool.backend = malloc(sizeof(ID3D12CommandAllocator));

    HRESULT hr = ID3D12Device10_CreateCommandAllocator(DXDevice, D3D12_COMMAND_LIST_TYPE_DIRECT, &IID_ID3D12CommandAllocator, &pool.backend);
    if (FAILED(hr)) {
        LOG_ERROR("[D3D12] Failed to allocate command allocator (HRESULT = 0x%08X)", (unsigned int) hr);
        uuid_destroy(&pool.uuid);
        free(pool.backend);
        return pool;
    }

    return pool;
}

void dx12_destroy_gfx_cmd_allocator(gfx_cmd_pool* pool)
{
    if (!uuid_is_null(&pool->uuid)) {
        uuid_destroy(&pool->uuid);
        if (pool->backend) {
            ID3D12CommandAllocator_Release((ID3D12CommandAllocator*) pool->backend);
            pool->backend = NULL;
        }
    }
}

gfx_cmd_buf dx12_create_gfx_cmd_buf(gfx_cmd_pool* pool)
{
    gfx_cmd_buf cmd_buf = {0};
    uuid_generate(&cmd_buf.uuid);
    cmd_buf.backend = malloc(sizeof(ID3D12GraphicsCommandList));

    HRESULT hr = ID3D12Device10_CreateCommandList(DXDevice, 0, D3D12_COMMAND_LIST_TYPE_DIRECT, ((ID3D12CommandAllocator*) (pool->backend)), NULL, &IID_ID3D12GraphicsCommandList, &cmd_buf.backend);
    hr         = ID3D12GraphicsCommandList_Close((ID3D12GraphicsCommandList*) (cmd_buf.backend));
    if (FAILED(hr)) {
        LOG_ERROR("[D3D12] Failed to allocate command lists (HRESULT = 0x%08X)", (unsigned int) hr);
        uuid_destroy(&cmd_buf.uuid);
        free(cmd_buf.backend);
        return cmd_buf;
    }

    return cmd_buf;
}

void dx12_free_gfx_cmd_buf(gfx_cmd_buf* cmd_buf)
{
    ID3D12GraphicsCommandList_Release((ID3D12GraphicsCommandList*) (cmd_buf->backend));
    uuid_destroy(&cmd_buf->uuid);
}

gfx_shader dx12_create_compute_shader(const char* cso_file_path)
{
    gfx_shader shader = {0};
    uuid_generate(&shader.uuid);

    char*           full_path = appendFileExt(cso_file_path, SHADER_BINARY_EXT);
    shader_backend* backend   = malloc(sizeof(shader_backend));
    if (backend) {
        shader.stages.CS = backend;
        backend->stage   = GFX_SHADER_STAGE_CS;

        int      len       = MultiByteToWideChar(CP_UTF8, 0, full_path, -1, NULL, 0);
        wchar_t* wide_path = malloc(len * sizeof(wchar_t));
        MultiByteToWideChar(CP_UTF8, 0, full_path, -1, wide_path, len);

        HRESULT hr = D3DReadFileToBlob((LPCWSTR) wide_path, &backend->bytecode);
        if (FAILED(hr)) {
            LOG_ERROR("Failed to load shader blob from file: %s (HRESULT = 0x%08X)", full_path, hr);
            uuid_destroy(&shader.uuid);
            return shader;
        }
        free(wide_path);
    }
    free(full_path);
    return shader;
}

void dx12_destroy_compute_shader(gfx_shader* shader)
{
    uuid_destroy(&shader->uuid);
    if (shader->stages.CS) {
        shader_backend* backend = (shader_backend*) (shader->stages.CS);

        ID3D10Blob_Release((ID3DBlob*) (backend)->bytecode);
        (ID3DBlob*) (backend)->bytecode = NULL;

        free(backend);
        shader->stages.CS = NULL;
    }
}

gfx_shader dx12_create_vs_ps_shader(const char* cso_file_path_vs, const char* cso_file_path_ps)
{
    gfx_shader shader = {0};
    uuid_generate(&shader.uuid);

    char*           full_path_vs = appendFileExt(cso_file_path_vs, SHADER_BINARY_EXT);
    shader_backend* backend_vs   = malloc(sizeof(shader_backend));
    if (backend_vs) {
        shader.stages.VS  = backend_vs;
        backend_vs->stage = GFX_SHADER_STAGE_CS;

        int      len       = MultiByteToWideChar(CP_UTF8, 0, full_path_vs, -1, NULL, 0);
        wchar_t* wide_path = malloc(len * sizeof(wchar_t));
        MultiByteToWideChar(CP_UTF8, 0, full_path_vs, -1, wide_path, len);

        HRESULT hr = D3DReadFileToBlob((LPCWSTR) wide_path, &backend_vs->bytecode);
        if (FAILED(hr)) {
            LOG_ERROR("Failed to load shader blob from file: %s (HRESULT = 0x%08X)", full_path_vs, hr);
            uuid_destroy(&shader.uuid);
            return shader;
        }
        free(wide_path);
    }
    free(full_path_vs);

    char*           full_path_ps = appendFileExt(cso_file_path_ps, SHADER_BINARY_EXT);
    shader_backend* backend_ps   = malloc(sizeof(shader_backend));
    if (backend_ps) {
        shader.stages.PS  = backend_ps;
        backend_ps->stage = GFX_SHADER_STAGE_PS;

        int      len       = MultiByteToWideChar(CP_UTF8, 0, full_path_ps, -1, NULL, 0);
        wchar_t* wide_path = malloc(len * sizeof(wchar_t));
        MultiByteToWideChar(CP_UTF8, 0, full_path_ps, -1, wide_path, len);

        HRESULT hr = D3DReadFileToBlob((LPCWSTR) wide_path, &backend_ps->bytecode);
        if (FAILED(hr)) {
            LOG_ERROR("Failed to load shader blob from file: %s (HRESULT = 0x%08X)", full_path_ps, hr);
            uuid_destroy(&shader.uuid);
            return shader;
        }
        free(wide_path);
    }
    free(full_path_ps);

    return shader;
}

void dx12_destroy_vs_ps_shader(gfx_shader* shader)
{
    uuid_destroy(&shader->uuid);

    if (shader->stages.VS) {
        shader_backend* backend = (shader_backend*) (shader->stages.VS);

        ID3D10Blob_Release((ID3DBlob*) (backend)->bytecode);
        (ID3DBlob*) (backend)->bytecode = NULL;

        free(backend);
        shader->stages.VS = NULL;
    }

    if (shader->stages.PS) {
        shader_backend* backend = (shader_backend*) (shader->stages.PS);

        ID3D10Blob_Release((ID3DBlob*) (backend)->bytecode);
        (ID3DBlob*) (backend)->bytecode = NULL;

        free(backend);
        shader->stages.PS = NULL;
    }
}

gfx_root_signature dx12_create_root_signature(const gfx_descriptor_set_layout* set_layouts, uint32_t set_layout_count, const gfx_push_constant_range* push_constants, uint32_t push_constant_count)
{
    gfx_root_signature root_sig = {0};
    uuid_generate(&root_sig.uuid);

    UNUSED(set_layouts);
    UNUSED(set_layout_count);
    UNUSED(push_constants);
    UNUSED(push_constant_count);

    D3D12_ROOT_SIGNATURE_DESC desc                         = {0};
    D3D12_ROOT_PARAMETER      root_params[MAX_ROOT_PARAMS] = {0};

    desc.NumParameters = set_layout_count + push_constant_count;
    desc.pParameters   = root_params;

    // Each set is a table entry
    for (uint32_t i = 0; i < set_layout_count; ++i) {
        const gfx_descriptor_set_layout* set_layout = &set_layouts[i];

        D3D12_ROOT_DESCRIPTOR_TABLE table_entry                   = {0};
        D3D12_DESCRIPTOR_RANGE      ranges[MAX_DESCRIPTOR_RANGES] = {0};

        table_entry.NumDescriptorRanges = set_layout->binding_count;

        for (uint32_t j = 0; j < table_entry.NumDescriptorRanges; ++j) {
            const gfx_descriptor_binding binding = set_layout->bindings[j];
            // Add a range into descriptor table
            ranges[j].RangeType                         = dx12_util_descriptor_range_type_translate(binding.type);
            ranges[j].NumDescriptors                    = binding.count;
            ranges[j].BaseShaderRegister                = binding.location.binding;
            ranges[j].RegisterSpace                     = binding.location.set;
            ranges[j].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
            ranges[j].OffsetInDescriptorsFromTableStart = i;
        }

        root_params[i].ParameterType   = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        table_entry.pDescriptorRanges  = ranges;
        root_params[i].DescriptorTable = table_entry;
        // TODO: Implement shader visibility for each table in a more granular fashion
        root_params[i].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;    // Currently ALL!
    }

    for (uint32_t i = 0; i < push_constant_count; ++i) {
        const gfx_push_constant_range pc    = push_constants[i];
        D3D12_ROOT_PARAMETER*         param = &root_params[set_layout_count + i];

        param->ParameterType            = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
        param->Constants.Num32BitValues = pc.size / SIZE_DWORD;
        param->Constants.ShaderRegister = set_layout_count + i;    // or we can (pc.offset / 4) encode offset in register this way
        param->Constants.RegisterSpace  = 0;
        param->ShaderVisibility         = D3D12_SHADER_VISIBILITY_ALL;    // Currently ALL!
    }

    // Serialize root signature
    ID3DBlob* signature_blob = NULL;
    ID3DBlob* error_blob     = NULL;

    HRESULT hr = D3D12SerializeRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION_1, &signature_blob, &error_blob);
    if (FAILED(hr)) {
        LOG_ERROR("Failed to serialize root signature (HRESULT = 0x%08X)", hr);
        if (error_blob) {
            const char* msg = ID3D10Blob_GetBufferPointer(error_blob);
            LOG_ERROR("Root Signature Error: %s", msg);
        }
        uuid_destroy(&root_sig.uuid);
    }

    ID3D12RootSignature* d3d_root_sig = NULL;
    hr                                = ID3D12Device10_CreateRootSignature(DXDevice, 0, ID3D10Blob_GetBufferPointer(signature_blob), ID3D10Blob_GetBufferSize(signature_blob), &IID_ID3D12RootSignature, &d3d_root_sig);
    if (FAILED(hr)) {
        LOG_ERROR("Failed to serializwe root signature (HRESULT = 0x%08X)", hr);
        uuid_destroy(&root_sig.uuid);
        ID3D10Blob_Release(signature_blob);
        ID3D10Blob_Release(error_blob);
    }

    if (signature_blob)
        ID3D10Blob_Release(signature_blob);
    if (error_blob)
        ID3D10Blob_Release(error_blob);

    root_sig.backend                 = d3d_root_sig;
    root_sig.descriptor_set_layouts  = (gfx_descriptor_set_layout*) set_layouts;
    root_sig.descriptor_layout_count = set_layout_count;
    root_sig.push_constants          = (gfx_push_constant_range*) push_constants;
    root_sig.push_constant_count     = push_constant_count;
    return root_sig;
}

void dx12_destroy_root_signature(gfx_root_signature* root_sig)
{
    uuid_destroy(&root_sig->uuid);

    if (root_sig->backend) {
        ID3D12RootSignature_Release((ID3D12RootSignature*) (root_sig->backend));
        root_sig->backend = NULL;
    }
}

static gfx_pipeline dx12_internal_create_gfx_pipeline(gfx_pipeline_create_info info)
{
    gfx_pipeline pipeline = {0};
    uuid_generate(&pipeline.uuid);

    pipeline_backend* backend = malloc(sizeof(pipeline_backend));
    pipeline.backend          = backend;
    if (!backend) {
        LOG_ERROR("[D3D12] Failed to malloc pipeline_backend");
        uuid_destroy(&pipeline.uuid);
        return pipeline;
    }

    D3D12_GRAPHICS_PIPELINE_STATE_DESC desc = {0};
    desc.NodeMask                           = 0;
    desc.Flags                              = D3D12_PIPELINE_STATE_FLAG_NONE;
    desc.pRootSignature                     = (ID3D12RootSignature*) (info.root_sig.backend);

    // Shaders
    D3D12_SHADER_BYTECODE VSByteCode = {0};
    VSByteCode.BytecodeLength        = ID3D10Blob_GetBufferSize(((shader_backend*) (info.shader.stages.VS))->bytecode);
    VSByteCode.pShaderBytecode       = ID3D10Blob_GetBufferPointer(((shader_backend*) (info.shader.stages.VS))->bytecode);
    desc.VS                          = VSByteCode;

    D3D12_SHADER_BYTECODE PSByteCode = {0};
    PSByteCode.BytecodeLength        = ID3D10Blob_GetBufferSize(((shader_backend*) (info.shader.stages.PS))->bytecode);
    PSByteCode.pShaderBytecode       = ID3D10Blob_GetBufferPointer(((shader_backend*) (info.shader.stages.PS))->bytecode);
    desc.PS                          = PSByteCode;

    //----------------------------
    // Input Assembly Stage
    //----------------------------
    // TODO: Fill this properly
    D3D12_INPUT_LAYOUT_DESC input_layout = {0};
    input_layout.NumElements             = 0;
    input_layout.pInputElementDescs      = NULL;
    desc.InputLayout                     = input_layout;

    // Set via IA to support dynamic state
    D3D12_PRIMITIVE_TOPOLOGY_TYPE prim_topology = {0};
    prim_topology                               = dx12_util_draw_type_prim_topology_type_translate(info.draw_type);
    desc.PrimitiveTopologyType                  = prim_topology;
    // Just translate and cache it
    backend->topology = dx12_util_draw_type_translate(info.draw_type);

    desc.NumRenderTargets = info.color_formats_count;
    for (uint32_t i = 0; i < info.color_formats_count; i++) {
        desc.RTVFormats[i] = dx12_util_format_translate(info.color_formats[i]);
    }
    desc.DSVFormat = dx12_util_format_translate(info.depth_format);

    //----------------------------
    // Rasterizer Stage
    //----------------------------
    D3D12_RASTERIZER_DESC rasterizer = {0};
    rasterizer.FrontCounterClockwise = TRUE;
    rasterizer.DepthClipEnable       = FALSE;
    rasterizer.CullMode              = dx12_util_cull_mode_translate(info.cull_mode);
    rasterizer.FillMode              = dx12_util_polygon_mode_translate(info.polygon_mode);
    rasterizer.MultisampleEnable     = FALSE;
    desc.RasterizerState             = rasterizer;

    //----------------------------
    // Color Blend State
    //----------------------------
    D3D12_BLEND_DESC blend_state       = {0};
    blend_state.AlphaToCoverageEnable  = FALSE;
    blend_state.IndependentBlendEnable = FALSE;

    D3D12_RENDER_TARGET_BLEND_DESC rt_blend_desc = {
        .BlendEnable           = info.enable_transparency,
        .SrcBlend              = dx12_util_blend_factor_translate(info.src_color_blend_factor),
        .DestBlend             = dx12_util_blend_factor_translate(info.dst_color_blend_factor),
        .BlendOp               = dx12_util_blend_op_translate(info.color_blend_op),
        .SrcBlendAlpha         = dx12_util_blend_factor_translate(info.src_alpha_blend_factor),
        .DestBlendAlpha        = dx12_util_blend_factor_translate(info.dst_alpha_blend_factor),
        .BlendOpAlpha          = dx12_util_blend_op_translate(info.alpha_blend_op),
        .RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL,
    };
    blend_state.RenderTarget[0] = rt_blend_desc;
    desc.BlendState             = blend_state;

    //----------------------------
    // Depth Stencil Stage
    //----------------------------
    D3D12_DEPTH_STENCIL_DESC depth = {0};
    depth.DepthEnable              = info.enable_depth_test,
    depth.DepthWriteMask           = info.enable_depth_write ? D3D12_DEPTH_WRITE_MASK_ALL : D3D12_DEPTH_WRITE_MASK_ZERO,
    depth.DepthFunc                = dx12_util_compare_op_translate(info.depth_compare),
    depth.StencilEnable            = FALSE;
    depth.StencilReadMask          = D3D12_DEFAULT_STENCIL_READ_MASK;
    depth.StencilWriteMask         = D3D12_DEFAULT_STENCIL_WRITE_MASK;
    depth.FrontFace                = (D3D12_DEPTH_STENCILOP_DESC){
                       .StencilFailOp      = D3D12_STENCIL_OP_KEEP,
                       .StencilDepthFailOp = D3D12_STENCIL_OP_KEEP,
                       .StencilPassOp      = D3D12_STENCIL_OP_KEEP,
                       .StencilFunc        = D3D12_COMPARISON_FUNC_ALWAYS};
    depth.BackFace         = depth.FrontFace;
    desc.DepthStencilState = depth;

    //----------------------------
    // Multi sample State (MSAA)
    //----------------------------
    desc.SampleDesc.Count   = 1;    // No MSAA
    desc.SampleDesc.Quality = 0;
    desc.SampleMask         = UINT32_MAX;

    //----------------------------
    // Gfx Pipeline
    //----------------------------

    ID3D12PipelineState* pso = NULL;
    HRESULT              hr  = ID3D12Device10_CreateGraphicsPipelineState(DXDevice, &desc, &IID_ID3D12PipelineState, &pso);
    if (FAILED(hr)) {
        LOG_ERROR("[D3D12] Failed to create gfx PSO (HRESULT=0x%08X)", hr);
        uuid_destroy(&pipeline.uuid);
        return pipeline;
    }
    backend->pso = pso;

    return pipeline;
}

static gfx_pipeline dx12_internal_create_compute_pipeline(gfx_pipeline_create_info info)
{
    gfx_pipeline pipeline = {0};
    uuid_generate(&pipeline.uuid);

    D3D12_COMPUTE_PIPELINE_STATE_DESC desc = {0};
    desc.NodeMask                          = 0;
    desc.Flags                             = D3D12_PIPELINE_STATE_FLAG_NONE;
    desc.pRootSignature                    = (ID3D12RootSignature*) (info.root_sig.backend);

    D3D12_SHADER_BYTECODE CSByteCode = {0};
    CSByteCode.BytecodeLength        = ID3D10Blob_GetBufferSize(((shader_backend*) (info.shader.stages.CS))->bytecode);
    CSByteCode.pShaderBytecode       = ID3D10Blob_GetBufferPointer(((shader_backend*) (info.shader.stages.CS))->bytecode);
    desc.CS                          = CSByteCode;

    ID3D12PipelineState* pso = NULL;
    HRESULT              hr  = ID3D12Device10_CreateComputePipelineState(DXDevice, &desc, &IID_ID3D12PipelineState, &pso);
    if (FAILED(hr)) {
        LOG_ERROR("[D3D12] Failed to create compute PSO (HRESULT=0x%08X)", hr);
        uuid_destroy(&pipeline.uuid);
        return pipeline;
    }
    pipeline.backend = pso;

    return pipeline;
}

gfx_pipeline dx12_create_pipeline(gfx_pipeline_create_info info)
{
    if (info.type == GFX_PIPELINE_TYPE_GRAPHICS)
        return dx12_internal_create_gfx_pipeline(info);
    else
        return dx12_internal_create_compute_pipeline(info);
}

void dx12_destroy_pipeline(gfx_pipeline* pipeline)
{
    uuid_destroy(&pipeline->uuid);
    if (pipeline->backend) {
        ID3D12PipelineState_Release((ID3D12PipelineState*) ((pipeline_backend*) (pipeline->backend))->pso);
        free(pipeline->backend);
        pipeline->backend = NULL;
    }
}

gfx_descriptor_table dx12_create_descriptor_table(const gfx_root_signature* root_signature)
{
    // Basically we create a descriptor heaps here
    gfx_descriptor_table descriptor_table = {0};
    uuid_generate(&descriptor_table.uuid);

    descriptor_table_backend* backend = malloc(sizeof(descriptor_table_backend));
    memset(backend, 0, sizeof(descriptor_table_backend));
    descriptor_table.backend = backend;

    uint32_t total_cbv_srv_uav_count = 0;
    uint32_t total_sampler_count     = 0;
    uint32_t total_rtv_count         = 0;
    uint32_t total_dsv_count         = 0;

    // Count descriptors per set and accumulate them, we create heaps with max sets
    // TODO: If this is taking too much time, use global heaps or ring buffer
    for (uint32_t set = 0; set < root_signature->descriptor_layout_count; ++set) {
        const gfx_descriptor_set_layout* set_layout = &root_signature->descriptor_set_layouts[set];

        for (uint32_t i = 0; i < set_layout->binding_count; ++i) {
            const gfx_descriptor_binding* binding = &set_layout->bindings[i];
            uint32_t                      count   = binding->count;

            switch (binding->type) {
                case GFX_RESOURCE_TYPE_UNIFORM_BUFFER:
                case GFX_RESOURCE_TYPE_STORAGE_BUFFER:
                case GFX_RESOURCE_TYPE_SAMPLED_IMAGE:
                case GFX_RESOURCE_TYPE_STORAGE_IMAGE:
                    total_cbv_srv_uav_count += count;
                    break;

                case GFX_RESOURCE_TYPE_SAMPLER:
                    total_sampler_count += count;
                    break;

                case GFX_RESOURCE_TYPE_COLOR_ATTACHMENT:
                    total_rtv_count += count;
                    break;

                case GFX_RESOURCE_TYPE_DEPTH_STENCIL_ATTACHMENT:
                    total_dsv_count += count;
                    break;

                default:
                    break;
            }
        }
    }

    // Now create the actual heaps
    if (total_cbv_srv_uav_count > 0) {
        backend->heap_count++;
        D3D12_DESCRIPTOR_HEAP_DESC desc = {
            .Type           = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
            .NumDescriptors = total_cbv_srv_uav_count,
            .Flags          = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
            .NodeMask       = 0,
        };
        HRESULT hr = ID3D12Device10_CreateDescriptorHeap(DXDevice, &desc, &IID_ID3D12DescriptorHeap, &backend->cbv_uav_srv_heap);
        DX_CHECK_HR(hr);
    }

    if (total_sampler_count > 0) {
        backend->heap_count++;
        D3D12_DESCRIPTOR_HEAP_DESC desc = {
            .Type           = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER,
            .NumDescriptors = total_sampler_count,
            .Flags          = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
            .NodeMask       = 0,
        };
        HRESULT hr = ID3D12Device10_CreateDescriptorHeap(DXDevice, &desc, &IID_ID3D12DescriptorHeap, &backend->sampler_heap);
        DX_CHECK_HR(hr);
    }

    // RTV/DSV heaps usually not shader visible — optional:
    if (total_rtv_count > 0) {
        backend->heap_count++;
        D3D12_DESCRIPTOR_HEAP_DESC desc = {
            .Type           = D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
            .NumDescriptors = total_rtv_count,
            .Flags          = D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
            .NodeMask       = 0,
        };
        HRESULT hr = ID3D12Device10_CreateDescriptorHeap(DXDevice, &desc, &IID_ID3D12DescriptorHeap, &backend->rtv_heap);
        DX_CHECK_HR(hr);
    }

    if (total_dsv_count > 0) {
        backend->heap_count++;
        D3D12_DESCRIPTOR_HEAP_DESC desc = {
            .Type           = D3D12_DESCRIPTOR_HEAP_TYPE_DSV,
            .NumDescriptors = total_dsv_count,
            .Flags          = D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
            .NodeMask       = 0,
        };
        HRESULT hr = ID3D12Device10_CreateDescriptorHeap(DXDevice, &desc, &IID_ID3D12DescriptorHeap, &backend->dsv_heap);
        DX_CHECK_HR(hr);
    }

    descriptor_table.set_count = root_signature->descriptor_layout_count;
    return descriptor_table;
}

void dx12_destroy_descriptor_table(gfx_descriptor_table* descriptor_table)
{
    (void) descriptor_table;
    uuid_destroy(&descriptor_table->uuid);
    descriptor_table_backend* backend = ((descriptor_table_backend*) (descriptor_table->backend));

    if (backend->cbv_uav_srv_heap) {
        ID3D12DescriptorHeap_Release(backend->cbv_uav_srv_heap);
        backend->cbv_uav_srv_heap = NULL;
    }

    if (backend->sampler_heap) {
        ID3D12DescriptorHeap_Release(backend->sampler_heap);
        backend->sampler_heap = NULL;
    }

    if (backend->rtv_heap) {
        ID3D12DescriptorHeap_Release(backend->rtv_heap);
        backend->rtv_heap = NULL;
    }

    if (backend->dsv_heap) {
        ID3D12DescriptorHeap_Release(backend->dsv_heap);
        backend->dsv_heap = NULL;
    }

    free(backend);
    descriptor_table->backend = NULL;
}

void dx12_update_descriptor_table(gfx_descriptor_table* descriptor_table, gfx_descriptor_table_entry* entries, uint32_t num_entries)
{
    descriptor_table_backend* backend = (descriptor_table_backend*) (descriptor_table->backend);

    D3D12_CPU_DESCRIPTOR_HANDLE cbv_srv_base   = {0};
    UINT                        cbv_srv_stride = 0;
    if (backend->cbv_uav_srv_heap) {
        ID3D12DescriptorHeap_GetCPUDescriptorHandleForHeapStart(backend->cbv_uav_srv_heap, &cbv_srv_base);
        cbv_srv_stride = DX12_DESCRIPTOR_SIZE(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    }

    D3D12_CPU_DESCRIPTOR_HANDLE sampler_base   = {0};
    UINT                        sampler_stride = 0;
    if (backend->sampler_heap) {
        ID3D12DescriptorHeap_GetCPUDescriptorHandleForHeapStart(backend->sampler_heap, &sampler_base);
        sampler_stride = DX12_DESCRIPTOR_SIZE(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
    }

    D3D12_CPU_DESCRIPTOR_HANDLE rtv_base   = {0};
    UINT                        rtv_stride = 0;
    if (backend->rtv_heap) {
        ID3D12DescriptorHeap_GetCPUDescriptorHandleForHeapStart(backend->rtv_heap, &rtv_base);
        rtv_stride = DX12_DESCRIPTOR_SIZE(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
    }

    D3D12_CPU_DESCRIPTOR_HANDLE dsv_base   = {0};
    UINT                        dsv_stride = 0;
    if (backend->dsv_heap) {
        ID3D12DescriptorHeap_GetCPUDescriptorHandleForHeapStart(backend->dsv_heap, &dsv_base);
        dsv_stride = DX12_DESCRIPTOR_SIZE(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
    }

    // Note: We don't care about or use UAV counters yet!

    for (uint32_t i = 0; i < num_entries; i++) {
        const gfx_resource*      res      = entries[i].resource;
        const gfx_resource_view* res_view = entries[i].resource_view;

        // Note: maybe we can also combine location.set/binding to find the right offset position in the heap or use flattened out layout
        // gfx_binding_location location = entries[i].location;

        switch (res_view->type) {
            case GFX_RESOURCE_TYPE_UNIFORM_BUFFER: {
                D3D12_CONSTANT_BUFFER_VIEW_DESC cbv_desc = ((resource_view_backend*) (res_view->backend))->cbv_desc;

                D3D12_CPU_DESCRIPTOR_HANDLE dst = cbv_srv_base;
                dst.ptr += i * cbv_srv_stride;
                ID3D12Device10_CreateConstantBufferView(DXDevice, &cbv_desc, dst);
            } break;
            case GFX_RESOURCE_TYPE_STORAGE_BUFFER: {
                D3D12_UNORDERED_ACCESS_VIEW_DESC uav_desc = ((resource_view_backend*) (res_view->backend))->uav_desc;

                D3D12_CPU_DESCRIPTOR_HANDLE dst = cbv_srv_base;
                dst.ptr += i * cbv_srv_stride;
                ID3D12Device10_CreateUnorderedAccessView(DXDevice, (ID3D12Resource*) res->ubo->backend, NULL, &uav_desc, dst);
            } break;
            case GFX_RESOURCE_TYPE_STORAGE_IMAGE: {
                D3D12_UNORDERED_ACCESS_VIEW_DESC uav_desc = ((resource_view_backend*) (res_view->backend))->uav_desc;

                D3D12_CPU_DESCRIPTOR_HANDLE dst = cbv_srv_base;
                dst.ptr += i * cbv_srv_stride;
                ID3D12Device10_CreateUnorderedAccessView(DXDevice, (ID3D12Resource*) res->texture->backend, NULL, &uav_desc, dst);
            } break;
            case GFX_RESOURCE_TYPE_SAMPLER: {
                D3D12_CPU_DESCRIPTOR_HANDLE dst = sampler_base;
                dst.ptr += i * sampler_stride;
                ID3D12Device10_CreateSampler(DXDevice, (D3D12_SAMPLER_DESC*) res->sampler->backend, dst);
            } break;
            case GFX_RESOURCE_TYPE_SAMPLED_IMAGE: {
                D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc = ((resource_view_backend*) (res_view->backend))->srv_desc;

                D3D12_CPU_DESCRIPTOR_HANDLE dst = cbv_srv_base;
                dst.ptr += i * cbv_srv_stride;
                ID3D12Device10_CreateShaderResourceView(DXDevice, (ID3D12Resource*) res->texture->backend, &srv_desc, dst);
            } break;
            default:
                LOG_ERROR("[D3D12] Unsupported resource type %d in descriptor table update", res_view->type);
                break;
        }
    }
}

gfx_resource dx12_create_texture_resource(gfx_texture_create_info desc)
{
    gfx_resource resource = {0};
    resource.texture      = malloc(sizeof(gfx_texture));
    uuid_generate(&resource.texture->uuid);

    ID3D12Resource*     d3dresource = NULL;
    D3D12_RESOURCE_DESC res_desc    = {0};
    res_desc.Width                  = desc.width;
    res_desc.Height                 = desc.height;
    res_desc.DepthOrArraySize       = (UINT16) desc.depth;
    res_desc.MipLevels              = 1;    // No mip-maps support for RHI yet
    res_desc.Dimension              = dx12_util_tex_type_dimensions_translate(desc.tex_type);
    res_desc.Format                 = dx12_util_format_translate(desc.format);
    res_desc.SampleDesc.Count       = 1;
    res_desc.SampleDesc.Quality     = 0;
    res_desc.Layout                 = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    res_desc.Flags                  = D3D12_RESOURCE_FLAG_NONE;

    if (desc.res_type >= GFX_RESOURCE_TYPE_STORAGE_IMAGE && desc.res_type <= GFX_RESOURCE_TYPE_STORAGE_TEXEL_BUFFER)
        res_desc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
    else if (desc.res_type == GFX_RESOURCE_TYPE_COLOR_ATTACHMENT)
        res_desc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
    else if (desc.res_type == GFX_RESOURCE_TYPE_DEPTH_STENCIL_ATTACHMENT)
        res_desc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

    // Crated resource with memory backing
    D3D12_HEAP_PROPERTIES heapProps    = {0};
    heapProps.Type                     = D3D12_HEAP_TYPE_DEFAULT;
    heapProps.CPUPageProperty          = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    heapProps.MemoryPoolPreference     = D3D12_MEMORY_POOL_UNKNOWN;
    D3D12_RESOURCE_STATES initialState = D3D12_RESOURCE_STATE_COMMON;

    HRESULT hr = ID3D12Device10_CreateCommittedResource(DXDevice, &heapProps, D3D12_HEAP_FLAG_NONE, &res_desc, initialState, NULL, &IID_ID3D12Resource, &d3dresource);
    if (FAILED(hr)) {
        if (resource.texture)
            uuid_destroy(&resource.texture->uuid);
        LOG_ERROR("[D3D12] Failed to create commited texture resource! (HRESULT=0x%08X)", hr);
        return resource;
    }

    resource.texture->backend = d3dresource;

    return resource;
}

void dx12_destroy_texture_resource(gfx_resource* resource)
{
    uuid_destroy(&resource->texture->uuid);

    if (resource->texture->backend)
        ID3D12Resource_Release((ID3D12Resource*) resource->texture->backend);
    resource->texture->backend = NULL;

    if (resource->texture)
        free(resource->texture);
    resource->texture = NULL;
}

gfx_resource dx12_create_sampler(gfx_sampler_create_info desc)
{
    gfx_resource resource = {0};
    resource.sampler      = malloc(sizeof(gfx_sampler));
    uuid_generate(&resource.sampler->uuid);
    gfx_sampler* sampler = resource.sampler;
    sampler->backend     = malloc(sizeof(D3D12_SAMPLER_DESC));

    // cache the sampler since we create it within the heap! D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER
    D3D12_SAMPLER_DESC samplerDesc = {0};
    samplerDesc.Filter             = dx12_util_filter_translate(desc.min_filter, desc.mag_filter);
    samplerDesc.AddressU           = dx12_util_wrap_mode_translate(desc.wrap_mode);
    samplerDesc.AddressV           = dx12_util_wrap_mode_translate(desc.wrap_mode);
    samplerDesc.AddressW           = dx12_util_wrap_mode_translate(desc.wrap_mode);
    samplerDesc.ComparisonFunc     = D3D12_COMPARISON_FUNC_ALWAYS;
    samplerDesc.MipLODBias         = 0.0f;
    samplerDesc.MaxAnisotropy      = (UINT) desc.max_anisotropy;
    samplerDesc.BorderColor[0]     = 1.0f;
    samplerDesc.BorderColor[1]     = 1.0f;
    samplerDesc.BorderColor[2]     = 1.0f;
    samplerDesc.BorderColor[3]     = 1.0f;
    samplerDesc.MinLOD             = desc.min_lod;
    samplerDesc.MaxLOD             = desc.max_lod;

    // Cache this, we use during Descriptor table update time, deferred even further
    *((D3D12_SAMPLER_DESC*) sampler->backend) = samplerDesc;

    return resource;
}

void dx12_destroy_sampler(gfx_resource* resource)
{
    uuid_destroy(&resource->sampler->uuid);

    if (resource->sampler->backend)
        free(resource->sampler->backend);
    resource->sampler->backend = NULL;

    if (resource->sampler)
        free(resource->sampler);
    resource->sampler = NULL;
}

gfx_resource dx12_create_uniform_buffer_resource(uint32_t size)
{
    gfx_resource resource = {0};
    resource.ubo          = malloc(sizeof(gfx_uniform_buffer));
    uuid_generate(&resource.ubo->uuid);

    D3D12_HEAP_PROPERTIES heapProps = {0};
    heapProps.Type                  = D3D12_HEAP_TYPE_UPLOAD;
    heapProps.CPUPageProperty       = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    heapProps.MemoryPoolPreference  = D3D12_MEMORY_POOL_UNKNOWN;

    // Constant buffers are 256 Bytes aligned
    uint32_t aligned_size = (uint32_t) align_memory_size(size, 256);

    D3D12_RESOURCE_DESC buffer_desc = {0};
    buffer_desc.Dimension           = D3D12_RESOURCE_DIMENSION_BUFFER;
    buffer_desc.Width               = aligned_size;
    buffer_desc.Height              = 1;
    buffer_desc.DepthOrArraySize    = 1;
    buffer_desc.MipLevels           = 1;
    buffer_desc.SampleDesc.Count    = 1;
    buffer_desc.Layout              = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

    ID3D12Resource* d3dresource = NULL;
    HRESULT         hr          = ID3D12Device10_CreateCommittedResource(DXDevice, &heapProps, D3D12_HEAP_FLAG_NONE, &buffer_desc, D3D12_RESOURCE_STATE_GENERIC_READ, NULL, &IID_ID3D12Resource, &d3dresource);
    if (FAILED(hr)) {
        if (resource.ubo)
            uuid_destroy(&resource.ubo->uuid);
        LOG_ERROR("[D3D12] Failed to create commited constatn buffer resource! (HRESULT=0x%08X)", hr);
        return resource;
    }

    resource.ubo->backend = d3dresource;
    resource.ubo->size    = aligned_size;
    resource.ubo->offset  = 0;

    return resource;
}

void dx12_destroy_uniform_buffer_resource(gfx_resource* resource)
{
    uuid_destroy(&resource->ubo->uuid);

    if (resource->ubo->backend)
        ID3D12Resource_Release((ID3D12Resource*) resource->ubo->backend);
    resource->ubo->backend = NULL;

    if (resource->ubo)
        free(resource->ubo);
    resource->ubo = NULL;
}

void dx12_update_uniform_buffer(gfx_resource* resource, uint32_t size, uint32_t offset, void* data)
{
    ID3D12Resource* buffer = (ID3D12Resource*) resource->ubo->backend;
    D3D12_RANGE     range  = {offset, offset + size};
    void*           mapped = NULL;

    HRESULT hr = ID3D12Resource_Map(buffer, 0, &range, &mapped);
    if (SUCCEEDED(hr)) {
        memcpy((uint8_t*) mapped + offset, data, size);
        ID3D12Resource_Unmap(buffer, 0, NULL);
    }
}

gfx_resource_view dx12_create_texture_resource_view(const gfx_resource_view_create_info desc)
{
    gfx_resource_view view = {0};
    uuid_generate(&view.uuid);
    view.type = desc.res_type;

    resource_view_backend* backend = malloc(sizeof(resource_view_backend));
    view.backend                   = backend;

    // choose between SRV and UAV
    if (desc.res_type >= GFX_RESOURCE_TYPE_STORAGE_IMAGE && desc.res_type <= GFX_RESOURCE_TYPE_STORAGE_TEXEL_BUFFER) {
        // UAV
        D3D12_UNORDERED_ACCESS_VIEW_DESC uav_desc = {0};
        uav_desc.Format                           = dx12_util_format_translate(desc.texture.format);

        if (desc.texture.texture_type == GFX_TEXTURE_TYPE_2D_ARRAY) {
            uav_desc.ViewDimension                  = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
            uav_desc.Texture2DArray.MipSlice        = desc.texture.base_mip;
            uav_desc.Texture2DArray.FirstArraySlice = desc.texture.base_layer;
            uav_desc.Texture2DArray.ArraySize       = desc.texture.layer_count;
            uav_desc.Texture2DArray.PlaneSlice      = 0;
        } else if (desc.texture.texture_type == GFX_TEXTURE_TYPE_2D) {
            uav_desc.ViewDimension        = D3D12_UAV_DIMENSION_TEXTURE2D;
            uav_desc.Texture2D.MipSlice   = desc.texture.base_mip;
            uav_desc.Texture2D.PlaneSlice = 0;
        } else {
            LOG_ERROR("Unsupported UAV texture view type");
        }

        backend->uav_desc = uav_desc;

    } else if (desc.res_type == GFX_RESOURCE_TYPE_SAMPLED_IMAGE) {
        // SRV

        D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc = {0};
        srv_desc.Format                          = dx12_util_format_translate(desc.texture.format);
        srv_desc.Shader4ComponentMapping         = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

        if (desc.texture.texture_type == GFX_TEXTURE_TYPE_2D_ARRAY) {
            srv_desc.ViewDimension                      = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
            srv_desc.Texture2DArray.MostDetailedMip     = desc.texture.base_mip;
            srv_desc.Texture2DArray.MipLevels           = desc.texture.mip_levels;
            srv_desc.Texture2DArray.FirstArraySlice     = desc.texture.base_layer;
            srv_desc.Texture2DArray.ArraySize           = desc.texture.layer_count;
            srv_desc.Texture2DArray.PlaneSlice          = 0;
            srv_desc.Texture2DArray.ResourceMinLODClamp = 0.0f;
        } else if (desc.texture.texture_type == GFX_TEXTURE_TYPE_2D) {
            srv_desc.ViewDimension                 = D3D12_SRV_DIMENSION_TEXTURE2D;
            srv_desc.Texture2D.MostDetailedMip     = desc.texture.base_mip;
            srv_desc.Texture2D.MipLevels           = desc.texture.mip_levels;
            srv_desc.Texture2D.PlaneSlice          = 0;
            srv_desc.Texture2D.ResourceMinLODClamp = 0.0f;
        } else {
            assert(0 && "Unsupported SRV texture view type");
        }

        backend->srv_desc = srv_desc;

    } else if (desc.res_type == GFX_RESOURCE_TYPE_COLOR_ATTACHMENT) {
        D3D12_RENDER_TARGET_VIEW_DESC rtv_desc = {0};
        rtv_desc.Format                        = dx12_util_format_translate(desc.texture.format);

        if (desc.texture.texture_type == GFX_TEXTURE_TYPE_2D_ARRAY) {
            rtv_desc.ViewDimension                  = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
            rtv_desc.Texture2DArray.MipSlice        = desc.texture.base_mip;
            rtv_desc.Texture2DArray.FirstArraySlice = desc.texture.base_layer;
            rtv_desc.Texture2DArray.ArraySize       = desc.texture.layer_count;
            rtv_desc.Texture2DArray.PlaneSlice      = 0;
        } else if (desc.texture.texture_type == GFX_TEXTURE_TYPE_2D) {
            rtv_desc.ViewDimension        = D3D12_RTV_DIMENSION_TEXTURE2D;
            rtv_desc.Texture2D.MipSlice   = desc.texture.base_mip;
            rtv_desc.Texture2D.PlaneSlice = 0;
        } else {
            assert(0 && "Unsupported RTV texture view type");
        }

        backend->rtv_desc = rtv_desc;
    } else if (desc.res_type == GFX_RESOURCE_TYPE_DEPTH_STENCIL_ATTACHMENT) {
        D3D12_DEPTH_STENCIL_VIEW_DESC dsv_desc = {0};
        dsv_desc.Format                        = dx12_util_format_translate(desc.texture.format);
        dsv_desc.Flags                         = D3D12_DSV_FLAG_NONE;

        if (desc.texture.texture_type == GFX_TEXTURE_TYPE_2D_ARRAY) {
            dsv_desc.ViewDimension                  = D3D12_DSV_DIMENSION_TEXTURE2DARRAY;
            dsv_desc.Texture2DArray.MipSlice        = desc.texture.base_mip;
            dsv_desc.Texture2DArray.FirstArraySlice = desc.texture.base_layer;
            dsv_desc.Texture2DArray.ArraySize       = desc.texture.layer_count;
        } else if (desc.texture.texture_type == GFX_TEXTURE_TYPE_2D) {
            dsv_desc.ViewDimension      = D3D12_DSV_DIMENSION_TEXTURE2D;
            dsv_desc.Texture2D.MipSlice = desc.texture.base_mip;
        } else {
            assert(0 && "Unsupported DSV texture view type");
        }

        backend->dsv_desc = dsv_desc;
    }

    return view;
}

void dx12_destroy_texture_resource_view(gfx_resource_view* view)
{
    dx12_internal_destroy_res_view(view);
}

gfx_resource_view dx12d_create_sampler_resource_view(gfx_resource_view_create_info desc)
{
    gfx_resource_view view = {0};
    uuid_generate(&view.uuid);
    view.type = desc.res_type;

    return view;
}

void dx12d_destroy_sampler_resource_view(gfx_resource_view* view)
{
    dx12_internal_destroy_res_view(view);
}

gfx_resource_view dx12_create_uniform_buffer_resource_view(gfx_resource* resource, uint32_t size, uint32_t offset)
{
    gfx_resource_view view = {0};
    uuid_generate(&view.uuid);
    view.type = GFX_RESOURCE_TYPE_UNIFORM_BUFFER;

    resource_view_backend* backend = malloc(sizeof(resource_view_backend));
    view.backend                   = backend;

    ID3D12Resource* d3dresource = (ID3D12Resource*) resource->ubo->backend;

    D3D12_CONSTANT_BUFFER_VIEW_DESC cbv_desc = {
        .BufferLocation = ID3D12Resource_GetGPUVirtualAddress(d3dresource) + offset,
        .SizeInBytes    = (UINT) align_memory_size(size, 256)    // CBVs must be 256-byte aligned
    };

    backend->cbv_desc = cbv_desc;

    return view;
}

void dx12_destroy_uniform_buffer_resource_view(gfx_resource_view* view)
{
    dx12_internal_destroy_res_view(view);
}

//--------------------------------------------------------

rhi_error_codes dx12_frame_begin(gfx_context* context)
{
    TracyCFrameMarkStart("MainFrame");
    #if ENABLE_SYNC_LOGGING
    LOG_ERROR("*************************FRAME BEGIN*************************/");

    LOG_WARN("[Frame Begin] --- Old Current BackBuffer Index: %d", context->swapchain.current_backbuffer_idx);
    #endif

    // This is reverse to what Vulkan does, we first wait for previous work to be done
    // and then acquire a new back buffer, because acquire is a GPU operation in vulkan,
    // here we just ask for index and wait on GPU until work is done and that back buffer is free to use
    dx12_acquire_image(context);
    uint32_t       frame_idx          = context->swapchain.current_backbuffer_idx;
    gfx_syncobj    frame_fence        = context->frame_sync.timeline_syncobj;
    gfx_sync_point current_sync_point = context->frame_sync.frame_syncpoint[frame_idx];
    #if ENABLE_SYNC_LOGGING
    LOG_SUCCESS("[Frame Begin] --- current_sync_point to wait on: %llu (frame_idx = %d)", current_sync_point, frame_idx);
    #endif

    dx12_wait_on_previous_cmds(&frame_fence, current_sync_point);

    // for API completion sake
    context->inflight_frame_idx  = frame_idx;
    context->current_syncobj_idx = frame_idx;

    memset(context->cmd_queue.cmds, 0, context->cmd_queue.cmds_count * sizeof(gfx_cmd_buf*));
    context->cmd_queue.cmds_count = 0;

    return Success;
}

rhi_error_codes dx12_frame_end(gfx_context* context)
{
    dx12_present(context);

    gfx_sync_point wait_value = dx12_internal_signal(context);
    uint32_t       frame_idx  = context->swapchain.current_backbuffer_idx;
    #if ENABLE_SYNC_LOGGING
    LOG_WARN("[POST SIGNAL] updating_sync_point_to_wait_next: %llu (frame_idx = %d)", wait_value, frame_idx);
    #endif
    context->frame_sync.frame_syncpoint[frame_idx] = wait_value;

    #if ENABLE_SYNC_LOGGING
    LOG_ERROR("//-----------------------FRAME END-------------------------//");
    #endif

    TracyCFrameMarkEnd("MainFrame");
    return Success;
}

//--------------------------------------------------------

rhi_error_codes dx12_wait_on_previous_cmds(const gfx_syncobj* in_flight_sync, gfx_sync_point sync_point)
{
    TracyCZoneNC(ctx, "Wait on Prev Frame Image", COLOR_WAIT, true);
    syncobj_backend* backend = (syncobj_backend*) (in_flight_sync->backend);

    gfx_sync_point completed = ID3D12Fence_GetCompletedValue(backend->fence);

    // Only verbose logging when diagnosing; treat waits as errors otherwise
    #if ENABLE_SYNC_LOGGING
    LOG_WARN("[WAIT] want=%llu; before completed=%llu", sync_point, completed);
    #endif

    if (completed < sync_point) {
        // Set the fence event and check for failure
        HRESULT hr = ID3D12Fence_SetEventOnCompletion(backend->fence, sync_point, backend->fenceEvent);
        if (FAILED(hr)) {
            LOG_ERROR("[WAIT ERR] SetEventOnCompletion(%llu) failed -> 0x%08X", sync_point, hr);
        }

        // Wait for the event and log errors only
        DWORD result = WaitForSingleObject(backend->fenceEvent, INFINITE);
        if (result != WAIT_OBJECT_0) {
            LOG_ERROR("[WAIT ERR] WaitForSingleObject -> %s",
                result == WAIT_TIMEOUT ? "WAIT_TIMEOUT" : "WAIT_FAILED");
        }

        // Verify fence advanced
        gfx_sync_point new_completed = ID3D12Fence_GetCompletedValue(backend->fence);
    #if ENABLE_SYNC_LOGGING
        LOG_WARN("[WAIT DONE] fence advanced to %llu (wanted >= %llu)", new_completed, sync_point);
    #else
        if (new_completed < sync_point) {
            LOG_ERROR("[WAIT ERR] fence did not advance: completed=%llu, expected>=%llu", new_completed, sync_point);
        }
    #endif
    } else {
    #if ENABLE_SYNC_LOGGING
        LOG_WARN("[NO WAIT] fence (%llu) already >= %llu", completed, sync_point);
    #endif
    }

    TracyCZoneEnd(ctx);
    return Success;
}

rhi_error_codes dx12_acquire_image(gfx_context* context)
{
    TracyCZoneNC(ctx, "Acquire Image", COLOR_ACQUIRE, true);

    swapchain_backend* sc_backend             = (swapchain_backend*) context->swapchain.backend;
    context->swapchain.current_backbuffer_idx = IDXGISwapChain4_GetCurrentBackBufferIndex(sc_backend->swapchain);
    #if ENABLE_SYNC_LOGGING
    LOG_WARN("New Current BackBuffer Index: %d", context->swapchain.current_backbuffer_idx);
    #endif
    TracyCZoneEnd(ctx);

    return Success;
}

rhi_error_codes dx12_gfx_cmd_enque_submit(gfx_cmd_queue* cmd_queue, gfx_cmd_buf* cmd_buff)
{
    TracyCZoneN(ctx, "dx12_gfx_cmd_enque_submit", true);

    cmd_queue->cmds[cmd_queue->cmds_count] = (ID3D12GraphicsCommandList*) (cmd_buff->backend);
    cmd_queue->cmds_count++;
    TracyCZoneEnd(ctx);

    return Success;
}

rhi_error_codes dx12_gfx_cmd_submit_queue(const gfx_cmd_queue* cmd_queue, gfx_submit_syncobj submit_sync)
{
    // DX12 doesn't do any sync for submission/presentation, so ignored
    // if want manual points we can insert signal/wait on those time points
    // This is better done via explicit API, since vulkan requires us to submit
    // sync points during submission but DX requires API calls to wait/signal
    // if we go this route, we can enqueue the sync points and pass it off to gfx_submit_syncobj for vulkan
    // but for simple single queue sync we can ignore it.
    UNUSED(submit_sync);
    UNUSED(cmd_queue);
    TracyCZoneNC(ctx, "ExecuteCommandLists", COLOR_CMD_SUBMIT, true);

    if (cmd_queue->cmds_count > 0)
        ID3D12CommandQueue_ExecuteCommandLists((ID3D12CommandQueue*) (s_DXCtx.direct_queue), cmd_queue->cmds_count, (ID3D12CommandList**) cmd_queue->cmds);
    TracyCZoneEnd(ctx);

    return Success;
}

rhi_error_codes dx12_gfx_cmd_submit_for_rendering(gfx_context* context)
{
    gfx_submit_syncobj submit_sync = {0};
    #if ENABLE_SYNC_LOGGING
    LOG_SUCCESS("[PRE-SUBMIT] global_syncpoint: %llu", context->frame_sync.global_syncpoint);
    #endif
    return dx12_gfx_cmd_submit_queue(&context->cmd_queue, submit_sync);
}

rhi_error_codes dx12_present(const gfx_context* context)
{
    TracyCZoneNC(ctx, "Present", COLOR_PRESENT, true);

    const swapchain_backend* sc_backend = (const swapchain_backend*) context->swapchain.backend;
    HRESULT                  hr         = IDXGISwapChain4_Present(sc_backend->swapchain, 0, DXGI_PRESENT_ALLOW_TEARING);
    if (FAILED(hr)) {
        LOG_ERROR("[D3D12] Swapchain failed to present (HRESULT = 0x%08X)", (unsigned int) hr);
        return FailedPresent;
    }

    TracyCZoneEnd(ctx);

    return Success;
}

rhi_error_codes dx12_resize_swapchain(gfx_context* context, uint32_t width, uint32_t height)
{
    gfx_swapchain* swapchain = &context->swapchain;

    dx12_flush_gpu_work(context);

    swapchain->width = width;
    swapchain->width = height;

    swapchain_backend* backend = (swapchain_backend*) swapchain->backend;

    for (uint32_t i = 0; i < backend->image_count; i++) {
        if (backend->backbuffers[i]) {
            ID3D12Resource_Release(backend->backbuffers[i]);
            backend->backbuffers[i] = NULL;
        }
    }

    DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
    IDXGISwapChain4*     sc4           = backend->swapchain;
    IDXGISwapChain4_GetDesc(sc4, &swapChainDesc);
    IDXGISwapChain4_ResizeBuffers(sc4, MAX_DX_SWAPCHAIN_BUFFERS, width, height, DX_SWAPCHAIN_FORMAT, swapChainDesc.Flags);

    dx12_internal_update_swapchain_rtv(swapchain);

    dx12_flush_gpu_work(context);

    return Success;
}

rhi_error_codes dx12_begin_gfx_cmd_recording(const gfx_cmd_pool* allocator, const gfx_cmd_buf* cmd_buf)
{
    TracyCZoneNC(ctx, "Begin Cmd Recording", COLOR_CMD_RECORD, true);

    // Don't reset allocator, we use a persistent design
    ID3D12CommandAllocator*    d3dallocator = (ID3D12CommandAllocator*) (allocator->backend);
    ID3D12GraphicsCommandList* cmd_list     = (ID3D12GraphicsCommandList*) (cmd_buf->backend);

    HRESULT hr = ID3D12CommandAllocator_Reset(d3dallocator);
    if (FAILED(hr)) {
        LOG_ERROR("[D3D12] Failed to reset command allocator for this frame!");
        return FailedCommandAllocatorReset;
    }

    hr = ID3D12GraphicsCommandList_Reset(cmd_list, d3dallocator, NULL);
    if (FAILED(hr)) {
        LOG_ERROR("[D3D12] Failed to reset gfx command list!");
        return FailedCommandBegin;
    }
    TracyCZoneEnd(ctx);

    return Success;
}

rhi_error_codes dx12_end_gfx_cmd_recording(const gfx_cmd_buf* cmd_buf)
{
    TracyCZoneNC(ctx, "End Cmd Recording", COLOR_CMD_RECORD, true);

    ID3D12GraphicsCommandList* cmd_list = (ID3D12GraphicsCommandList*) (cmd_buf->backend);
    HRESULT                    hr       = ID3D12GraphicsCommandList_Close(cmd_list);
    if (FAILED(hr)) {
        LOG_ERROR("[D3D12] Failed to close gfx command list!");
        return FailedCommandEnd;
    }
    TracyCZoneEnd(ctx);

    return Success;
}

rhi_error_codes dx12_begin_render_pass(const gfx_cmd_buf* cmd_buf, gfx_render_pass render_pass, uint32_t backbuffer_index)
{
    TracyCZoneNC(ctx, "Begin Render Pass", COLOR_ACQUIRE, true);

    ID3D12GraphicsCommandList* cmd_list = (ID3D12GraphicsCommandList*) cmd_buf->backend;
    if (!render_pass.is_compute_pass) {
        D3D12_CPU_DESCRIPTOR_HANDLE rtvs[MAX_RT] = {0};
        D3D12_CPU_DESCRIPTOR_HANDLE dsv          = {0};

        if (render_pass.is_swap_pass) render_pass.color_attachments_count = 1;

        for (uint32_t i = 0; i < render_pass.color_attachments_count; ++i) {
            gfx_attachment attachment = render_pass.color_attachments[i];

            if (render_pass.is_swap_pass) {
                if (!render_pass.swapchain)
                    LOG_ERROR("[Vulkan] pass is marked as swap pass but swapchain is empty");
                swapchain_backend*          sc_backend = (swapchain_backend*) render_pass.swapchain->backend;
                D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle  = sc_backend->rtv_handle_start;
                rtvHandle.ptr += backbuffer_index * ID3D12Device10_GetDescriptorHandleIncrementSize(DXDevice, D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
                // store the correct back buffer offset RTV handle
                rtvs[i] = rtvHandle;
                // clear it
                if (attachment.clear)
                    ID3D12GraphicsCommandList_ClearRenderTargetView(cmd_list, rtvHandle, attachment.clear_color.raw, 0, NULL);

            } else {
                LOG_ERROR("// TODO: UNIMPLEMENTED");
                LOG_ERROR("// TODO: use gfx_texture->backend as imageview");
    #ifdef _MSC_VER
                __debugbreak();
    #endif
            }
        }

        UNUSED(dsv);
        // TODO: Depth attachments, the SDF renderer doesn't need this yet!
        //if (render_pass.depth_attachment.texture) {
        //    texture_backend*            tex_backend = (texture_backend*) render_pass.depth_attachment.texture->backend;
        //    D3D12_CPU_DESCRIPTOR_HANDLE dsv         = tex_backend->dsv_handle;
        //    if (render_pass.depth_attachment.clear) {
        //        D3D12_CLEAR_FLAGS flags = D3D12_CLEAR_FLAG_DEPTH;
        //        cmd_list->lpVtbl->ClearDepthStencilView(cmd_list, dsv, flags, 1.0f, 0, 0, NULL);
        //    }
        //    cmd_list->lpVtbl->OMSetRenderTargets(cmd_list, render_pass.color_attachments_count, rtvs, FALSE, &dsv);
        //}

        ID3D12GraphicsCommandList_OMSetRenderTargets(cmd_list, render_pass.color_attachments_count, rtvs, FALSE, NULL);

        D3D12_VIEWPORT vp = {
            .TopLeftX = 0.0f,
            .TopLeftY = 0.0f,
            .Width    = (FLOAT) render_pass.extents.x,
            .Height   = (FLOAT) render_pass.extents.y,
            .MinDepth = 0.0f,
            .MaxDepth = 1.0f};
        D3D12_RECT scissor = {
            .left   = 0,
            .top    = 0,
            .right  = (LONG) render_pass.extents.x,
            .bottom = (LONG) render_pass.extents.y};

        ID3D12GraphicsCommandList_RSSetViewports(cmd_list, 1, &vp);
        ID3D12GraphicsCommandList_RSSetScissorRects(cmd_list, 1, &scissor);
    }
    TracyCZoneEnd(ctx);
    return Success;
}

rhi_error_codes dx12_end_render_pass(const gfx_cmd_buf* cmd_buf, gfx_render_pass render_pass)
{
    UNUSED(cmd_buf);
    UNUSED(render_pass);
    TracyCZoneNC(ctx, "Begin Render Pass", COLOR_ACQUIRE, true);
    TracyCZoneEnd(ctx);

    return Success;
}

rhi_error_codes dx12_set_viewport(const gfx_cmd_buf* cmd_buf, gfx_viewport viewport)
{
    D3D12_VIEWPORT desc = {0};
    desc.TopLeftX       = (FLOAT) viewport.x;
    desc.TopLeftY       = (FLOAT) viewport.y;
    desc.Width          = (FLOAT) viewport.width;
    desc.Height         = (FLOAT) viewport.height;
    desc.MinDepth       = (FLOAT) viewport.min_depth;
    desc.MaxDepth       = (FLOAT) viewport.max_depth;

    ID3D12GraphicsCommandList* cmd_list = (ID3D12GraphicsCommandList*) (cmd_buf->backend);
    ID3D12GraphicsCommandList_RSSetViewports(cmd_list, 1, &desc);

    return Success;
}
rhi_error_codes dx12_set_scissor(const gfx_cmd_buf* cmd_buf, gfx_scissor scissor)
{
    D3D12_RECT rect = {0};
    rect.left       = (LONG) scissor.x;
    rect.top        = (LONG) scissor.y;
    rect.right      = (LONG) (scissor.x + scissor.width);
    rect.bottom     = (LONG) (scissor.y + scissor.height);

    ID3D12GraphicsCommandList* cmd_list = (ID3D12GraphicsCommandList*) (cmd_buf->backend);
    ID3D12GraphicsCommandList_RSSetScissorRects(cmd_list, 1, &rect);

    return Success;
}

rhi_error_codes dx12_bind_gfx_pipeline(const gfx_cmd_buf* cmd_buf, const gfx_pipeline* pipeline)
{
    pipeline_backend* backend = (pipeline_backend*) (pipeline->backend);

    ID3D12GraphicsCommandList* cmd_list = (ID3D12GraphicsCommandList*) (cmd_buf->backend);
    ID3D12GraphicsCommandList_IASetPrimitiveTopology(cmd_list, backend->topology);
    ID3D12GraphicsCommandList_SetPipelineState(cmd_list, backend->pso);

    return Success;
}

rhi_error_codes dx12_bind_compute_pipeline(const gfx_cmd_buf* cmd_buf, const gfx_pipeline* pipeline)
{
    ID3D12GraphicsCommandList* cmd_list = (ID3D12GraphicsCommandList*) (cmd_buf->backend);
    ID3D12GraphicsCommandList_SetPipelineState(cmd_list, (ID3D12PipelineState*) pipeline->backend);

    return Success;
}

rhi_error_codes dx12_device_bind_root_signature(const gfx_cmd_buf* cmd_buf, const gfx_root_signature* root_signature)
{
    ID3D12GraphicsCommandList* cmd_list = (ID3D12GraphicsCommandList*) (cmd_buf->backend);
    ID3D12GraphicsCommandList_SetGraphicsRootSignature(cmd_list, (ID3D12RootSignature*) root_signature->backend);

    return Success;
}

rhi_error_codes dx12_bind_descriptor_table(const gfx_cmd_buf* cmd_buf, const gfx_descriptor_table* descriptor_table, gfx_pipeline_type pipeline_type)
{
    descriptor_table_backend* descriptor_backend = (descriptor_table_backend*) (descriptor_table->backend);

    ID3D12DescriptorHeap* heaps[] = {
        descriptor_backend->cbv_uav_srv_heap,
        descriptor_backend->sampler_heap,
        descriptor_backend->rtv_heap,
        descriptor_backend->dsv_heap,
    };

    ID3D12GraphicsCommandList* cmd_list = (ID3D12GraphicsCommandList*) (cmd_buf->backend);

    // Bind heaps and GPU descriptor handles
    ID3D12GraphicsCommandList_SetDescriptorHeaps(cmd_list, descriptor_backend->heap_count, heaps);

    D3D12_GPU_DESCRIPTOR_HANDLE cbv_srv_uav_base;
    ID3D12DescriptorHeap_GetGPUDescriptorHandleForHeapStart(descriptor_backend->cbv_uav_srv_heap, &cbv_srv_uav_base);
    D3D12_GPU_DESCRIPTOR_HANDLE sampler_base;
    ID3D12DescriptorHeap_GetGPUDescriptorHandleForHeapStart(descriptor_backend->cbv_uav_srv_heap, &sampler_base);

    //--------------------------------------------------------------------
    // FIXME: This is a very fixed design assuming we have samplers at root param2 after CBV/SRV/UAV and only 2 root params but that's not the case
    // Bind to root parameter index == set index
    if (pipeline_type == GFX_PIPELINE_TYPE_GRAPHICS) {
        ID3D12GraphicsCommandList_SetGraphicsRootDescriptorTable(cmd_list, 0, cbv_srv_uav_base);
        ID3D12GraphicsCommandList_SetGraphicsRootDescriptorTable(cmd_list, 1, sampler_base);

    } else {
        ID3D12GraphicsCommandList_SetComputeRootDescriptorTable(cmd_list, 0, cbv_srv_uav_base);
        ID3D12GraphicsCommandList_SetComputeRootDescriptorTable(cmd_list, 1, sampler_base);
    }
    //--------------------------------------------------------------------

    return Success;
}

rhi_error_codes dx12_bind_push_constant(const gfx_cmd_buf* cmd_buf, const gfx_root_signature* root_sig, gfx_push_constant push_constant)
{
    ID3D12GraphicsCommandList* cmd_list = (ID3D12GraphicsCommandList*) cmd_buf->backend;

    // STart off after the descriptor tables, just one per shader so we are good to go this way
    uint32_t root_param_index = root_sig->descriptor_layout_count;

    uint32_t num_values = push_constant.size / sizeof(uint32_t);

    if (push_constant.stage == GFX_PIPELINE_TYPE_GRAPHICS) {
        ID3D12GraphicsCommandList_SetGraphicsRoot32BitConstants(cmd_list, root_param_index, num_values, push_constant.data, push_constant.offset);
    } else {
        ID3D12GraphicsCommandList_SetComputeRoot32BitConstants(cmd_list, root_param_index, num_values, push_constant.data, push_constant.offset);
    }

    return Success;
}

rhi_error_codes dx12_draw(const gfx_cmd_buf* cmd_buf, uint32_t vertex_count, uint32_t instance_count, uint32_t first_vertex, uint32_t first_instance)
{
    TracyCZoneNC(ctx, "DrawAuto", COLOR_ACQUIRE, true);

    ID3D12GraphicsCommandList* cmd_list = (ID3D12GraphicsCommandList*) (cmd_buf->backend);
    ID3D12GraphicsCommandList_DrawInstanced(cmd_list, vertex_count, instance_count, first_vertex, first_instance);

    TracyCZoneEnd(ctx);
    return Success;
}

rhi_error_codes dx12_dispatch(const gfx_cmd_buf* cmd_buf, uint32_t dimX, uint32_t dimY, uint32_t dimZ)
{
    TracyCZoneNC(ctx, "Dispatch", COLOR_ACQUIRE, true);

    ID3D12GraphicsCommandList* cmd_list = (ID3D12GraphicsCommandList*) (cmd_buf->backend);
    ID3D12GraphicsCommandList_Dispatch(cmd_list, dimX, dimY, dimZ);

    TracyCZoneEnd(ctx);
    return Success;
}

rhi_error_codes dx12_transition_image_layout(const gfx_cmd_buf* cmd_buf, const gfx_resource* image, gfx_image_layout old_layout, gfx_image_layout new_layout)
{
    TracyCZoneNC(ctx, "ImageTransitionLayout", COLOR_ACQUIRE, true);

    ID3D12GraphicsCommandList* cmd_list = (ID3D12GraphicsCommandList*) (cmd_buf->backend);

    D3D12_RESOURCE_BARRIER barrier = {0};
    barrier.Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Flags                  = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barrier.Transition.pResource   = image->texture->backend;
    barrier.Transition.StateBefore = dx12_util_translate_image_layout_to_res_state(old_layout);
    barrier.Transition.StateAfter  = dx12_util_translate_image_layout_to_res_state(new_layout);
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

    ID3D12GraphicsCommandList_ResourceBarrier(cmd_list, 1, &barrier);

    TracyCZoneEnd(ctx);
    return Success;
}

rhi_error_codes dx12_transition_swapchain_layout(const gfx_cmd_buf* cmd_buf, const gfx_swapchain* sc, gfx_image_layout old_layout, gfx_image_layout new_layout)
{
    TracyCZoneNC(ctx, "SwapchainTransitionLayout", COLOR_ACQUIRE, true);

    ID3D12GraphicsCommandList* cmd_list = (ID3D12GraphicsCommandList*) (cmd_buf->backend);
    swapchain_backend*         backend  = (swapchain_backend*) sc->backend;

    D3D12_RESOURCE_BARRIER barrier = {0};
    barrier.Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Flags                  = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barrier.Transition.pResource   = backend->backbuffers[sc->current_backbuffer_idx];
    barrier.Transition.StateBefore = dx12_util_translate_image_layout_to_res_state(old_layout);
    barrier.Transition.StateAfter  = dx12_util_translate_image_layout_to_res_state(new_layout);
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

    ID3D12GraphicsCommandList_ResourceBarrier(cmd_list, 1, &barrier);

    TracyCZoneEnd(ctx);
    return Success;
}

#endif    // _WIN32
