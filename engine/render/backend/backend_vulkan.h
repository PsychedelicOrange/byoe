#ifndef BACKEND_VULKAN_N
#define BACKEND_VULKAN_N

#include <stdbool.h>

#include "../frontend/gfx_frontend.h"

//------------------------------------------
// Context
//------------------------------------------
bool vulkan_ctx_init(uint32_t width, uint32_t height);

//------------------------------------------
// Debug
//------------------------------------------

//------------------------------------------
// Device
//------------------------------------------

//------------------------------------------
// RHI
//------------------------------------------
rhi_error_codes vulkan_draw();

#endif    // BACKEND_VULKAN_N