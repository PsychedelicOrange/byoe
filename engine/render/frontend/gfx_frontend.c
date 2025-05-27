#include "gfx_frontend.h"

#include "../backend/backend_vulkan.h"

static rhi_api s_CurrentAPI;

gfx_config g_gfxConfig = {
    .use_timeline_semaphores = false,
};

gfx_context (*gfx_ctx_init)(GLFWwindow* window, uint32_t width, uint32_t height);
void        (*gfx_ctx_destroy)(gfx_context* ctx);

void (*gfx_flush_gpu_work)(void);

gfx_swapchain (*gfx_create_swapchain)(uint32_t width, uint32_t height);
void          (*gfx_destroy_swapchain)(gfx_swapchain* sc);

gfx_cmd_pool (*gfx_create_gfx_cmd_pool)(void);
void         (*gfx_destroy_gfx_cmd_pool)(gfx_cmd_pool*);

gfx_cmd_buf (*gfx_create_gfx_cmd_buf)(gfx_cmd_pool* pool);

gfx_frame_sync (*gfx_create_frame_sync)(void);
void           (*gfx_destroy_frame_sync)(gfx_frame_sync* in_flight_sync);

gfx_shader (*gfx_create_compute_shader)(const char* spv_file_path);
void       (*gfx_destroy_compute_shader)(gfx_shader* shader);

gfx_shader (*gfx_create_vs_ps_shader)(const char* spv_file_path_vs, const char* spv_file_path_ps);
void       (*gfx_destroy_vs_ps_shader)(gfx_shader* shader);

gfx_pipeline (*gfx_create_pipeline)(gfx_pipeline_create_info info);
void         (*gfx_destroy_pipeline)(gfx_pipeline* pipeline);

gfx_root_signature (*gfx_create_root_signature)(const gfx_descriptor_set_layout* set_layouts, uint32_t set_layout_count, const gfx_push_constant_range* push_constants, uint32_t push_constant_count);
void               (*gfx_destroy_root_signature)(gfx_root_signature* root_sig);

gfx_descriptor_table (*gfx_create_descriptor_table)(const gfx_root_signature* root_signature);
void                 (*gfx_destroy_descriptor_table)(gfx_descriptor_table* descriptor_table);
void                 (*gfx_update_descriptor_table)(gfx_descriptor_table* descriptor_table, gfx_descriptor_table_entry* entires, uint32_t num_entries);

gfx_resource (*gfx_create_texture_resource)(gfx_texture_create_info);
void         (*gfx_destroy_texture_resource)(gfx_resource* resource);

gfx_resource (*gfx_create_sampler)(gfx_sampler_create_info);
void         (*gfx_destroy_sampler)(gfx_resource* sampler);

gfx_resource (*gfx_create_uniform_buffer_resource)(uint32_t size);
void         (*gfx_destroy_uniform_buffer_resource)(gfx_resource* resource);
void         (*gfx_update_uniform_buffer)(gfx_resource* resource, uint32_t size, uint32_t offset, void* data);

gfx_resource_view (*gfx_create_texture_resource_view)(gfx_resource_view_create_info desc);
void              (*gfx_destroy_texture_resource_view)(gfx_resource_view* view);

gfx_resource_view (*gfx_create_sampler_resource_view)(gfx_resource_view_create_info desc);
void              (*gfx_destroy_sampler_resource_view)(gfx_resource_view* view);

gfx_resource_view (*gfx_create_uniform_buffer_resource_view)(gfx_resource* resource, uint32_t size, uint32_t offset);
void              (*gfx_destroy_uniform_buffer_resource_view)(gfx_resource_view* view);

gfx_cmd_buf (*gfx_create_single_time_cmd_buffer)(void);
void        (*gfx_destroy_single_time_cmd_buffer)(gfx_cmd_buf* cmd_buf);

gfx_texture_readback (*gfx_readback_swapchain)(const gfx_swapchain* swapchain);

//-----------


 gfx_frame_sync* (*rhi_frame_begin)(gfx_context* context);
 rhi_error_codes (*rhi_frame_end)(gfx_context* context);
// Begin/End RenderPass
//---------------------------

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

rhi_error_codes (*rhi_clear_image)(const gfx_cmd_buf* cmd_buffer, const gfx_resource* image);



//----------

