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
void        (*gfx_ctx_destroy)(gfx_context ctx);

gfx_swapchain (*gfx_create_swapchain)(uint32_t width, uint32_t height);
void          (*gfx_destroy_swapchain)(gfx_swapchain sc);

gfx_cmd_pool (*gfx_create_gfx_cmd_pool)(void);
void         (*gfx_destroy_gfx_cmd_pool)(gfx_cmd_pool);

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

rhi_error_codes (*rhi_acquire_image)(gfx_swapchain* swapchain, gfx_fence* frame_sync);

rhi_error_codes (*rhi_clear)(void);
rhi_error_codes (*rhi_draw)(void);
rhi_error_codes (*rhi_present)(void);

#endif    // RHI_H
