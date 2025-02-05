#include "viewport.h"
#include "engine/render/render_utils.h"
#include "glad/glad.h"
#include "logging/log.h"
#include "sdf/sdf_format/sdf_file_types.h"
#include "shader.h"

#include <GLFW/glfw3.h>
#include <stdio.h>

void viewport_window_resize_callback(viewport_state* state, int w, int h)
{
    state->w          = w;
    state->h          = h;
    state->projection = glms_perspective(state->camera.fov, (float) state->w / (float) state->h, state->camera.near_plane, state->camera.far_plane);
}
void update_third_person_camera(Camera* camera)
{
    static vec3s up    = {0, 1, 0};
    camera->position.x = camera->third_person_distance * cos(glm_rad(camera->pitch)) * (sqrt(75)) *cos(glm_rad(camera->yaw)) + camera->center.x;
    camera->position.y = camera->third_person_distance * sin(glm_rad(camera->pitch)) * (sqrt(75)) + camera->center.y;
    camera->position.z = camera->third_person_distance * cos(glm_rad(camera->pitch)) * (sqrt(75)) *sin(glm_rad(camera->yaw)) + camera->center.z;
    //LOG_INFO("%f", camera->pitch);
    //LOG_INFO("%f, %f, %f", camera->position.x, camera->position.y, camera->position.z);
    camera->front = glms_normalize(glms_vec3_sub(camera->position, camera->center));
    camera->right = glms_normalize(glms_cross(camera->front, up));    // normalize the vectors, because their length gets closer to 0 the more you look up or down which results in slower movement.
    camera->up    = glms_normalize(glms_cross(camera->right, camera->front));

    glm_lookat(camera->position.raw, camera->center.raw, camera->up.raw, camera->lookAt.raw);
}
void update_first_person_camera(Camera* camera)
{
    static vec3s up = {0, 1, 0};
    vec3s        front;
    front.x       = cos(glm_rad(camera->yaw)) * cos(glm_rad(camera->pitch));
    front.y       = sin(glm_rad(camera->pitch));
    front.z       = sin(glm_rad(camera->yaw)) * cos(glm_rad(camera->pitch));
    camera->front = glms_normalize(front);
    camera->right = glms_normalize(glms_cross(camera->front, up));    // normalize the vectors, because their length gets closer to 0 the more you look up or down which results in slower movement.
    camera->up    = glms_normalize(glms_cross(camera->right, camera->front));
    vec3 v        = {0, 0, 0};

    vec3 cam_dir = {0};
    glm_vec3_add(camera->position.raw, camera->front.raw, cam_dir);
    (void) cam_dir;

    glm_look(camera->position.raw, camera->front.raw, camera->up.raw, camera->lookAt.raw);
}

void update_camera_mouse_callback(viewport_state* viewport, float xoffset, float yoffset)
{
    if (viewport->orbit_mode) {
        viewport->camera.yaw += xoffset;
        viewport->camera.pitch -= yoffset;
        float pitch = viewport->camera.pitch;
        if (pitch > 89.0f)
            pitch = 89.0f;
        if (pitch < -89.0f)
            pitch = -89.0f;
        viewport->camera.pitch = pitch;
    } else if (viewport->pan_mode) {
        // move the camera in camera-front plane, according to offset
        vec3s CameraMovement    = {GLM_VEC3_ZERO_INIT};
        viewport->camera.center = glms_vec3_add(viewport->camera.center, glms_vec3_scale(viewport->camera.right, 0.1 * xoffset));
        viewport->camera.center = glms_vec3_add(viewport->camera.center, glms_vec3_scale(viewport->camera.up, -0.1 * yoffset));
    }
}
void viewport_zoom(viewport_state* v, double xoff, double yoff)
{
    v->camera.third_person_distance -= 0.1 * yoff;
}

