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

bool renderer_sdf_init(renderer_desc desc);
void renderer_sdf_destroy(void);

// this is where the drawcalls and rendering logic is handled including culling
void renderer_sdf_render(void);

const SDF_Scene* renderer_sdf_get_scene(void);
void             renderer_sdf_set_scene(const SDF_Scene* scene);

void renderer_sdf_draw_scene(const SDF_Scene* scene);

#endif