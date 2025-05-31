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
}

void gfx_util_ignite_init_resources(gfx_context* ctx)
{
    // command pool: only per thread and 2 in-flight command buffers per pool
    ctx->draw_cmds_pool = g_rhi.create_gfx_cmd_pool();
    for (uint32_t i = 0; i < MAX_FRAME_INFLIGHT; i++) {
        ctx->frame_sync[i] = g_rhi.create_frame_sync();

        // Allocate and create the command buffers
        ctx->draw_cmds[i] = g_rhi.create_gfx_cmd_buf(&ctx->draw_cmds_pool);
    }
}

void gfx_util_recreate_frame_sync(gfx_context* ctx)
{
    for (uint32_t i = 0; i < MAX_FRAME_INFLIGHT; i++) {
        g_rhi.destroy_frame_sync(&ctx->frame_sync[i]);
        ctx->frame_sync[i] = g_rhi.create_frame_sync();
    }
}

uint32_t gfx_util_get_back_buffer_idx(const gfx_swapchain* swapchain)
{
    return swapchain->current_backbuffer_idx;
}

uint32_t gfx_util_get_current_frame_idx(const gfx_context* ctx)
{
    return ctx->frame_idx;
}