int gfx_init(rhi_api api)
{
    s_CurrentAPI = api;

    if (s_CurrentAPI == Vulkan) {
        // Context and Device
        gfx_ctx_init    = vulkan_ctx_init;
        gfx_ctx_destroy = vulkan_ctx_destroy;

        gfx_flush_gpu_work = vulkan_flush_gpu_work;

        gfx_create_swapchain  = vulkan_device_create_swapchain;
        gfx_destroy_swapchain = vulkan_device_destroy_swapchain;

        gfx_create_gfx_cmd_pool  = vulkan_device_create_gfx_cmd_pool;
        gfx_destroy_gfx_cmd_pool = vulkan_device_destroy_gfx_cmd_pool;

        gfx_create_gfx_cmd_buf = vulkan_device_create_gfx_cmd_buf;

        gfx_create_frame_sync  = vulkan_device_create_frame_sync;
        gfx_destroy_frame_sync = vulkan_device_destroy_frame_sync;

        gfx_create_compute_shader  = vulkan_device_create_compute_shader;
        gfx_destroy_compute_shader = vulkan_device_destroy_compute_shader;

        gfx_create_vs_ps_shader  = vulkan_device_create_vs_ps_shader;
        gfx_destroy_vs_ps_shader = vulkan_device_destroy_vs_ps_shader;

        gfx_create_pipeline  = vulkan_device_create_pipeline;
        gfx_destroy_pipeline = vulkan_device_destroy_pipeline;

        gfx_create_root_signature  = vulkan_device_create_root_signature;
        gfx_destroy_root_signature = vulkan_device_destroy_root_signature;

        gfx_create_descriptor_table  = vulkan_device_create_descriptor_table;
        gfx_destroy_descriptor_table = vulkan_device_destroy_descriptor_table;
        gfx_update_descriptor_table  = vulkan_device_update_descriptor_table;

        gfx_create_texture_resource  = vulkan_device_create_texture_resource;
        gfx_destroy_texture_resource = vulkan_device_destroy_texture_resource;

        gfx_create_sampler  = vulkan_device_create_sampler;
        gfx_destroy_sampler = vulkan_device_destroy_sampler;

        gfx_create_uniform_buffer_resource  = vulkan_device_create_uniform_buffer_resource;
        gfx_destroy_uniform_buffer_resource = vulkan_device_destroy_uniform_buffer_resource;
        gfx_update_uniform_buffer           = vulkan_device_update_uniform_buffer;

        gfx_create_texture_resource_view  = vulkan_device_create_texture_resource_view;
        gfx_destroy_texture_resource_view = vulkan_device_destroy_texture_resource_view;

        gfx_create_sampler_resource_view  = vulkan_backend_create_sampler_resource_view;
        gfx_destroy_sampler_resource_view = vulkan_backend_destroy_sampler_resource_view;

        gfx_create_uniform_buffer_resource_view  = vulkan_device_create_uniform_buffer_resource_view;
        gfx_destroy_uniform_buffer_resource_view = vulkan_device_destroy_uniform_buffer_resource_view;

        gfx_create_single_time_cmd_buffer  = vulkan_device_create_single_time_command_buffer;
        gfx_destroy_single_time_cmd_buffer = vulkan_device_destroy_single_time_command_buffer;

        gfx_readback_swapchain = vulkan_device_readback_swapchain;

        // RHI
        rhi_frame_begin = vulkan_frame_begin;
        rhi_frame_end   = vulkan_frame_end;

        rhi_wait_on_previous_cmds = vulkan_wait_on_previous_cmds;
        rhi_acquire_image         = vulkan_acquire_image;
        rhi_gfx_cmd_enque_submit  = vulkan_gfx_cmd_enque_submit;
        rhi_gfx_cmd_submit_queue  = vulkan_gfx_cmd_submit_queue;
        rhi_present               = vulkan_present;
        rhi_resize_swapchain      = vulkan_resize_swapchain;

        rhi_begin_gfx_cmd_recording = vulkan_begin_gfx_cmd_recording;
        rhi_end_gfx_cmd_recording   = vulkan_end_gfx_cmd_recording;

        rhi_begin_render_pass = vulkan_begin_render_pass;
        rhi_end_render_pass   = vulkan_end_render_pass;

        rhi_set_viewport = vulkan_set_viewport;
        rhi_set_scissor  = vulkan_set_scissor;

        rhi_bind_gfx_pipeline     = vulkan_bind_gfx_pipeline;
        rhi_bind_compute_pipeline = vulkan_bind_compute_pipeline;

        rhi_bind_root_signature   = vulkan_device_bind_root_signature;
        rhi_bind_descriptor_table = vulkan_device_bind_descriptor_table;
        rhi_bind_push_constant    = vulkan_device_bind_push_constant;

        rhi_draw     = vulkan_draw;
        rhi_dispatch = vulkan_dispatch;

        rhi_insert_image_layout_barrier = vulkan_transition_image_layout;

        rhi_clear_image = vulkan_clear_image;
    }

    return Success;
}

void gfx_destroy(void)
{
}

void gfx_ctx_ignite(gfx_context* ctx)
{
    // command pool: only per thread and 2 in-flight command buffers per pool
    ctx->draw_cmds_pool = gfx_create_gfx_cmd_pool();
    for (uint32_t i = 0; i < MAX_FRAME_INFLIGHT; i++) {
        ctx->frame_sync[i] = gfx_create_frame_sync();

        // Allocate and create the command buffers
        ctx->draw_cmds[i] = gfx_create_gfx_cmd_buf(&ctx->draw_cmds_pool);
    }
}

void gfx_ctx_recreate_frame_sync(gfx_context* ctx)
{
    for (uint32_t i = 0; i < MAX_FRAME_INFLIGHT; i++) {
        gfx_destroy_frame_sync(&ctx->frame_sync[i]);
        ctx->frame_sync[i] = gfx_create_frame_sync();
    }
}

uint32_t rhi_get_back_buffer_idx(const gfx_swapchain* swapchain)
{
    return swapchain->current_backbuffer_idx;
}

uint32_t rhi_get_current_frame_idx(const gfx_context* ctx)
{
    return ctx->frame_idx;
}
