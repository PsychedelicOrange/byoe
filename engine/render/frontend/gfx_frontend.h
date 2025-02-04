#ifndef RHI_H
#define RHI_H

// TODO FUTURE: separate it into high level: frontend/backend and low level:frontend/backend --> 4 files

#include "../render/render_structs.h"

#include <stdint.h>

#define DEFAULT_WIDTH  1024
#define DEFAULT_HEIGHT 1024

typedef struct GLFWwindow GLFWwindow;

typedef enum rhi_api
{
    Vulkan,
    D3D12
} rhi_api;

//------------------------------------------
// Context
//------------------------------------------

int  gfx_init(rhi_api api);
void gfx_destroy(void);
void gfx_ctx_ignite(gfx_context* ctx);
void gfx_ctx_recreate_frame_sync(gfx_context* ctx);

gfx_context (*gfx_ctx_init)(GLFWwindow* window, uint32_t width, uint32_t height);
void (*gfx_ctx_destroy)(gfx_context* ctx);

void (*gfx_flush_gpu_work)(void);

gfx_swapchain (*gfx_create_swapchain)(uint32_t width, uint32_t height);
void (*gfx_destroy_swapchain)(gfx_swapchain* sc);

gfx_cmd_pool (*gfx_create_gfx_cmd_pool)(void);
void (*gfx_destroy_gfx_cmd_pool)(gfx_cmd_pool*);

gfx_cmd_buf (*gfx_create_gfx_cmd_buf)(gfx_cmd_pool* pool);

gfx_frame_sync (*gfx_create_frame_sync)(void);
void (*gfx_destroy_frame_sync)(gfx_frame_sync* in_flight_sync);

gfx_shader (*gfx_create_compute_shader)(const char* spv_file_path);
void (*gfx_destroy_compute_shader)(gfx_shader* shader);

gfx_shader (*gfx_create_vs_ps_shader)(const char* spv_file_path_vs, const char* spv_file_path_ps);
void (*gfx_destroy_vs_ps_shader)(gfx_shader* shader);

gfx_pipeline (*gfx_create_pipeline)(gfx_pipeline_create_info info);
void (*gfx_destroy_pipeline)(gfx_pipeline* pipeline);

gfx_root_signature (*gfx_create_root_signature)(const gfx_descriptor_set_layout* set_layouts, uint32_t set_layout_count, const gfx_push_constant_range* push_constants, uint32_t push_constant_count);
void (*gfx_destroy_root_signature)(gfx_root_signature* root_sig);

gfx_descriptor_table (*gfx_create_descriptor_table)(const gfx_root_signature* root_signature);
void (*gfx_destroy_descriptor_table)(gfx_descriptor_table* descriptor_table);
void (*gfx_update_descriptor_table)(gfx_descriptor_table* descriptor_table, gfx_descriptor_table_entry* entires, uint32_t num_entries);

gfx_resource (*gfx_create_texture_resource)(gfx_texture_create_info desc);
void (*gfx_destroy_texture_resource)(gfx_resource* resource);

gfx_resource (*gfx_create_sampler)(gfx_sampler_create_info desc);
void (*gfx_destroy_sampler)(gfx_resource* sampler);

gfx_resource (*gfx_create_uniform_buffer_resource)(uint32_t size);
void (*gfx_destroy_uniform_buffer_resource)(gfx_resource* resource);
void (*gfx_update_uniform_buffer)(gfx_resource* resource, uint32_t size, uint32_t offset, void* data);

gfx_resource_view (*gfx_create_texture_resource_view)(gfx_resource_view_create_info desc);
void (*gfx_destroy_texture_resource_view)(gfx_resource_view* view);

gfx_resource_view (*gfx_create_sampler_resource_view)(gfx_resource_view_create_info desc);
void (*gfx_destroy_sampler_resource_view)(gfx_resource_view* view);

gfx_resource_view (*gfx_create_uniform_buffer_resource_view)(gfx_resource* resource, uint32_t size, uint32_t offset);
void (*gfx_destroy_uniform_buffer_resource_view)(gfx_resource_view* view);

