#ifndef BACKEND_VULKAN_N
#define BACKEND_VULKAN_N

#include <stdbool.h>

#include "../frontend/gfx_frontend.h"

//------------------------------------------
// Context
//------------------------------------------
gfx_context vulkan_ctx_init(GLFWwindow* window, uint32_t width, uint32_t height);
void        vulkan_ctx_destroy(gfx_context* ctx);

//void vulkan_ctx_ignite(gfx_context* context);

//------------------------------------------
// Debug
//------------------------------------------

//------------------------------------------
// Device
//------------------------------------------
// https://www.sctheblog.com/blog/vulkan-synchronization/
// https://www.khronos.org/blog/vulkan-timeline-semaphores
//gfx_fence vulkan_device_create_timeline_semaphores();

gfx_swapchain vulkan_device_create_swapchain(uint32_t width, uint32_t height);
void          vulkan_device_destroy_swapchain(gfx_swapchain* sc);

gfx_cmd_pool vulkan_device_create_gfx_cmd_pool(void);
void         vulkan_device_destroy_gfx_cmd_pool(gfx_cmd_pool* pool);

gfx_cmd_buf vulkan_create_gfx_cmd_buf(gfx_cmd_pool* pool);

gfx_frame_sync vulkan_device_create_frame_sync();
void           vulkan_device_destroy_frame_sync(gfx_frame_sync* frame_sync);
//------------------------------------------
// RHI
//------------------------------------------

gfx_frame_sync* vulkan_frame_begin(gfx_context* context);
// Begin/End RenderPass
rhi_error_codes vulkan_frame_end(gfx_context* context);

rhi_error_codes vulkan_wait_on_previous_cmds(const gfx_frame_sync* in_flight_sync);
rhi_error_codes vulkan_acquire_image(gfx_swapchain* swapchain, const gfx_frame_sync* in_flight_sync);
rhi_error_codes vulkan_gfx_cmd_enque_submit(gfx_cmd_queue* cmd_queue, gfx_cmd_buf* cmd_buff);
rhi_error_codes vulkan_gfx_cmd_submit_queue(gfx_cmd_queue* cmd_queue, gfx_frame_sync* frame_sync);
rhi_error_codes vulkan_present(gfx_swapchain* swapchain, gfx_frame_sync* frame_sync);

rhi_error_codes vulkan_resize_swapchain(gfx_swapchain* swapchain, uint32_t width, uint32_t height);

rhi_error_codes vulkan_begin_gfx_cmd_recording(gfx_cmd_buf* cmd_buf);
rhi_error_codes vulkan_end_gfx_cmd_recording(gfx_cmd_buf* cmd_buf);

rhi_error_codes vulkan_draw(void);

#endif    // BACKEND_VULKAN_N