#include "gfx_frontend.h"

#include "../backend/backend_vulkan.h"

int gfx_init(rhi_api api, uint32_t width, uint32_t height)
{
    (void) width;
    (void) height;

    if (api == Vulkan) {
        vulkan_ctx_init(width, height);
        rhi_clear = vulkan_draw;
    }

    return Success;
}

void gfx_destroy()
{
}
