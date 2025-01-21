#include "gfx_frontend.h"

#include "../core/logging/log.h"

#include "../backend/backend_vulkan.h"

static rhi_api s_CurrentAPI;

int gfx_init(rhi_api api, GLFWwindow* window, uint32_t width, uint32_t height)
{
    (void) width;
    (void) height;
    s_CurrentAPI = api;

    if (s_CurrentAPI == Vulkan) {
        bool success = vulkan_ctx_init(window, width, height);
        if (!success) {
            LOG_ERROR("Failed to create vulkan backend!");
            return FailedUnknown;
        }
        rhi_clear = vulkan_draw;
    }

    return Success;
}

void gfx_destroy(void)
{
    if (s_CurrentAPI == Vulkan) {
        vulkan_ctx_destroy();
    }
}
