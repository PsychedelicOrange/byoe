#include "camera_controller.h"

#include <logging/log.h>

#include <engine/core/game_state.h>

#include <GLFW/glfw3.h>

void Camera_Start(uuid_t uuid)
{
    (void)uuid;

    Camera* camera = &((GameState*)gamestate_get_global_instance())->camera;

    camera->position.x = 0;
    camera->position.y = 0;
    camera->position.z = 3;

    camera->front.x = 0;
    camera->front.y = 0;
    camera->front.z = -1;

    camera->right.x = 1;
    camera->right.y = 0;
    camera->right.z = 0;
}

void Camera_Update(uuid_t uuid, float dt)
{
    (void)uuid;
    (void)dt;

    Camera* camera = &((GameState*)gamestate_get_global_instance())->camera;
    GameState* gameStatePtr = (GameState*)gamestate_get_global_instance();

    if (gameStatePtr->keycodes[GLFW_KEY_W])
        camera->position = glms_vec3_add(camera->position, glms_vec3_scale_as(camera->front, speed * dt));
    if (gameStatePtr->keycodes[GLFW_KEY_A])
        camera->position = glms_vec3_add(camera->position, glms_vec3_scale_as(camera->right, -speed * dt));
    if (gameStatePtr->keycodes[GLFW_KEY_S])
        camera->position = glms_vec3_add(camera->position, glms_vec3_scale_as(camera->front, -speed * dt));
    if (gameStatePtr->keycodes[GLFW_KEY_D])
        camera->position = glms_vec3_add(camera->position, glms_vec3_scale_as(camera->right, speed * dt));

    glm_look(camera->position.raw, camera->front.raw, up.raw, camera->lookAt.raw);

    LOG_WARN("[Camera Script] deltaTime : %fms", dt * 1000.0f);
}
