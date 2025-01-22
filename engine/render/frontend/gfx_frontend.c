#include "gfx_frontend.h"

#include "../core/logging/log.h"

#include "../backend/backend_vulkan.h"

static rhi_api s_CurrentAPI;

int gfx_init(rhi_api api)
{
    s_CurrentAPI = api;

    if (s_CurrentAPI == Vulkan) {
        // Context and Device
        gfx_ctx_init    = vulkan_ctx_init;
        gfx_ctx_destroy = vulkan_ctx_destroy;

        gfx_create_swapchain  = vulkan_device_create_swapchain;
        gfx_destroy_swapchain = vulkan_device_destroy_swapchain;

        gfx_create_gfx_cmd_pool  = vulkan_device_create_gfx_cmd_pool;
        gfx_destroy_gfx_cmd_pool = vulkan_device_destroy_gfx_cmd_pool;

        // RHI
        rhi_clear = vulkan_draw;
    }

    return Success;
}

void gfx_destroy(void)
{
}

void gfx_ctx_ignite(gfx_context* ctx)
{
    // create sync prims, command pool and buffers per thread (in this case only 1)
    ctx->frame_sync = gfx_create_frame_syncs(MAX_FRAME_INFLIGHT);

}
