#ifndef BACKEND_VULKAN_N
#define BACKEND_VULKAN_N

#include <stdbool.h>

#include "../frontend/gfx_frontend.h"

//------------------------------------------
extern const rhi_jumptable vulkan_jumptable;
//------------------------------------------

//------------------------------------------
// Context
//------------------------------------------
gfx_context vulkan_ctx_init(GLFWwindow* window);
void        vulkan_ctx_destroy(gfx_context* ctx);

void vulkan_flush_gpu_work(gfx_context* context);

//------------------------------------------
// Debug
//------------------------------------------

//------------------------------------------
// Device
//------------------------------------------

gfx_swapchain vulkan_device_create_swapchain(uint32_t width, uint32_t height);
void          vulkan_device_destroy_swapchain(gfx_swapchain* sc);

gfx_syncobj vulkan_device_create_syncobj(gfx_syncobj_type type);
void        vulkan_device_destroy_syncobj(gfx_syncobj* syncobj);

gfx_cmd_pool vulkan_device_create_gfx_cmd_pool(void);
void         vulkan_device_destroy_gfx_cmd_pool(gfx_cmd_pool* pool);

gfx_cmd_buf vulkan_device_create_gfx_cmd_buf(gfx_cmd_pool* pool);
void        vulkan_device_free_gfx_cmd_buf(gfx_cmd_buf* cmd_buf);

// TODO: Use a Span DS to make this more intuitive
gfx_shader vulkan_device_create_compute_shader(const char* spv_file_path);
void       vulkan_device_destroy_compute_shader(gfx_shader* shader);

gfx_shader vulkan_device_create_vs_ps_shader(const char* spv_file_path_vs, const char* spv_file_path_ps);
void       vulkan_device_destroy_vs_ps_shader(gfx_shader* shader);

gfx_root_signature vulkan_device_create_root_signature(const gfx_descriptor_table_layout* set_layouts, uint32_t set_layout_count, const gfx_root_constant_range* push_constants, uint32_t push_constant_count);
void               vulkan_device_destroy_root_signature(gfx_root_signature* root_sig);

gfx_pipeline vulkan_device_create_pipeline(gfx_pipeline_create_info info);
void         vulkan_device_destroy_pipeline(gfx_pipeline* pipeline);

gfx_descriptor_heap vulkan_device_create_descriptor_heap(gfx_heap_type heap_type, uint32_t num_descriptors);
void                vulkan_device_destroy_descriptor_heap(gfx_descriptor_heap* heap);

gfx_descriptor_table vulkan_device_build_descriptor_table(const gfx_root_signature*, gfx_descriptor_heap* heap, gfx_descriptor_table_entry* entries, uint32_t num_entries);

gfx_resource vulkan_device_create_texture_resource(gfx_texture_create_info desc);
void         vulkan_device_destroy_texture_resource(gfx_resource* resource);

gfx_resource vulkan_device_create_sampler(gfx_sampler_create_info desc);
void         vulkan_device_destroy_sampler(gfx_resource* resource);

gfx_resource vulkan_device_create_uniform_buffer_resource(uint32_t size);
void         vulkan_device_destroy_uniform_buffer_resource(gfx_resource* resource);
void         vulkan_device_update_uniform_buffer(gfx_resource* resource, uint32_t size, uint32_t offset, void* data);

gfx_resource_view vulkan_device_create_texture_resource_view(const gfx_resource_view_create_info desc);
void              vulkan_device_destroy_texture_resource_view(gfx_resource_view* view);

gfx_resource_view vulkan_backend_create_sampler_resource_view(gfx_resource_view_create_info desc);
void              vulkan_backend_destroy_sampler_resource_view(gfx_resource_view* view);

gfx_resource_view vulkan_device_create_uniform_buffer_resource_view(gfx_resource* resource, uint32_t size, uint32_t offset);
void              vulkan_device_destroy_uniform_buffer_resource_view(gfx_resource_view* view);

gfx_cmd_buf vulkan_device_create_single_time_command_buffer(void);
void        vulkan_device_destroy_single_time_command_buffer(gfx_cmd_buf* cmd_buf);

