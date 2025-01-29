#include "viewport.h"
#include "engine/render/render_utils.h"
#include "glad/glad.h"
#include "logging/log.h"
#include "shader.h"

#include <GLFW/glfw3.h>

void viewport_window_resize_callback(viewport_state* state, int w, int h)
{
    state->w          = w;
    state->h          = h;
    state->projection = glms_perspective(state->camera.fov, (float) state->w / (float) state->h, state->camera.near_plane, state->camera.far_plane);
}

void update_first_person_camera(Camera* camera)
{
    vec3s front;
    front.x       = cos(glm_rad(camera->yaw)) * cos(glm_rad(camera->pitch));
    front.y       = sin(glm_rad(camera->pitch));
    front.z       = sin(glm_rad(camera->yaw)) * cos(glm_rad(camera->pitch));
    camera->front = glms_normalize(front);
    camera->right = glms_normalize(glms_cross(camera->front, camera->up));    // normalize the vectors, because their length gets closer to 0 the more you look up or down which results in slower movement.
    camera->up    = glms_normalize(glms_cross(camera->right, camera->front));
}

void update_camera_mouse_callback(viewport_state* viewport, float xoffset, float yoffset)
{
    viewport->camera.yaw += xoffset;
    viewport->camera.pitch += yoffset;
    float pitch = viewport->camera.pitch;
    if (pitch > 89.0f)
        pitch = 89.0f;
    if (pitch < -89.0f)
        pitch = -89.0f;
    viewport->camera.pitch = pitch;
}

void viewport_draw(viewport_state* state)
{
    update_first_person_camera(&(state->camera));

    int window[2] = {state->w, state->h};
    setUniformVec2Int(state->shader, window, "resolution");

    mat4s viewproj = glms_mul(state->projection, state->camera.lookAt);
    setUniformMat4(state->shader, viewproj, "viewproj");

    glClear(GL_COLOR_BUFFER_BIT);
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    glViewport(state->x, state->y, window[0], window[1]);
    glUseProgram(state->shader);
    glBindVertexArray(state->VAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
}

Camera camera_init()
{
    Camera camera;
    camera.position.x = 0;
    camera.position.y = 0;
    camera.position.z = 0;

    camera.front.x = 0;
    camera.front.y = 0;
    camera.front.z = 1;

    camera.right.x = 1;
    camera.right.y = 0;
    camera.right.z = 0;

    camera.yaw   = -90.;
    camera.pitch = 0;

    camera.near_plane = 0.01f;
    camera.far_plane  = 100.0f;
    camera.fov        = 45.0f;

    glm_lookat(camera.position.raw, camera.front.raw, camera.up.raw, camera.lookAt.raw);
    return camera;
}

viewport_state viewport_default(int window[2])
{
    viewport_state v = {
        .x      = 0,
        .y      = 0,
        .w      = window[0],
        .h      = window[1],
        .shader = create_shader("../../engine/shaders/screen_quad.vert", "resources/raymarch.frag"),
        .VAO    = render_utils_setup_screen_quad(),
        .camera = camera_init()};
    v.projection = glms_perspective(v.camera.fov, (float) window[0] / (float) window[1], v.camera.near_plane, v.camera.far_plane);

    LOG_INFO("Viewport Initialized");
    // set shader uniforms
    //setUniformMat4(v.shader, v.projection, "projection");
    //mat4s model = GLMS_MAT4_IDENTITY_INIT;
    //setUniformMat4(v.shader, model, "model");

    return v;
}
