#ifndef RHI_H
#define RHI_H

#include "../render/render_structs.h"

#include <stdint.h>

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

gfx_context (*gfx_ctx_init)(GLFWwindow* window, uint32_t width, uint32_t height);
void        (*gfx_ctx_destroy)(gfx_context* ctx);

void gfx_ctx_ignite(gfx_context* ctx);

gfx_swapchain (*gfx_create_swapchain)(uint32_t width, uint32_t height);
void          (*gfx_destroy_swapchain)(gfx_swapchain sc);

gfx_cmd_pool (*gfx_create_gfx_cmd_pool)(void);
void         (*gfx_destroy_gfx_cmd_pool)(gfx_cmd_pool);

gfx_frame_sync* (*gfx_create_frame_syncs)(uint32_t num_frames_in_flight);
void            (*gfx_destroy_frame_syncs)(gfx_frame_sync* in_flight_syncs);

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

rhi_error_codes (*rhi_frame_begin)(gfx_context* context);
rhi_error_codes (*rhi_frame_end)(gfx_context* context);

rhi_error_codes (*rhi_wait_on_previous_cmds)(const gfx_frame_sync* in_flight_sync);
rhi_error_codes (*rhi_acquire_image)(gfx_swapchain* swapchain, const gfx_frame_sync* in_flight_sync);

rhi_error_codes (*rhi_clear)(void);
rhi_error_codes (*rhi_draw)(void);
rhi_error_codes (*rhi_present)(void);

uint32_t              (*rhi_get_back_buffer_idx)(gfx_swapchain* swapchain);
uint32_t              (*rhi_get_current_frame_idx)();
const gfx_frame_sync* (*rhi_get_current_frame_sync)();

#endif    // RHI_H
