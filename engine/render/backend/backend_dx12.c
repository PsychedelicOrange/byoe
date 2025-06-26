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
// - [ ] gfx_syncobj primitives
// - [ ] Create command allocators
// - [ ] Create command lists from allocator
// - [ ] Basic submit/present flow using Fence counting +
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
// - Restore SDF renderer

//--------------------------------------------------------
const rhi_jumptable dx12_jumptable = {
    dx12_ctx_init,
    dx12_ctx_destroy,
    dx12_flush_gpu_work,

    dx12_create_swapchain,
    dx12_destroy_swapchain,
};
//--------------------------------------------------------

    #define HR_CHECK(x)                                                                                                      \
        do {                                                                                                                 \
            HRESULT hr__ = (x);                                                                                              \
            if (FAILED(hr__)) {                                                                                              \
                LOG_ERROR("[DX12] %s:%d\n ==> %s failed (HRESULT = 0x%08X)\n", __FILE__, __LINE__, #x, (unsigned int) hr__); \
                abort();                                                                                                     \
            }                                                                                                                \
        } while (0)

    #define IID_PPV_ARGS_C(iid, ppType) \
        (const IID*) (iid), (void**) (ppType)

    #define MAX_DX_SWAPCHAIN_BUFFERS 3

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

        HRESULT hr = factory->lpVtbl->EnumAdapters1(factory, i, &adapter1);
        if (hr == DXGI_ERROR_NOT_FOUND)
            break;
        if (FAILED(hr))
            continue;

        // Query for IDXGIAdapter4 to get DXGI_ADAPTER_DESC3
        IDXGIAdapter4* adapter4 = NULL;
        hr                      = adapter1->lpVtbl->QueryInterface(adapter1, IID_PPV_ARGS_C(&IID_IDXGIAdapter4, &adapter4));
        if (FAILED(hr)) {
            LOG_ERROR("[DX12] Failed to query IDXGIAdapter4 from IDXGIAdapter1 (HRESULT = 0x%08X)", (unsigned int) hr);
            adapter1->lpVtbl->Release(adapter1);
            continue;    // Adapter doesn't support DXGI 1.4+
        }
        adapter1->lpVtbl->Release(adapter1);    // Done with IDXGIAdapter1

        // Get the adapter description
        DXGI_ADAPTER_DESC3 desc = {0};
        hr                      = adapter4->lpVtbl->GetDesc3(adapter4, &desc);
        if (FAILED(hr)) {
            LOG_ERROR("[DX12] Failed to get DXGI_ADAPTER_DESC3 from IDXGIAdapter4 (HRESULT = 0x%08X)", (unsigned int) hr);
            adapter4->lpVtbl->Release(adapter4);
            continue;    // Failed to get description
        }

        // Skip software adapters (like WARP)
        if (desc.Flags & DXGI_ADAPTER_FLAG3_SOFTWARE) {
            adapter4->lpVtbl->Release(adapter4);
            continue;
        }

        // Select adapter with highest VRAM

        if (desc.DedicatedVideoMemory > maxDedicatedVRAM) {
            if (best_adapter)
                best_adapter->lpVtbl->Release(best_adapter);

            maxDedicatedVRAM = desc.DedicatedVideoMemory;
            // ++ Also check if the adapter supports min feature level
            HR_CHECK(D3D12CreateDevice((IUnknown*) adapter4, min_feat_level, IID_PPV_ARGS_C(&IID_ID3D12Device, NULL)));
            best_adapter = adapter4;
            break;
        }

        // Fallback - keep first hardware adapter if no discrete GPU is found
        if (!best_adapter) {
            best_adapter = adapter4;
        } else {
            adapter4->lpVtbl->Release(adapter4);
        }
    }

    if (!best_adapter) {
        LOG_ERROR("[DX12] No suitable GPU found for D3D12");
        return NULL;
    } else {
        LOG_INFO("Selected Adapter Info: ");
        DXGI_ADAPTER_DESC3 adapterDesc = {0};
        best_adapter->lpVtbl->GetDesc3(best_adapter, &adapterDesc);
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

    device->lpVtbl->CheckFeatureSupport(device, D3D12_FEATURE_D3D12_OPTIONS, &f->options, sizeof(f->options));
    device->lpVtbl->CheckFeatureSupport(device, D3D12_FEATURE_D3D12_OPTIONS1, &f->options1, sizeof(f->options1));
    device->lpVtbl->CheckFeatureSupport(device, D3D12_FEATURE_D3D12_OPTIONS5, &f->options5, sizeof(f->options5));
    device->lpVtbl->CheckFeatureSupport(device, D3D12_FEATURE_D3D12_OPTIONS10, &f->options10, sizeof(f->options10));
    device->lpVtbl->CheckFeatureSupport(device, D3D12_FEATURE_D3D12_OPTIONS12, &f->options12, sizeof(f->options12));

    f->architecture.NodeIndex = 0;
    device->lpVtbl->CheckFeatureSupport(device, D3D12_FEATURE_ARCHITECTURE1, &f->architecture, sizeof(f->architecture));
    f->isUMA              = f->architecture.UMA;
    f->isCacheCoherentUMA = f->architecture.CacheCoherentUMA;

    f->shaderModel.HighestShaderModel = D3D_SHADER_MODEL_6_7;
    if (FAILED(device->lpVtbl->CheckFeatureSupport(device, D3D12_FEATURE_SHADER_MODEL, &f->shaderModel, sizeof(f->shaderModel)))) {
        f->shaderModel.HighestShaderModel = D3D_SHADER_MODEL_6_0;
    }

    f->rootSig.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;
    if (FAILED(device->lpVtbl->CheckFeatureSupport(device, D3D12_FEATURE_ROOT_SIGNATURE, &f->rootSig, sizeof(f->rootSig)))) {
        f->rootSig.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
    }

    device->lpVtbl->CheckFeatureSupport(device, D3D12_FEATURE_GPU_VIRTUAL_ADDRESS_SUPPORT, &f->vaSupport, sizeof(f->vaSupport));

    f->nodeCount = device->lpVtbl->GetNodeCount(device);
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
        backend->d3d12_debug->lpVtbl->EnableDebugLayer(backend->d3d12_debug);
        backend->d3d12_debug->lpVtbl->SetEnableGPUBasedValidation(backend->d3d12_debug, TRUE);
        LOG_INFO("D3D12 debug layer and GPU-based validation enabled");
    } else {
        LOG_WARN("D3D12 debug interface not available. Debug layer not enabled.");
    }
}

