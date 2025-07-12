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

gfx_root_signature dx12_create_root_signature(const gfx_descriptor_table_layout* set_layouts, uint32_t set_layout_count, const gfx_root_constant_range* push_constants, uint32_t push_constant_count);
void               dx12_destroy_root_signature(gfx_root_signature* root_sig);

gfx_pipeline dx12_create_pipeline(gfx_pipeline_create_info info);
void         dx12_destroy_pipeline(gfx_pipeline* pipeline);

gfx_descriptor_heap dx12_create_descriptor_heap(gfx_resource_type res_type, uint32_t num_descriptors);
void                dx12_destroy_descriptor_heap(gfx_descriptor_heap* heap);

gfx_descriptor_table dx12_build_descriptor_table(const gfx_root_signature* root_sig, gfx_descriptor_heap* heap, gfx_descriptor_table_entry* entries, uint32_t num_entries);

gfx_resource dx12_create_texture_resource(gfx_texture_create_info desc);
void         dx12_destroy_texture_resource(gfx_resource* resource);

gfx_resource dx12_create_sampler(gfx_sampler_create_info desc);
void         dx12_destroy_sampler(gfx_resource* resource);

gfx_resource dx12_create_uniform_buffer_resource(uint32_t size);
void         dx12_destroy_uniform_buffer_resource(gfx_resource* resource);
void         dx12_update_uniform_buffer(gfx_resource* resource, uint32_t size, uint32_t offset, void* data);

gfx_resource_view dx12_create_texture_resource_view(const gfx_resource_view_create_info desc);
void              dx12_destroy_texture_resource_view(gfx_resource_view* view);

gfx_resource_view dx12d_create_sampler_resource_view(gfx_resource_view_create_info desc);
void              dx12d_destroy_sampler_resource_view(gfx_resource_view* view);

gfx_resource_view dx12_create_uniform_buffer_resource_view(gfx_resource* resource, uint32_t size, uint32_t offset);
void              dx12_destroy_uniform_buffer_resource_view(gfx_resource_view* view);

gfx_cmd_buf dx12_create_single_time_command_buffer(void);
void        dx12_destroy_single_time_command_buffer(gfx_cmd_buf* cmd_buf);

gfx_texture_readback dx12_readback_swapchain(const gfx_swapchain* swapchain);
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

rhi_error_codes dx12_set_viewport(const gfx_cmd_buf* cmd_buf, gfx_viewport viewport);
rhi_error_codes dx12_set_scissor(const gfx_cmd_buf* cmd_buf, gfx_scissor scissor);

rhi_error_codes dx12_bind_gfx_pipeline(const gfx_cmd_buf* cmd_buf, const gfx_pipeline* pipeline);
rhi_error_codes dx12_bind_compute_pipeline(const gfx_cmd_buf* cmd_buf, const gfx_pipeline* pipeline);
rhi_error_codes dx12_bind_root_signature(const gfx_cmd_buf* cmd_buf, const gfx_root_signature* root_signature, gfx_pipeline_type pipeline_type);
rhi_error_codes dx12_bind_descriptor_heaps(const gfx_cmd_buf* cmd_buf, const gfx_descriptor_heap* heaps, uint32_t num_heaps);
rhi_error_codes dx12_bind_descriptor_tables(const gfx_cmd_buf* cmd_buf, const gfx_descriptor_table* tables, uint32_t num_tables, gfx_pipeline_type pipeline_type);
rhi_error_codes dx12_bind_root_constant(const gfx_cmd_buf* cmd_buf, const gfx_root_signature* root_sig, gfx_root_constant push_constant);

rhi_error_codes dx12_draw(const gfx_cmd_buf* cmd_buf, uint32_t vertex_count, uint32_t instance_count, uint32_t first_vertex, uint32_t first_instance);
rhi_error_codes dx12_dispatch(const gfx_cmd_buf* cmd_buf, uint32_t dimX, uint32_t dimY, uint32_t dimZ);

rhi_error_codes dx12_transition_image_layout(const gfx_cmd_buf* cmd_buffer, const gfx_resource* image, gfx_image_layout old_layout, gfx_image_layout new_layout);
rhi_error_codes dx12_transition_swapchain_layout(const gfx_cmd_buf* cmd_buffer, const gfx_swapchain* swapchain, gfx_image_layout old_layout, gfx_image_layout new_layout);

rhi_error_codes dx12_clear_image(const gfx_cmd_buf* cmd_buf, const gfx_resource* image);

#endif    // _WIN32
#endif    // BACKEND_DX12_H
