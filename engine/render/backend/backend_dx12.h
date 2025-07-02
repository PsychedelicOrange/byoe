#ifndef BACKEND_DX12_H
#define BACKEND_DX12_H

#include <stdbool.h>

#include "../frontend/gfx_frontend.h"

#ifdef _WIN32

//------------------------------------------
extern const rhi_jumptable dx12_jumptable;
//------------------------------------------

//------------------------------------------
// Context
//------------------------------------------
gfx_context dx12_ctx_init(GLFWwindow* window);
void        dx12_ctx_destroy(gfx_context* ctx);

void dx12_flush_gpu_work(gfx_context* context);

//------------------------------------------
// Device
//------------------------------------------

gfx_swapchain dx12_create_swapchain(uint32_t width, uint32_t height);
void          dx12_destroy_swapchain(gfx_swapchain* swapchain);

gfx_syncobj dx12_create_syncobj(gfx_syncobj_type type);
void        dx12_destroy_syncobj(gfx_syncobj* syncobj);

gfx_cmd_pool dx12_create_gfx_cmd_allocator(void);
void         dx12_destroy_gfx_cmd_allocator(gfx_cmd_pool* pool);

gfx_cmd_buf dx12_create_gfx_cmd_buf(gfx_cmd_pool* pool);
void        dx12_free_gfx_cmd_buf(gfx_cmd_buf* cmd_buf);

gfx_shader dx12_create_compute_shader(const char* cso_file_path);
void       dx12_destroy_compute_shader(gfx_shader* shader);

gfx_shader dx12_create_vs_ps_shader(const char* cso_file_path_vs, const char* cso_file_path_ps);
void       dx12_destroy_vs_ps_shader(gfx_shader* shader);

gfx_root_signature dx12_create_root_signature(const gfx_descriptor_set_layout* set_layouts, uint32_t set_layout_count, const gfx_push_constant_range* push_constants, uint32_t push_constant_count);
void               dx12_destroy_root_signature(gfx_root_signature* root_sig);

gfx_pipeline dx12_create_pipeline(gfx_pipeline_create_info info);
void         dx12_destroy_pipeline(gfx_pipeline* pipeline);

// ...

//------------------------------------------
// RHI
//------------------------------------------

rhi_error_codes dx12_frame_begin(gfx_context* context);
rhi_error_codes dx12_frame_end(gfx_context* context);

rhi_error_codes dx12_wait_on_previous_cmds(const gfx_syncobj* in_flight_sync, gfx_sync_point sync_point);
rhi_error_codes dx12_acquire_image(gfx_context* context);
rhi_error_codes dx12_gfx_cmd_enque_submit(gfx_cmd_queue* cmd_queue, gfx_cmd_buf* cmd_buff);
rhi_error_codes dx12_gfx_cmd_submit_queue(const gfx_cmd_queue* cmd_queue, gfx_submit_syncobj submit_sync);
rhi_error_codes dx12_gfx_cmd_submit_for_rendering(gfx_context* context);
rhi_error_codes dx12_present(const gfx_context* context);

rhi_error_codes dx12_resize_swapchain(gfx_context* context, uint32_t width, uint32_t height);

rhi_error_codes dx12_begin_gfx_cmd_recording(const gfx_cmd_pool* allocator, const gfx_cmd_buf* cmd_buf);
rhi_error_codes dx12_end_gfx_cmd_recording(const gfx_cmd_buf* cmd_buf);

rhi_error_codes dx12_begin_render_pass(const gfx_cmd_buf* cmd_buf, gfx_render_pass render_pass, uint32_t backbuffer_index);
rhi_error_codes dx12_end_render_pass(const gfx_cmd_buf* cmd_buf, gfx_render_pass render_pass);

// ...

rhi_error_codes dx12_draw(const gfx_cmd_buf* cmd_buf, uint32_t vertex_count, uint32_t instance_count, uint32_t first_vertex, uint32_t first_instance);
rhi_error_codes dx12_dispatch(const gfx_cmd_buf* cmd_buf, uint32_t dimX, uint32_t dimY, uint32_t dimZ);

rhi_error_codes dx12_transition_image_layout(const gfx_cmd_buf* cmd_buffer, const gfx_resource* image, gfx_image_layout old_layout, gfx_image_layout new_layout);
rhi_error_codes dx12_transition_swapchain_layout(const gfx_cmd_buf* cmd_buffer, const gfx_swapchain* swapchain, gfx_image_layout old_layout, gfx_image_layout new_layout);

    // ...

#endif    // _WIN32
#endif    // BACKEND_DX12_H
