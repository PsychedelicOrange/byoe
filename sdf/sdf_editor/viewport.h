#ifndef INCLUDE_VIEWPORT_BYOE_H
#define INCLUDE_VIEWPORT_BYOE_H

#include "engine/scene/camera.h"

typedef struct viewport_state
{
    float        x, y, w, h;
    unsigned int shader;
    unsigned int VAO;
    mat4s        projection;
    float        lastX, lastY;
    Camera       camera;
} viewport_state;

void           viewport_draw(viewport_state* state);
viewport_state viewport_default(int window[2]);

void viewport_window_resize_callback(viewport_state* state, int w, int h);
void update_camera_mouse_callback(viewport_state* viewport, float xoffset, float yoffset);

#endif    // INCLUDE_VIEWPORT_BYOE_H