gfx_texture_readback vulkan_device_readback_swapchain(const gfx_swapchain* swapchain);

//------------------------------------------
// RHI
//------------------------------------------

rhi_error_codes vulkan_frame_begin(gfx_context* context);
rhi_error_codes vulkan_frame_end(gfx_context* context);

rhi_error_codes vulkan_wait_on_previous_cmds(const gfx_syncobj* in_flight_sync, gfx_sync_point sync_point);
rhi_error_codes vulkan_acquire_image(gfx_context* context);
rhi_error_codes vulkan_gfx_cmd_enque_submit(gfx_cmd_queue* cmd_queue, gfx_cmd_buf* cmd_buff);
rhi_error_codes vulkan_gfx_cmd_submit_queue(const gfx_cmd_queue* cmd_queue, gfx_submit_syncobj submit_sync);
rhi_error_codes vulkan_gfx_cmd_submit_for_rendering(gfx_context* context);
rhi_error_codes vulkan_present(const gfx_context* context);

rhi_error_codes vulkan_resize_swapchain(gfx_context* context, uint32_t width, uint32_t height);

rhi_error_codes vulkan_begin_gfx_cmd_recording(const gfx_cmd_pool* allocator, const gfx_cmd_buf* cmd_buf);
rhi_error_codes vulkan_end_gfx_cmd_recording(const gfx_cmd_buf* cmd_buf);

rhi_error_codes vulkan_begin_render_pass(const gfx_cmd_buf* cmd_buf, gfx_render_pass render_pass, uint32_t backbuffer_index);
rhi_error_codes vulkan_end_render_pass(const gfx_cmd_buf* cmd_buf, gfx_render_pass render_pass);

rhi_error_codes vulkan_set_viewport(const gfx_cmd_buf* cmd_buf, gfx_viewport viewport);
rhi_error_codes vulkan_set_scissor(const gfx_cmd_buf* cmd_buf, gfx_scissor scissor);

rhi_error_codes vulkan_bind_gfx_pipeline(const gfx_cmd_buf* cmd_buf, const gfx_pipeline* pipeline);
rhi_error_codes vulkan_bind_compute_pipeline(const gfx_cmd_buf* cmd_buf, const gfx_pipeline* pipeline);
rhi_error_codes vulkan_device_bind_root_signature(const gfx_cmd_buf* cmd_buf, const gfx_root_signature* root_signature, gfx_pipeline_type pipeline_type);
rhi_error_codes vulkan_device_bind_descriptor_heaps(const gfx_cmd_buf* cmd_buf, const gfx_descriptor_heap* heaps, uint32_t num_heaps);
rhi_error_codes vulkan_device_bind_descriptor_tables(const gfx_cmd_buf* cmd_buf, const gfx_descriptor_table* tables, uint32_t num_tables, gfx_pipeline_type pipeline_type);
rhi_error_codes vulkan_device_bind_push_constant(const gfx_cmd_buf* cmd_buf, gfx_root_signature* root_sig, gfx_root_constant push_constant);

rhi_error_codes vulkan_draw(const gfx_cmd_buf* cmd_buf, uint32_t vertex_count, uint32_t instance_count, uint32_t first_vertex, uint32_t first_instance);
rhi_error_codes vulkan_dispatch(const gfx_cmd_buf* cmd_buf, uint32_t dimX, uint32_t dimY, uint32_t dimZ);

rhi_error_codes vulkan_transition_image_layout(const gfx_cmd_buf* cmd_buffer, const gfx_resource* image, gfx_image_layout old_layout, gfx_image_layout new_layout);
rhi_error_codes vulkan_transition_swapchain_layout(const gfx_cmd_buf* cmd_buffer, const gfx_swapchain* swapchain, gfx_image_layout old_layout, gfx_image_layout new_layout);

rhi_error_codes vulkan_clear_image(const gfx_cmd_buf* cmd_buffer, const gfx_resource* image);

#endif    // BACKEND_VULKAN_N
