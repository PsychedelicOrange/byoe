#ifndef RENDERER_SDF_H
#define RENDERER_SDF_H

#include <stdint.h>
#include <stdbool.h>

typedef struct renderer_desc
{
    uint32_t width;
    uint32_t height;
}renderer_desc;

bool renderer_sdf_init(renderer_desc desc);
void renderer_sdf_destroy();

#endif