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
gfx_context dx12_ctx_init(GLFWwindow* window, uint32_t width, uint32_t height);
void        dx12_ctx_destroy(gfx_context* ctx);

void dx12_flush_gpu_work(void);

gfx_swapchain dx12_create_swapchain(uint32_t width, uint32_t height);
void          dx12_destroy_swapchain(gfx_swapchain* swapchain);

#endif
#endif    // BACKEND_DX12_H
