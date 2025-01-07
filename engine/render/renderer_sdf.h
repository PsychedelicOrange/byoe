#ifndef RENDERER_SDF_H
#define RENDERER_SDF_H

#include "render_structs.h"

typedef struct renderer_desc
{
    uint32_t width;
    uint32_t height;
    struct GLFWwindow* window;
} renderer_desc;

bool renderer_sdf_create(renderer_desc desc);
void renderer_sdf_destroy(void);

// this is where the drawcalls and rendering logic is handled including culling
void renderer_sdf_render(void);

#endif