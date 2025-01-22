#ifndef BACKEND_VULKAN_N
#define BACKEND_VULKAN_N

#include <stdbool.h>

#include "../frontend/gfx_frontend.h"

//------------------------------------------
// Context
//------------------------------------------
bool vulkan_ctx_init(GLFWwindow* window, uint32_t width, uint32_t height);
void vulkan_ctx_destroy(void);

//------------------------------------------
// Debug
//------------------------------------------

//------------------------------------------
// Device
//------------------------------------------
gfx_swapchain vulkan_device_create_swapchain(uint32_t width, uint32_t height);
void          vulkan_device_destroy_swapchain(gfx_swapchain sc);

gfx_cmd_pool vulkan_device_create_gfx_cmd_pool(void);
void         vulkan_device_destroy_gfx_cmd_pool(gfx_cmd_pool pool);
//------------------------------------------
// RHI
//------------------------------------------
rhi_error_codes vulkan_draw(void);

#endif    // BACKEND_VULKAN_N