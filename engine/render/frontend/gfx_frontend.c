#include "gfx_frontend.h"

#include "../backend/backend_vulkan.h"

//---------------------------
rhi_api g_current_api = Vulkan;    // Default Value
//---------------------------
rhi_jumptable g_rhi;
//---------------------------
gfx_config g_gfxConfig = {
    .use_timeline_semaphores = false,
};
//---------------------------

int gfx_init(rhi_api api)
{
    g_current_api = api;

    if (g_current_api == Vulkan) {
        g_rhi = vulkan_jumptable;
    }

    return Success;
}

void gfx_destroy(void)
{
    rhi_jumptable null = {0};
    g_rhi = null;
}

uint32_t gfx_util_get_back_buffer_idx(const gfx_swapchain* swapchain)
{
    return swapchain->current_backbuffer_idx;
}

uint32_t gfx_util_get_current_inflight_idx(const gfx_context* ctx)
{
    return ctx->inflight_frame_idx;
}
