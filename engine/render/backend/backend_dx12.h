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

#endif    // _WIN32
#endif    // BACKEND_DX12_H