static void dx12_internal_d3d12_register_info_queue(context_backend* backend)
{
    if (!backend->device) {
        LOG_ERROR("D3D12 device is NULL; can't register info queue.");
        return;
    }

    // This increases the device refcount.
    if (SUCCEEDED(backend->device->lpVtbl->QueryInterface(backend->device, &IID_ID3D12InfoQueue, (void**) &backend->d3d12_info_queue))) {
        ID3D12InfoQueue* q = backend->d3d12_info_queue;

        q->lpVtbl->SetBreakOnSeverity(q, D3D12_MESSAGE_SEVERITY_ERROR, TRUE);
        q->lpVtbl->SetBreakOnSeverity(q, D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE);

        LOG_INFO("D3D12 info queue registered and debug message filters installed");
    } else {
        LOG_WARN("Failed to query ID3D12InfoQueue interface. Validation messages will not be captured.");
    }
}

static void dx12_internal_dxgi_register_info_queue(context_backend* backend)
{
    if (SUCCEEDED(DXGIGetDebugInterface1(0, &IID_IDXGIInfoQueue, (void**) &backend->dxgi_info_queue))) {
        IDXGIInfoQueue* q = backend->dxgi_info_queue;

        q->lpVtbl->SetBreakOnSeverity(q, DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_ERROR, TRUE);
        q->lpVtbl->SetBreakOnSeverity(q, DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_CORRUPTION, TRUE);

        LOG_INFO("DXGI debug info queue registered");
    } else {
        LOG_WARN("DXGI info queue not available. No message filtering.");
    }
}

static void dx12_internal_track_dxgi_liveobjects(context_backend* backend)
{
    LOG_WARN("Tracking live DXGI objects. This will report all live objects at the end of the program.");
    if (SUCCEEDED(DXGIGetDebugInterface1(0, &IID_IDXGIDebug, (void**) &backend->dxgi_debug))) {
        backend->dxgi_debug->lpVtbl->ReportLiveObjects(
            backend->dxgi_debug,
            DXGI_DEBUG_ALL,
            (DXGI_DEBUG_RLO_FLAGS) (DXGI_DEBUG_RLO_DETAIL | DXGI_DEBUG_RLO_SUMMARY | DXGI_DEBUG_RLO_IGNORE_INTERNAL));
        LOG_INFO("DXGI live object report completed");
        backend->dxgi_debug->lpVtbl->Release(backend->dxgi_debug);
    }
}

