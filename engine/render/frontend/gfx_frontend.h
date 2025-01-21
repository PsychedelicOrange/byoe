#ifndef RHI_H
#define RHI_H

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

int  gfx_init(rhi_api api, GLFWwindow* window, uint32_t width, uint32_t height);
void gfx_destroy();

//------------------------------------------
// RHI function pointers
//------------------------------------------
typedef enum rhi_error_codes
{
    Success,
    FailedUnknown,
    FailedMemoryAlloc,
    FailedHandleCreation
} rhi_error_codes;

rhi_error_codes (*rhi_clear)();
rhi_error_codes (*rhi_draw)();
rhi_error_codes (*rhi_present)();

#endif    // RHI_H