gfx_cmd_buf (*gfx_create_single_time_cmd_buffer)(void);
void (*gfx_destroy_single_time_cmd_buffer)(gfx_cmd_buf* cmd_buf);

//------------------------------------------
// RHI function pointers
//------------------------------------------
typedef enum rhi_error_codes
{
    Success,
    FailedUnknown,
    FailedMemoryAlloc,
    FailedSwapAcquire,
    FailedHandleCreation,
    FailedMemoryBarrier
} rhi_error_codes;
//---------------------------
// High-level API
gfx_frame_sync* (*rhi_frame_begin)(gfx_context* context);
rhi_error_codes (*rhi_frame_end)(gfx_context* context);
// Begin/End RenderPass
//---------------------------
// TODO: Make all gfx_cmf_buf* consts

rhi_error_codes (*rhi_wait_on_previous_cmds)(const gfx_frame_sync* in_flight_sync);
rhi_error_codes (*rhi_acquire_image)(gfx_swapchain* swapchain, const gfx_frame_sync* in_flight_sync);
rhi_error_codes (*rhi_gfx_cmd_enque_submit)(gfx_cmd_queue* cmd_queue, const gfx_cmd_buf* cmd_buff);
rhi_error_codes (*rhi_gfx_cmd_submit_queue)(const gfx_cmd_queue* cmd_queue, gfx_frame_sync* frame_sync);
rhi_error_codes (*rhi_present)(const gfx_swapchain* swapchain, const gfx_frame_sync* frame_sync);

rhi_error_codes (*rhi_resize_swapchain)(gfx_swapchain* swapchain, uint32_t width, uint32_t height);

rhi_error_codes (*rhi_begin_gfx_cmd_recording)(const gfx_cmd_buf* cmd_buf);
rhi_error_codes (*rhi_end_gfx_cmd_recording)(const gfx_cmd_buf* cmd_buf);

rhi_error_codes (*rhi_begin_render_pass)(const gfx_cmd_buf* cmd_buf, gfx_render_pass render_pass, uint32_t backbuffer_index);
rhi_error_codes (*rhi_end_render_pass)(const gfx_cmd_buf* cmd_buf, gfx_render_pass render_pass);

rhi_error_codes (*rhi_set_viewport)(const gfx_cmd_buf* cmd_buf, gfx_viewport viewport);
rhi_error_codes (*rhi_set_scissor)(const gfx_cmd_buf* cmd_buf, gfx_scissor scissor);

rhi_error_codes (*rhi_bind_gfx_pipeline)(const gfx_cmd_buf* cmd_buf, gfx_pipeline pipeline);
rhi_error_codes (*rhi_bind_compute_pipeline)(const gfx_cmd_buf* cmd_buf, gfx_pipeline pipeline);
rhi_error_codes (*rhi_bind_root_signature)(const gfx_cmd_buf* cmd_buf, const gfx_root_signature* root_signature);
rhi_error_codes (*rhi_bind_descriptor_table)(const gfx_cmd_buf* cmd_buf, const gfx_descriptor_table* descriptor_table, gfx_pipeline_type pipeline_type);
rhi_error_codes (*rhi_bind_push_constant)(const gfx_cmd_buf* cmd_buf, gfx_root_signature* root_sig, gfx_push_constant push_constant);

rhi_error_codes (*rhi_draw)(const gfx_cmd_buf* cmd_buf, uint32_t vertex_count, uint32_t instance_count, uint32_t first_vertex, uint32_t first_instance);
rhi_error_codes (*rhi_dispatch)(const gfx_cmd_buf* cmd_buf, uint32_t dimX, uint32_t dimY, uint32_t dimZ);

rhi_error_codes (*rhi_insert_image_layout_barrier)(const gfx_cmd_buf* cmd_buf, const gfx_resource* resource, gfx_image_layout old_layout, gfx_image_layout new_layout);

uint32_t rhi_get_back_buffer_idx(const gfx_swapchain* swapchain);
uint32_t rhi_get_current_frame_idx(const gfx_context* ctx);
#endif    // RHI_H
