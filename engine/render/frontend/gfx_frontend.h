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

//---------------------------
extern rhi_api g_current_api;
//---------------------------

typedef enum rhi_error_codes
{
    Success,
    FailedUnknown,
    FailedMemoryAlloc,
    FailedSwapAcquire,
    FailedPresent,
    FailedHandleCreation,
    FailedMemoryBarrier,
    FailedCommandAllocatorReset,
    FailedCommandBegin,
    FailedCommandEnd,
} rhi_error_codes;

//------------------------------------------
// Gfx RHI API
//------------------------------------------

int  gfx_init(rhi_api api);
void gfx_destroy(void);

typedef struct rhi_jumptablefrontend
{
    gfx_context (*ctx_init)(GLFWwindow*);
    void        (*ctx_destroy)(gfx_context*);
    void        (*flush_gpu_work)(gfx_context*);

    gfx_swapchain (*create_swapchain)(uint32_t, uint32_t);
    void          (*destroy_swapchain)(gfx_swapchain*);

    gfx_syncobj (*create_syncobj)(gfx_syncobj_type);
    void        (*destroy_syncobj)(gfx_syncobj*);

    gfx_cmd_pool (*create_gfx_cmd_pool)(void);
    void         (*destroy_gfx_cmd_pool)(gfx_cmd_pool*);

    gfx_cmd_buf (*create_gfx_cmd_buf)(gfx_cmd_pool*);
    void        (*free_gfx_cmd_buf)(gfx_cmd_buf*);

    gfx_shader (*create_compute_shader)(const char*);
    void       (*destroy_compute_shader)(gfx_shader*);

    gfx_shader (*create_vs_ps_shader)(const char*, const char*);
    void       (*destroy_vs_ps_shader)(gfx_shader*);

    gfx_root_signature (*create_root_signature)(const gfx_descriptor_set_layout*, uint32_t, const gfx_push_constant_range*, uint32_t);
    void               (*destroy_root_signature)(gfx_root_signature*);

    gfx_pipeline (*create_pipeline)(gfx_pipeline_create_info);
    void         (*destroy_pipeline)(gfx_pipeline*);

    gfx_descriptor_table (*create_descriptor_table)(const gfx_root_signature*);
    void                 (*destroy_descriptor_table)(gfx_descriptor_table*);
    void                 (*update_descriptor_table)(gfx_descriptor_table*, gfx_descriptor_table_entry*, uint32_t);

    gfx_resource (*create_texture_resource)(gfx_texture_create_info);
    void         (*destroy_texture_resource)(gfx_resource*);

    gfx_resource (*create_sampler)(gfx_sampler_create_info);
    void         (*destroy_sampler)(gfx_resource*);

    gfx_resource (*create_uniform_buffer_resource)(uint32_t);
    void         (*destroy_uniform_buffer_resource)(gfx_resource*);
    void         (*update_uniform_buffer)(gfx_resource*, uint32_t, uint32_t, void*);

    gfx_resource_view (*create_texture_resource_view)(gfx_resource_view_create_info);
    void              (*destroy_texture_resource_view)(gfx_resource_view*);

    gfx_resource_view (*create_sampler_resource_view)(gfx_resource_view_create_info);
    void              (*destroy_sampler_resource_view)(gfx_resource_view*);

    gfx_resource_view (*create_uniform_buffer_resource_view)(gfx_resource*, uint32_t, uint32_t);
    void              (*destroy_uniform_buffer_resource_view)(gfx_resource_view*);

    gfx_cmd_buf (*create_single_time_cmd_buffer)(void);
    void        (*destroy_single_time_cmd_buffer)(gfx_cmd_buf*);

    gfx_texture_readback (*readback_swapchain)(const gfx_swapchain*);

    rhi_error_codes (*frame_begin)(gfx_context*);
    rhi_error_codes (*frame_end)(gfx_context*);

    rhi_error_codes (*wait_on_previous_cmds)(const gfx_syncobj*, gfx_sync_point);
    rhi_error_codes (*acquire_image)(gfx_context*);
    rhi_error_codes (*gfx_cmd_enque_submit)(gfx_cmd_queue*, gfx_cmd_buf*);
    rhi_error_codes (*gfx_cmd_submit_queue)(const gfx_cmd_queue*, gfx_submit_syncobj);
    rhi_error_codes (*gfx_cmd_submit_for_rendering)(gfx_context*);
    rhi_error_codes (*present)(const gfx_context*);

    rhi_error_codes (*resize_swapchain)(gfx_context*, uint32_t, uint32_t);

    rhi_error_codes (*begin_gfx_cmd_recording)(const gfx_cmd_pool* allocator, const gfx_cmd_buf*);
    rhi_error_codes (*end_gfx_cmd_recording)(const gfx_cmd_buf*);

    rhi_error_codes (*begin_render_pass)(const gfx_cmd_buf*, gfx_render_pass, uint32_t);
    rhi_error_codes (*end_render_pass)(const gfx_cmd_buf*, gfx_render_pass);

    rhi_error_codes (*set_viewport)(const gfx_cmd_buf*, gfx_viewport);
    rhi_error_codes (*set_scissor)(const gfx_cmd_buf*, gfx_scissor);

    rhi_error_codes (*bind_gfx_pipeline)(const gfx_cmd_buf*, gfx_pipeline);
    rhi_error_codes (*bind_compute_pipeline)(const gfx_cmd_buf*, gfx_pipeline);
    rhi_error_codes (*bind_root_signature)(const gfx_cmd_buf*, const gfx_root_signature*);
    rhi_error_codes (*bind_descriptor_table)(const gfx_cmd_buf*, const gfx_descriptor_table*, gfx_pipeline_type);
    rhi_error_codes (*bind_push_constant)(const gfx_cmd_buf*, gfx_root_signature*, gfx_push_constant);

    rhi_error_codes (*draw)(const gfx_cmd_buf*, uint32_t, uint32_t, uint32_t, uint32_t);
    rhi_error_codes (*dispatch)(const gfx_cmd_buf*, uint32_t, uint32_t, uint32_t);

    rhi_error_codes (*insert_image_layout_barrier)(const gfx_cmd_buf*, const gfx_resource*, gfx_image_layout, gfx_image_layout);
    rhi_error_codes (*clear_image)(const gfx_cmd_buf*, const gfx_resource*);
} rhi_jumptable;

//---------------------------
extern rhi_jumptable g_rhi;
//---------------------------

#endif    // RHI_H
