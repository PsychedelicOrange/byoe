#ifndef RHI_H
#define RHI_H

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

extern gfx_context (*gfx_ctx_init)(GLFWwindow* window, uint32_t width, uint32_t height);
extern void        (*gfx_ctx_destroy)(gfx_context* ctx);

extern void (*gfx_flush_gpu_work)(void);

extern gfx_swapchain (*gfx_create_swapchain)(uint32_t width, uint32_t height);
extern void          (*gfx_destroy_swapchain)(gfx_swapchain* sc);

extern gfx_cmd_pool (*gfx_create_gfx_cmd_pool)(void);
extern void         (*gfx_destroy_gfx_cmd_pool)(gfx_cmd_pool*);

extern gfx_cmd_buf (*gfx_create_gfx_cmd_buf)(gfx_cmd_pool* pool);

extern gfx_frame_sync (*gfx_create_frame_sync)(void);
extern void           (*gfx_destroy_frame_sync)(gfx_frame_sync* in_flight_sync);

extern gfx_shader (*gfx_create_compute_shader)(const char* spv_file_path);
extern void       (*gfx_destroy_compute_shader)(gfx_shader* shader);

extern gfx_shader (*gfx_create_vs_ps_shader)(const char* spv_file_path_vs, const char* spv_file_path_ps);
extern void       (*gfx_destroy_vs_ps_shader)(gfx_shader* shader);

extern gfx_pipeline (*gfx_create_pipeline)(gfx_pipeline_create_info info);
extern void         (*gfx_destroy_pipeline)(gfx_pipeline* pipeline);

extern gfx_root_signature (*gfx_create_root_signature)(const gfx_descriptor_set_layout* set_layouts, uint32_t set_layout_count, const gfx_push_constant_range* push_constants, uint32_t push_constant_count);
extern void               (*gfx_destroy_root_signature)(gfx_root_signature* root_sig);

extern gfx_descriptor_table (*gfx_create_descriptor_table)(const gfx_root_signature* root_signature);
extern void                 (*gfx_destroy_descriptor_table)(gfx_descriptor_table* descriptor_table);
extern void                 (*gfx_update_descriptor_table)(gfx_descriptor_table* descriptor_table, gfx_descriptor_table_entry* entires, uint32_t num_entries);

extern gfx_resource (*gfx_create_texture_resource)(gfx_texture_create_info);
extern void         (*gfx_destroy_texture_resource)(gfx_resource* resource);

extern gfx_resource (*gfx_create_sampler)(gfx_sampler_create_info);
extern void         (*gfx_destroy_sampler)(gfx_resource* sampler);

extern gfx_resource (*gfx_create_uniform_buffer_resource)(uint32_t size);
extern void         (*gfx_destroy_uniform_buffer_resource)(gfx_resource* resource);
extern void         (*gfx_update_uniform_buffer)(gfx_resource* resource, uint32_t size, uint32_t offset, void* data);

extern gfx_resource_view (*gfx_create_texture_resource_view)(gfx_resource_view_create_info desc);
extern void              (*gfx_destroy_texture_resource_view)(gfx_resource_view* view);

extern gfx_resource_view (*gfx_create_sampler_resource_view)(gfx_resource_view_create_info desc);
extern void              (*gfx_destroy_sampler_resource_view)(gfx_resource_view* view);

extern gfx_resource_view (*gfx_create_uniform_buffer_resource_view)(gfx_resource* resource, uint32_t size, uint32_t offset);
extern void              (*gfx_destroy_uniform_buffer_resource_view)(gfx_resource_view* view);

extern gfx_cmd_buf (*gfx_create_single_time_cmd_buffer)(void);
extern void        (*gfx_destroy_single_time_cmd_buffer)(gfx_cmd_buf* cmd_buf);

extern gfx_texture_readback (*gfx_readback_swapchain)(const gfx_swapchain* swapchain);

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
extern gfx_frame_sync* (*rhi_frame_begin)(gfx_context* context);
extern rhi_error_codes (*rhi_frame_end)(gfx_context* context);
// Begin/End RenderPass
//---------------------------

extern rhi_error_codes (*rhi_wait_on_previous_cmds)(const gfx_frame_sync* in_flight_sync);
extern rhi_error_codes (*rhi_acquire_image)(gfx_swapchain* swapchain, const gfx_frame_sync* in_flight_sync);
extern rhi_error_codes (*rhi_gfx_cmd_enque_submit)(gfx_cmd_queue* cmd_queue, const gfx_cmd_buf* cmd_buff);
extern rhi_error_codes (*rhi_gfx_cmd_submit_queue)(const gfx_cmd_queue* cmd_queue, gfx_frame_sync* frame_sync);
extern rhi_error_codes (*rhi_present)(const gfx_swapchain* swapchain, const gfx_frame_sync* frame_sync);

extern rhi_error_codes (*rhi_resize_swapchain)(gfx_swapchain* swapchain, uint32_t width, uint32_t height);

extern rhi_error_codes (*rhi_begin_gfx_cmd_recording)(const gfx_cmd_buf* cmd_buf);
extern rhi_error_codes (*rhi_end_gfx_cmd_recording)(const gfx_cmd_buf* cmd_buf);

extern rhi_error_codes (*rhi_begin_render_pass)(const gfx_cmd_buf* cmd_buf, gfx_render_pass render_pass, uint32_t backbuffer_index);
extern rhi_error_codes (*rhi_end_render_pass)(const gfx_cmd_buf* cmd_buf, gfx_render_pass render_pass);

extern rhi_error_codes (*rhi_set_viewport)(const gfx_cmd_buf* cmd_buf, gfx_viewport viewport);
extern rhi_error_codes (*rhi_set_scissor)(const gfx_cmd_buf* cmd_buf, gfx_scissor scissor);

extern rhi_error_codes (*rhi_bind_gfx_pipeline)(const gfx_cmd_buf* cmd_buf, gfx_pipeline pipeline);
extern rhi_error_codes (*rhi_bind_compute_pipeline)(const gfx_cmd_buf* cmd_buf, gfx_pipeline pipeline);
extern rhi_error_codes (*rhi_bind_root_signature)(const gfx_cmd_buf* cmd_buf, const gfx_root_signature* root_signature);
extern rhi_error_codes (*rhi_bind_descriptor_table)(const gfx_cmd_buf* cmd_buf, const gfx_descriptor_table* descriptor_table, gfx_pipeline_type pipeline_type);
extern rhi_error_codes (*rhi_bind_push_constant)(const gfx_cmd_buf* cmd_buf, gfx_root_signature* root_sig, gfx_push_constant push_constant);

extern rhi_error_codes (*rhi_draw)(const gfx_cmd_buf* cmd_buf, uint32_t vertex_count, uint32_t instance_count, uint32_t first_vertex, uint32_t first_instance);
extern rhi_error_codes (*rhi_dispatch)(const gfx_cmd_buf* cmd_buf, uint32_t dimX, uint32_t dimY, uint32_t dimZ);

extern rhi_error_codes (*rhi_insert_image_layout_barrier)(const gfx_cmd_buf* cmd_buf, const gfx_resource* resource, gfx_image_layout old_layout, gfx_image_layout new_layout);

extern rhi_error_codes (*rhi_clear_image)(const gfx_cmd_buf* cmd_buffer, const gfx_resource* image);

uint32_t rhi_get_back_buffer_idx(const gfx_swapchain* swapchain);
uint32_t rhi_get_current_frame_idx(const gfx_context* ctx);
#endif    // RHI_H
