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
void        (*gfx_ctx_destroy)(gfx_context* ctx);

gfx_swapchain (*gfx_create_swapchain)(uint32_t width, uint32_t height);
void          (*gfx_destroy_swapchain)(gfx_swapchain* sc);

gfx_cmd_pool (*gfx_create_gfx_cmd_pool)(void);
void         (*gfx_destroy_gfx_cmd_pool)(gfx_cmd_pool*);

gfx_cmd_buf (*gfx_create_gfx_cmd_buf)(gfx_cmd_pool* pool);

gfx_frame_sync (*gfx_create_frame_sync)(void);
void           (*gfx_destroy_frame_sync)(gfx_frame_sync* in_flight_sync);

//------------------------------------------
// RHI function pointers
//------------------------------------------
typedef enum rhi_error_codes
{
    Success,
    FailedUnknown,
    FailedMemoryAlloc,
    FailedSwapAcquire,
    FailedHandleCreation
} rhi_error_codes;
//---------------------------
// High-level API
gfx_frame_sync* (*rhi_frame_begin)(gfx_context* context);
rhi_error_codes (*rhi_frame_end)(gfx_context* context);
// Begin/End RenderPass
//---------------------------

rhi_error_codes (*rhi_wait_on_previous_cmds)(const gfx_frame_sync* in_flight_sync);
rhi_error_codes (*rhi_acquire_image)(gfx_swapchain* swapchain, const gfx_frame_sync* in_flight_sync);
rhi_error_codes (*rhi_gfx_cmd_enque_submit)(gfx_cmd_queue* cmd_queue, gfx_cmd_buf* cmd_buff);
rhi_error_codes (*rhi_gfx_cmd_submit_queue)(gfx_cmd_queue* cmd_queue, gfx_frame_sync* frame_sync);
rhi_error_codes (*rhi_present)(gfx_swapchain* swapchain, gfx_frame_sync* frame_sync);

rhi_error_codes (*rhi_resize_swapchain)(gfx_swapchain* swapchain, uint32_t width, uint32_t height);

rhi_error_codes (*rhi_begin_gfx_cmd_recording)(gfx_cmd_buf* cmd_buf);
rhi_error_codes (*rhi_end_gfx_cmd_recording)(gfx_cmd_buf* cmd_buf);

rhi_error_codes (*rhi_clear)(void);
rhi_error_codes (*rhi_draw)(void);

uint32_t rhi_get_back_buffer_idx(const gfx_swapchain* swapchain);
uint32_t rhi_get_current_frame_idx(const gfx_context* ctx);
#endif    // RHI_H