static void dx12_internal_destroy_debug_handles(context_backend* backend)
{
    if (backend->d3d12_debug)
        backend->d3d12_debug->lpVtbl->Release(backend->d3d12_debug);

    if (backend->d3d12_info_queue)
        backend->d3d12_info_queue->lpVtbl->Release(backend->d3d12_info_queue);

    if (backend->dxgi_info_queue)
        backend->dxgi_info_queue->lpVtbl->Release(backend->dxgi_info_queue);
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
    HRESULT hr = CreateDXGIFactory2(createFactoryFlags, IID_PPV_ARGS_C(&IID_IDXGIFactory7, &backend->factory));
    if (FAILED(hr)) {
        LOG_ERROR("Failed to create DXGI Factory7 (HRESULT = 0x%08X)", (unsigned int) hr);
        uuid_destroy(&ctx.uuid);
        free(backend);
        return ctx;
    }

    backend->feat_level = D3D_FEATURE_LEVEL_12_0;

    backend->gpu = dx12_internal_select_best_adapter(backend->factory, backend->feat_level);

    // Now create the logical device from the Adapter
    if (!backend->gpu) {
        LOG_ERROR("Failed to select a suitable GPU for D3D12");
        uuid_destroy(&ctx.uuid);
        free(backend);
        return ctx;
    }

    // Enable DX12 debug layer if available
    #ifdef _DEBUG
    dx12_internal_register_debug_interface(backend);
    #endif

    LOG_INFO("Creating D3D12 Device...");
    hr = D3D12CreateDevice((IUnknown*) backend->gpu, backend->feat_level, IID_PPV_ARGS_C(&IID_ID3D12Device10, &backend->device));
    if (FAILED(hr)) {
        LOG_ERROR("Failed to create D3D12 Device (HRESULT = 0x%08X)", (unsigned int) hr);
        backend->gpu->lpVtbl->Release(backend->gpu);
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
    backend->device->lpVtbl->CreateCommandQueue(backend->device, &desc, IID_PPV_ARGS_C(&IID_ID3D12CommandQueue, &backend->direct_queue));
    if (!backend->direct_queue) {
        LOG_ERROR("Failed to create D3D12 Direct Command Queue");
        dx12_ctx_destroy(&ctx);
        return ctx;
    }

    uuid_destroy(&ctx.uuid);
    return ctx;
}

void dx12_ctx_destroy(gfx_context* ctx)
{
    if (!ctx || !ctx->backend) {
        LOG_ERROR("Context or backend is NULL, cannot destroy D3D12 context");
        return;
    }
    context_backend* backend = (context_backend*) ctx->backend;

    if (backend->direct_queue) {
        backend->direct_queue->lpVtbl->Release(backend->direct_queue);
        backend->direct_queue = NULL;
    }

    if (backend->device) {
        backend->device->lpVtbl->Release(backend->device);
        backend->device = NULL;
    }

    if (backend->gpu) {
        backend->gpu->lpVtbl->Release(backend->gpu);
        backend->gpu = NULL;
    }

    if (backend->factory) {
        backend->factory->lpVtbl->Release(backend->factory);
        backend->factory = NULL;
    }

    #ifdef _DEBUG
    dx12_internal_destroy_debug_handles(backend);
    dx12_internal_track_dxgi_liveobjects(backend);
    #endif

    uuid_destroy(&ctx->uuid);
}

void dx12_flush_gpu_work(void)
{
    // No exact equivalent, we need to do this on a per-queue basis
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
    HRESULT hr                             = s_DXCtx.device->lpVtbl->CreateDescriptorHeap(s_DXCtx.device, &rtvHeapDesc, IID_PPV_ARGS_C(&IID_ID3D12DescriptorHeap, &backend->rtv_heap));
    if (FAILED(hr)) {
        LOG_ERROR("Failed to create RTV Descriptor Heap (HRESULT = 0x%08X)", (unsigned int) hr);
        backend->swapchain->lpVtbl->Release(backend->swapchain);
        uuid_destroy(&sc->uuid);
        free(backend);
        return;
    }

    backend->image_count = MAX_DX_SWAPCHAIN_BUFFERS;
    backend->rtv_heap->lpVtbl->GetCPUDescriptorHandleForHeapStart(backend->rtv_heap, &backend->rtv_handle_start);

    for (uint32_t i = 0; i < MAX_DX_SWAPCHAIN_BUFFERS; i++) {
        hr = backend->swapchain->lpVtbl->GetBuffer(backend->swapchain, i, IID_PPV_ARGS_C(&IID_ID3D12Resource, &backend->backbuffers[i]));
        if (FAILED(hr)) {
            LOG_ERROR("Failed to get backbuffer %u from swapchain (HRESULT = 0x%08X)", i, (unsigned int) hr);
            for (uint32_t j = 0; j < i; j++) {
                if (backend->backbuffers[j]) {
                    backend->backbuffers[j]->lpVtbl->Release(backend->backbuffers[j]);
                }
            }
            backend->rtv_heap->lpVtbl->Release(backend->rtv_heap);
            backend->swapchain->lpVtbl->Release(backend->swapchain);
            uuid_destroy(&sc->uuid);
            free(backend);
            return;
        }

        // Create RTV for the back buffer
        D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = backend->rtv_handle_start;
        rtvHandle.ptr += i * s_DXCtx.device->lpVtbl->GetDescriptorHandleIncrementSize(s_DXCtx.device, D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
        s_DXCtx.device->lpVtbl->CreateRenderTargetView(s_DXCtx.device, backend->backbuffers[i], NULL, rtvHandle);
    }
}

static void dx12_internal_destroy_backbuffers(gfx_swapchain* sc)
{
    swapchain_backend* backend = (swapchain_backend*) sc->backend;

    if (backend->rtv_heap) {
        backend->rtv_heap->lpVtbl->Release(backend->rtv_heap);
        backend->rtv_heap = NULL;
    }

    for (uint32_t i = 0; i < backend->image_count; i++) {
        if (backend->backbuffers[i]) {
            backend->backbuffers[i]->lpVtbl->Release(backend->backbuffers[i]);
            backend->backbuffers[i] = NULL;
        }
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
        LOG_ERROR("error malloc swapchain_backend");
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
    HRESULT          hr         = s_DXCtx.factory->lpVtbl->CreateSwapChainForHwnd(s_DXCtx.factory, (IUnknown*) s_DXCtx.direct_queue, s_DXCtx.hwnd, &swapChainDesc, NULL, NULL, &swapchain1);
    if (FAILED(hr)) {
        LOG_ERROR("Failed to create DXGI Swapchain (HRESULT = 0x%08X)", (unsigned int) hr);
        uuid_destroy(&swapchain.uuid);
        free(backend);
        return swapchain;
    }
    swapchain1->lpVtbl->QueryInterface(swapchain1, IID_PPV_ARGS_C(&IID_IDXGISwapChain4, &backend->swapchain));
    if (!backend->swapchain) {
        LOG_ERROR("Failed to query IDXGISwapChain4 from IDXGISwapChain1");
        swapchain1->lpVtbl->Release(swapchain1);
        uuid_destroy(&swapchain.uuid);
        free(backend);
        return swapchain;
    }
    swapchain1->lpVtbl->Release(swapchain1);    // Release IDXGISwapChain1, we only need IDXGISwapChain4 now

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
            backend->swapchain->lpVtbl->Release(backend->swapchain);
            backend->swapchain = NULL;
            free(backend);
            sc->backend = NULL;
        }
    }
}

gfx_syncobj dx12_device_create_syncobj(gfx_syncobj_type type)
{
    gfx_syncobj syncobj = {0};
    uuid_generate(&syncobj.uuid);

    syncobj_backend* backend = malloc(sizeof(syncobj_backend));
    syncobj.type             = type;
    syncobj.wait_value       = 0;
    syncobj.backend          = backend;

    HRESULT hr = s_DXCtx.device->lpVtbl->CreateFence(s_DXCtx.device, 0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS_C(&IID_ID3D12Fence, &backend->fence));
    if (FAILED(hr)) {
        LOG_ERROR("Failed to query IDXGISwapChain4 from IDXGISwapChain1");
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

void dx12_device_destroy_syncobj(gfx_syncobj* syncobj)
{
    if (!uuid_is_null(&syncobj->uuid)) {
        uuid_destroy(&syncobj->uuid);
        syncobj_backend* backend = syncobj->backend;

        if (backend->fence) {
            backend->fence->lpVtbl->Release(backend->fence);
            backend->fence = NULL;

            if (syncobj->type == GFX_SYNCOBJ_TYPE_CPU)
                CloseHandle(backend->fenceEvent);

            free(backend);
            syncobj->backend = NULL;
        }
    }
}

#endif    // _WIN32