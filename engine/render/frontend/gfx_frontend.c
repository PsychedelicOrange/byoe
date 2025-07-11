#include "gfx_frontend.h"

#include "../core/logging/log.h"

#include "../backend/backend_dx12.h"
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
#ifdef _WIN32
    else if (g_current_api == D3D12) {
        g_rhi = dx12_jumptable;
    }
#endif
    else {
        LOG_ERROR("Unsupported RHI API: %d", g_current_api);
        return -1;
    }

    return Success;
}

void gfx_destroy(void)
{
    rhi_jumptable null = {0};
    g_rhi              = null;
}
