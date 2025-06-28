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

void dx12_flush_gpu_work(void);

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
void dx12_free_gfx_cmd_buf(gfx_cmd_buf* cmd_buf);

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

// ...

rhi_error_codes dx12_begin_gfx_cmd_recording(const gfx_cmd_pool* allocator, const gfx_cmd_buf* cmd_buf);
rhi_error_codes dx12_end_gfx_cmd_recording(const gfx_cmd_buf* cmd_buf);

    // ...

#endif    // _WIN32
#endif    // BACKEND_DX12_H
