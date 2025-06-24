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
    #include <dxgi1_6.h>
    #include <d3d12.h>
#endif
// clang-format on

    #include <GLFW/glfw3.h>

    #include <inttypes.h>
    #include <stdlib.h>
    #include <string.h>    // memset

//--------------------------------------------------------
// TODO:
// - [ ] Use the IDXGIFactory7 to query for the best GPUa and create the IDXGIAdapter4 or use AgilitySDK with DeviceFactory
//   - [ ] Figure out how to replicate COM model in C, use lpvtable and manually release memory for ID3DXXXX/iDXGIXXXX objects
// - [ ] Create the device and cache the Features
// - [ ] Debug Layers
// - [ ] Create swapchain and Sync Primitives
// - [ ] Basic submit/present flow using Fence counting + gfx_syncobj
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

//--------------------------------------------------------
// Internal Types
//--------------------------------------------------------

typedef struct context_backend
{
    IDXGIFactory7*    factory;
    IDXGIAdapter4*    gpu;
    ID3D12Device10*   device;    // Win 11 latest, other latest version needs Agility SDK
    HWND              window;
    D3D_FEATURE_LEVEL feat_level;
} context_backend;

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

//--------------------------------------------------------

gfx_context dx12_ctx_init(GLFWwindow* window, uint32_t width, uint32_t height)
{
    (void) width;
    (void) height;
    (void) window;

    gfx_context ctx = {0};
    uuid_generate(&ctx.uuid);

    context_backend* backend = malloc(sizeof(context_backend));
    if (!backend) {
        LOG_ERROR("error malloc swapchain_backend");
        uuid_destroy(&ctx.uuid);
        return ctx;
    }
    memset(backend, 0, sizeof(context_backend));
    ctx.backend = backend;

    LOG_INFO("Creating DXGI Factory7");
    HRESULT hr = CreateDXGIFactory2(0, IID_PPV_ARGS_C(&IID_IDXGIFactory7, &backend->factory));
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

    LOG_INFO("Creating D3D12 Device");
    hr = D3D12CreateDevice((IUnknown*) backend->gpu, backend->feat_level, IID_PPV_ARGS_C(&IID_ID3D12Device10, &backend->device));
    if (FAILED(hr)) {
        LOG_ERROR("Failed to create D3D12 Device (HRESULT = 0x%08X)", (unsigned int) hr);
        backend->gpu->lpVtbl->Release(backend->gpu);
        uuid_destroy(&ctx.uuid);
        free(backend);
        return ctx;
    }

    LOG_SUCCESS("Created D3D12 Device!");

    // WIP!
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

    free(ctx->backend);
    ctx->backend = NULL;

    uuid_destroy(&ctx->uuid);
}

#endif    // _WIN32