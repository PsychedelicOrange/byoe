#include "camera_controller.h"

#include <logging/log.h>

#include <engine/core/game_state.h>

#include <GLFW/glfw3.h>

uuid_t cameraUUID;

void Camera_Start(void* gameState, void* gameObjData)
{
    (void)gameState;
    (void)gameObjData;

    Camera* camera = &((GameState*)gameState)->camera;

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

void Camera_Update(void* gameState, void* gameObjData, float dt)
{
    (void)gameState;
    (void)gameObjData;
    (void)dt;

    Camera* camera = &((GameState*)gameState)->camera;
    GameState* gameStatePtr = (GameState*)gameState;

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
