#include "backend_dx12.h"

#ifdef _WIN32

    #include "../core/common.h"
    #include "../core/containers/typed_growable_array.h"
    #include "../core/logging/log.h"

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

#endif

#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

// clang-format on

    #include <inttypes.h>
    #include <stdlib.h>
    #include <string.h>    // memset

//--------------------------------------------------------
// TODO:
// - [x] Use the IDXGIFactory7 to query for the best GPUa and create the IDXGIAdapter4 or use AgilitySDK with DeviceFactory
//   - [x] Figure out how to replicate COM model in C, use lpvtable and manually release memory for ID3DXXXX/iDXGIXXXX objects
// - [x] Create the device
// - [x] cache the device Features
// - [x] Debug Layers (D3D12 + DXGI + LiveObjectTracking)
// - [x] Create swapchain and RTV heaps and extract back buffer
// - [x] gfx_syncobj primitives
// - [x] Create command allocators
// - [x] Create command lists from allocator
// - [ ] Rendering Loop: Basic submit/present flow using Fence counting etc. for Clear color
//   - [x] frame_begin & end/wait_on_cmds/submit/present
//   - [x] Create in context! + decide on per-in flight fence or just one global fence
//   - [x] Test and resolve issues
//   - [ ] Swapchain resize
//   - [ ] **SUBMIT** API
// - [ ] Hello Triangle without VB/IB in single shader quad
//   - [ ] Drawing API
//   - [ ] Shaders/Loading
//   - [ ] Pipeline API
//   - [ ] Root Signature
//   - [ ] Hello Triangle Shader + Draw loop
// - [ ] Descriptor Heaps API
// - [ ] Create Texture/Buffer resources
// - [ ] Resource Views API
// - [ ] Barriers API
// - [ ] Root Constants
// - [ ] Dispatch API
// - [ ] Restore SDF renderer

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
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
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
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
};
//--------------------------------------------------------

    #define HR_CHECK(x)                                                                                                              \
        do {                                                                                                                         \
            HRESULT hr__ = (x);                                                                                                      \
            if (FAILED(hr__)) {                                                                                                      \
                LOG_ERROR("[D3D12] [DX12] %s:%d\n ==> %s failed (HRESULT = 0x%08X)\n", __FILE__, __LINE__, #x, (unsigned int) hr__); \
                abort();                                                                                                             \
            }                                                                                                                        \
        } while (0)

    #define MAX_DX_SWAPCHAIN_BUFFERS 3

    #define DXDevice s_DXCtx.device

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
            HR_CHECK(D3D12CreateDevice((IUnknown*) adapter4, min_feat_level, &IID_ID3D12Device, NULL));
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
    gfx_syncobj* timeline_syncobj = &context->frame_sync.timeline_syncobj;

    uint64_t signal_value = ++(context->frame_sync.global_syncpoint);
    ID3D12CommandQueue_Signal(((context_backend*) context->backend)->direct_queue, ((syncobj_backend*) (timeline_syncobj->backend))->fence, signal_value);

    dx12_wait_on_previous_cmds(timeline_syncobj, context->frame_sync.global_syncpoint);
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

    backend->image_count = MAX_DX_SWAPCHAIN_BUFFERS;
    ID3D12DescriptorHeap_GetCPUDescriptorHandleForHeapStart(backend->rtv_heap, &backend->rtv_handle_start);

    for (uint32_t i = 0; i < MAX_DX_SWAPCHAIN_BUFFERS; i++) {
        hr = IDXGISwapChain4_GetBuffer(backend->swapchain, i, &IID_ID3D12Resource, &backend->backbuffers[i]);
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
    swapChainDesc.Format                = DXGI_FORMAT_R8G8B8A8_UNORM;
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

    if (type == GFX_SYNCOBJ_TYPE_CPU) {
        // create a CPU event to wait on
        backend->fenceEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
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

            if (syncobj->type == GFX_SYNCOBJ_TYPE_CPU)
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
}

//--------------------------------------------------------

rhi_error_codes dx12_frame_begin(gfx_context* context)
{
    // This is reverse to what Vulkan does, we first wait for previous work to be done
    // and then acquire a new back buffer, because acquire is a GPU operation in vulkan,
    // here we just ask for index and wait on GPU until work is done and that back buffer is free to use
    dx12_acquire_image(context);
    uint32_t       frame_idx          = context->swapchain.current_backbuffer_idx;
    gfx_syncobj    frame_fence        = context->frame_sync.timeline_syncobj;
    gfx_sync_point current_sync_point = context->frame_sync.frame_syncpoint[frame_idx];
    dx12_wait_on_previous_cmds(&frame_fence, current_sync_point);

    memset(context->cmd_queue.cmds, 0, context->cmd_queue.cmds_count * sizeof(gfx_cmd_buf*));
    context->cmd_queue.cmds_count = 0;

    gfx_cmd_pool*           pool         = &context->draw_cmds_pool[frame_idx];
    ID3D12CommandAllocator* d3dallocator = (ID3D12CommandAllocator*) (pool->backend);
    HRESULT                 hr           = ID3D12CommandAllocator_Reset(d3dallocator);
    if (FAILED(hr)) {
        LOG_ERROR("[D3D12] Failed to reset command allocator for this frame!");
        return FailedCommandAllocatorReset;
    }

    return Success;
}

rhi_error_codes dx12_frame_end(gfx_context* context)
{
    dx12_present(context);

    uint32_t frame_idx = context->swapchain.current_backbuffer_idx;

    gfx_syncobj* timeline_syncobj                  = &context->frame_sync.timeline_syncobj;
    uint64_t     signal_value                      = ++(context->frame_sync.global_syncpoint);
    context->frame_sync.frame_syncpoint[frame_idx] = signal_value;

    // for API completion sake
    context->inflight_frame_idx  = frame_idx;
    context->current_syncobj_idx = frame_idx;

    ID3D12CommandQueue_Signal(((context_backend*) context->backend)->direct_queue, ((syncobj_backend*) (timeline_syncobj->backend))->fence, signal_value);

    return Success;
}

//--------------------------------------------------------

rhi_error_codes dx12_wait_on_previous_cmds(const gfx_syncobj* in_flight_sync, gfx_sync_point sync_point)
{
    syncobj_backend* backend = (syncobj_backend*) (in_flight_sync->backend);
    if (ID3D12Fence_GetCompletedValue(backend->fence) < sync_point) {
        ID3D12Fence_SetEventOnCompletion(backend->fence, sync_point, backend->fenceEvent);
        WaitForSingleObject(backend->fenceEvent, UINT64_MAX);
    }

    return Success;
}

rhi_error_codes dx12_acquire_image(gfx_context* context)
{
    swapchain_backend* sc_backend             = (swapchain_backend*) context->swapchain.backend;
    context->swapchain.current_backbuffer_idx = IDXGISwapChain4_GetCurrentBackBufferIndex(sc_backend->swapchain);

    return Success;
}

rhi_error_codes dx12_gfx_cmd_enque_submit(gfx_cmd_queue* cmd_queue, gfx_cmd_buf* cmd_buff)
{
    cmd_queue->cmds[cmd_queue->cmds_count] = cmd_buff;
    cmd_queue->cmds_count++;

    return Success;
}

rhi_error_codes dx12_gfx_cmd_submit_queue(const gfx_cmd_queue* cmd_queue, gfx_submit_syncobj submit_sync)
{
    UNUSED(cmd_queue);
    UNUSED(submit_sync);
    return FailedUnknown;
}

rhi_error_codes dx12_gfx_cmd_submit_for_rendering(gfx_context* context)
{
    UNUSED(context);
    return FailedUnknown;
}

rhi_error_codes dx12_present(const gfx_context* context)
{
    const swapchain_backend* sc_backend = (const swapchain_backend*) context->swapchain.backend;
    HRESULT                  hr         = IDXGISwapChain4_Present(sc_backend->swapchain, 0, DXGI_PRESENT_ALLOW_TEARING);
    if (FAILED(hr)) {
        LOG_ERROR("[D3D12] Swapchain failed to present (HRESULT = 0x%08X)", (unsigned int) hr);
        return FailedPresent;
    }

    return Success;
}

rhi_error_codes dx12_resize_swapchain(gfx_context* context, uint32_t width, uint32_t height)
{
    gfx_swapchain* swapchain = &context->swapchain;

    dx12_flush_gpu_work(context);

    dx12_internal_destroy_backbuffers(swapchain);

    for (int i = 0; i < MAX_DX_SWAPCHAIN_BUFFERS; ++i) {
        context->frame_sync.frame_syncpoint[i] = context->frame_sync.global_syncpoint;
    }

    swapchain->width = width;
    swapchain->width = height;

    dx12_internal_create_backbuffers(swapchain);

    return Success;
}

rhi_error_codes dx12_begin_gfx_cmd_recording(const gfx_cmd_pool* allocator, const gfx_cmd_buf* cmd_buf)
{
    // Don't reset allocator, we use a persistent design
    ID3D12CommandAllocator*    d3dallocator = (ID3D12CommandAllocator*) (allocator->backend);
    ID3D12GraphicsCommandList* cmd_list     = (ID3D12GraphicsCommandList*) (cmd_buf->backend);

    HRESULT hr = ID3D12GraphicsCommandList_Reset(cmd_list, d3dallocator, NULL);
    if (FAILED(hr)) {
        LOG_ERROR("[D3D12] Failed to reset gfx command list!");
        return FailedCommandBegin;
    }

    return Success;
}

rhi_error_codes dx12_end_gfx_cmd_recording(const gfx_cmd_buf* cmd_buf)
{
    ID3D12GraphicsCommandList* cmd_list = (ID3D12GraphicsCommandList*) (cmd_buf->backend);
    HRESULT                    hr       = ID3D12GraphicsCommandList_Close(cmd_list);
    if (FAILED(hr)) {
        LOG_ERROR("[D3D12] Failed to close gfx command list!");
        return FailedCommandEnd;
    }

    return Success;
}

#endif    // _WIN32