#ifndef INCLUDE_VIEWPORT_BYOE_H
#define INCLUDE_VIEWPORT_BYOE_H

#include "engine/scene/camera.h"
#include "sdf/sdf_format/sdf_file_types.h"

struct grid
{
    unsigned int vao;
    unsigned int shader;
};
typedef struct viewport_state
{
    float        x, y, w, h;
    unsigned int shader;
    struct grid  grid;
    unsigned int VAO;
    mat4s        projection;
    float        lastX, lastY;
    Camera       camera;
    int          selected_object;
    int          pan_mode;
    int          orbit_mode;
} viewport_state;
void           viewport_draw(viewport_state* state, struct format_sdf_file file);
viewport_state viewport_default(int window[2]);

void viewport_window_resize_callback(viewport_state* state, int w, int h);
void update_camera_mouse_callback(viewport_state* viewport, float xoffset, float yoffset);

void viewport_zoom(viewport_state* v, double xoff, double yoff);

#endif    // INCLUDE_VIEWPORT_BYOE_H