void draw_grid(viewport_state* v)
{
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glUseProgram(v->grid.shader);
    setUniformMat4(v->grid.shader, v->projection, "projection");
    setUniformMat4(v->grid.shader, GLMS_MAT4_IDENTITY, "model");
    setUniformMat4(v->grid.shader, v->camera.lookAt, "view");
    glBindVertexArray(v->grid.vao);
    glDrawArrays(GL_LINES, 0, 21 * 21 * 21);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

void viewport_draw(viewport_state* state, struct format_sdf_file file)
{
    glUseProgram(state->shader);
    glEnable(GL_DEPTH_TEST);

    for (int i = 0; i < file.primitives_count; i++) {
        switch (file.primitives[i].type) {
            case SDF_PRIM_Sphere:
                setUniformFloat(state->shader, file.primitives->props.sphere.radius, "sphere_radius_0");
                break;
            default:
                LOG_ERROR("Not supported");
                break;
        }
    }
    //update_first_person_camera(&(state->camera));
    update_third_person_camera(&(state->camera));

    int window[2] = {state->w, state->h};
    setUniformVec2Int(state->shader, window, "resolution");

    mat4s viewproj = glms_mul(state->projection, state->camera.lookAt);
    setUniformMat4(state->shader, viewproj, "viewproj");

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    glViewport(state->x, state->y, window[0], window[1]);
    glUseProgram(state->shader);
    glBindVertexArray(state->VAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    draw_grid(state);
}

Camera camera_init()
{
    Camera camera;
    camera.position.x = 0;
    camera.position.y = 1;
    camera.position.z = 5;

    camera.center.x = 0;
    camera.center.y = 0;
    camera.center.z = 0;

    camera.front.x = 0;
    camera.front.y = 0;
    camera.front.z = -1;

    camera.right.x = 1;
    camera.right.y = 0;
    camera.right.z = 0;

    camera.yaw   = -90.;
    camera.pitch = 0;

    camera.near_plane            = 0.01f;
    camera.far_plane             = 1000.0f;
    camera.fov                   = 20.0f;
    camera.third_person_distance = 1;

    vec3s origin = {0, 0, 0};

    glm_lookat(camera.position.raw, origin.raw, camera.up.raw, camera.lookAt.raw);
    return camera;
}

unsigned int upload_lines(float unit, int count)
{
    struct line
    {
        float x, y, z;
        float x1, y1, z1;
    };
    int          line_count_per_axis = (count * 2 + 1) * (count * 2 + 1);
    struct line* l                   = malloc(sizeof(struct line) * line_count_per_axis * 3);
    int          lindex              = 0;
    float        boundary            = unit * count;
    for (int i = -count; i <= count; i++) {
        for (int j = -count; j <= count; j++) {
            // z axis lines
            l[lindex].x  = i * unit;
            l[lindex].y  = j * unit;
            l[lindex].x1 = i * unit;
            l[lindex].y1 = j * unit;

            l[lindex].z  = boundary;
            l[lindex].z1 = -boundary;
            lindex++;
            // x axis lines
            l[lindex].z  = i * unit;
            l[lindex].y  = j * unit;
            l[lindex].z1 = i * unit;
            l[lindex].y1 = j * unit;

            l[lindex].x1 = -boundary;
            l[lindex].x  = boundary;
            lindex++;
            // y axis lines
            l[lindex].z  = i * unit;
            l[lindex].x  = j * unit;
            l[lindex].z1 = i * unit;
            l[lindex].x1 = j * unit;

            l[lindex].y  = boundary;
            l[lindex].y1 = -boundary;
            lindex++;
        }
    }
    unsigned int VAO, VBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, (sizeof(struct line) * line_count_per_axis * 3), l, GL_STATIC_DRAW);
    free(l);

    // Position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*) 0);
    glEnableVertexAttribArray(0);
    return VAO;
}
viewport_state viewport_default(int window[2])
{
    viewport_state v = {
        .x               = 0,
        .y               = 0,
        .w               = window[0],
        .h               = window[1],
        .shader          = create_shader("../../engine/shaders/screen_quad.vert", "resources/raymarch.frag"),
        .VAO             = render_utils_setup_screen_quad(),
        .grid            = {upload_lines(10, 2), create_shader("resources/gridvert", "resources/gridfrag")},
        .selected_object = -1,
        .camera          = camera_init(),
        .pan_mode        = 0,
        .orbit_mode      = 0};

    v.projection = glms_perspective(v.camera.fov, (float) window[0] / (float) window[1], v.camera.near_plane, v.camera.far_plane);

    //LOG_INFO("Viewport Initialized");

    return v;
}
