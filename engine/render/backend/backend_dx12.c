#include "backend_dx12.h"

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
// - [ ] Use the IDXGIFactory7 to query for the best GPUa dn create the IDXGIAdapter or use AgilitySDK with DeviceFactory
//   - [ ] Figure out how to replicate COM model in C, use lpvtable and manually release memory for ID3DXXXX/iDXGIXXXX objects
// - [ ] Create the device and cache the Features
// - [ ] Debug Layers
// - [ ] Create swapchain and Sync Primitives
// - [ ] Basic submit/present flow using Fence counting + gfx_syncobj
// - [ ] Hello Trianle without VB/IB in single shader quad
//   - [ ] Drawing API
//   - [ ] Shaders/Loading
//   - [ ] Pipelien API
//   - [ ] Root Signature
//   - [ ] Hello Triangle Shader + Drawloop 
// - [ ] Descriptor Heaps API
// - [ ] Create Texture/Buffer resources
// - [ ] Resource Views API
// - [ ] Barriers API
// - [ ] Root Contants
// - [ ] Dispatch API
// - Restore SDF renderer


//--------------------------------------------------------

const rhi_jumptable dx12_jumptable = {0};

//--------------------------------------------------------
// Internal Types
//--------------------------------------------------------

typedef struct context_backend 
{
    IDXGIFactory7* factory;
    IDXGIAdapter4* gpu;
    ID3D12Device8* device;
} context_backend;

//--------------------------------------------------------

gfx_context dx12_ctx_init(GLFWwindow *window, uint32_t width, uint32_t height)
{
    (void) width;
    (void) height;

    gfx_context ctx = {0};
    uuid_generate(&ctx.uuid);

    context_backend* backend = malloc(sizeof(context_backend));
    if (!backend) {
        LOG_ERROR("error malloc swapchain_backend");
        uuid_destroy(&ctx.uuid);
        return ctx;
    }
    memset(&backend, 0, sizeof(context_backend));
    ctx.backend = backend;

    LOG_INFO("Creating DXGI Factory7");
    


    return ctx;
}
