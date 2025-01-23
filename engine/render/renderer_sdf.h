#ifndef RENDERER_SDF_H
#define RENDERER_SDF_H

// Design explanation: https://github.com/PsychedelicOrange/byoe/pull/10

#include "gameobject.h"
#include "render_structs.h"

// forward declaration
typedef struct SDF_Scene SDF_Scene;

typedef struct renderer_desc
{
    uint32_t           width;
    uint32_t           height;
    struct GLFWwindow* window;
} renderer_desc;

typedef struct texture_readback
{
    alignas(16) uint32_t width;
    uint32_t height;
    uint32_t bits_per_pixel;
    uint32_t _pad0;
    char*    pixels;
    bool     _pad1[8];
} texture_readback;

bool renderer_sdf_init(renderer_desc desc);
void renderer_sdf_destroy(void);

// this is where the draw calls and rendering logic is handled including culling
void renderer_sdf_render(void);

const SDF_Scene* renderer_sdf_get_scene(void);
void             renderer_sdf_set_scene(const SDF_Scene* scene);

void renderer_sdf_set_clear_color(color_rgba rgba);

void renderer_sdf_draw_scene(const SDF_Scene* scene);

void             renderer_sdf_set_capture_swapchain_ready(void);
texture_readback renderer_sdf_read_swapchain(void);
texture_readback renderer_sdf_get_last_swapchain_readback(void);

void renderer_sdf_resize(uint32_t width, uint32_t height);

#endif