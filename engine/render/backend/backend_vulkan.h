#ifndef BACKEND_VULKAN_N
#define BACKEND_VULKAN_N

#include <stdbool.h>

#include "../frontend/gfx_frontend.h"

//------------------------------------------
// Context
//------------------------------------------
gfx_context vulkan_ctx_init(GLFWwindow* window, uint32_t width, uint32_t height);
void        vulkan_ctx_destroy(gfx_context* ctx);

void vulkan_flush_gpu_work(void);

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

gfx_frame_sync vulkan_device_create_frame_sync(void);
void           vulkan_device_destroy_frame_sync(gfx_frame_sync* frame_sync);

// TODO: Use a Span DS to make this more intuitive
gfx_shader vulkan_device_create_compute_shader(const char* spv_file_path);
gfx_shader vulkan_device_create_vs_ps_shader(const char* spv_file_path_vs, const char* spv_file_path_ps);

void vulkan_device_destroy_compute_shader(gfx_shader* shader);
void vulkan_device_destroy_vs_ps_shader(gfx_shader* shader);

gfx_pipeline vulkan_device_create_pipeline(gfx_pipeline_create_info info);
void         vulkan_device_destroy_pipeline(gfx_pipeline* pipeline);

gfx_root_signature vulkan_device_create_root_signature(const gfx_descriptor_set_layout* set_layouts, uint32_t set_layout_count, const gfx_push_constant_range* push_constants, uint32_t push_constant_count);
void               vulkan_device_destroy_root_signature(gfx_root_signature* root_sig);

gfx_descriptor_table vulkan_device_create_descriptor_table(const gfx_root_signature* root_signature);
void                 vulkan_device_destroy_descriptor_table(gfx_descriptor_table* descriptor_table);
//void                 vulkan_device_update_descriptor_table(gfx_descriptor_table* descriptor_table, gfx_resource* resources, uint32_t num_resources);

//------------------------------------------
// RHI
//------------------------------------------

gfx_frame_sync* vulkan_frame_begin(gfx_context* context);
rhi_error_codes vulkan_frame_end(gfx_context* context);

rhi_error_codes vulkan_wait_on_previous_cmds(const gfx_frame_sync* in_flight_sync);
rhi_error_codes vulkan_acquire_image(gfx_swapchain* swapchain, const gfx_frame_sync* in_flight_sync);
rhi_error_codes vulkan_gfx_cmd_enque_submit(gfx_cmd_queue* cmd_queue, const gfx_cmd_buf* cmd_buff);
rhi_error_codes vulkan_gfx_cmd_submit_queue(const gfx_cmd_queue* cmd_queue, gfx_frame_sync* frame_sync);
rhi_error_codes vulkan_present(const gfx_swapchain* swapchain, const gfx_frame_sync* frame_sync);

rhi_error_codes vulkan_resize_swapchain(gfx_swapchain* swapchain, uint32_t width, uint32_t height);

rhi_error_codes vulkan_begin_gfx_cmd_recording(const gfx_cmd_buf* cmd_buf);
rhi_error_codes vulkan_end_gfx_cmd_recording(const gfx_cmd_buf* cmd_buf);

rhi_error_codes vulkan_begin_render_pass(const gfx_cmd_buf* cmd_buf, gfx_render_pass render_pass, uint32_t backbuffer_index);
rhi_error_codes vulkan_end_render_pass(const gfx_cmd_buf* cmd_buf);

rhi_error_codes vulkan_set_viewport(const gfx_cmd_buf* cmd_buf, gfx_viewport viewport);
rhi_error_codes vulkan_set_scissor(const gfx_cmd_buf* cmd_buf, gfx_scissor scissor);

rhi_error_codes vulkan_bind_pipeline(const gfx_cmd_buf* cmd_buf, gfx_pipeline pipeline);

rhi_error_codes vulkan_draw(const gfx_cmd_buf* cmd_buf, uint32_t vertex_count, uint32_t instance_count, uint32_t first_vertex, uint32_t first_instance);

#endif    // BACKEND_VULKAN_N