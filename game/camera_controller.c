#include "camera_controller.h"

#include <logging/log.h>

#include <engine/core/game_state.h>

#include <GLFW/glfw3.h>

void Camera_Start(random_uuid_t* uuid)
{
    (void)uuid;

    GameState* gameState = gamestate_get_global_instance();
    Camera* camera = &gameState->camera;
    
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

void Camera_Update(random_uuid_t* uuid, float dt)
{
    (void)uuid;
    (void)dt;

	vec3s up = {{0,1,0}};
	float speed = 2.5f;

    GameState* gameState = gamestate_get_global_instance();
    Camera* camera = &gameState->camera;

    if (gameState->keycodes[GLFW_KEY_W])
        camera->position = glms_vec3_add(camera->position, glms_vec3_scale_as(camera->front, speed * dt));
    if (gameState->keycodes[GLFW_KEY_A])
        camera->position = glms_vec3_add(camera->position, glms_vec3_scale_as(camera->right, -speed * dt));
    if (gameState->keycodes[GLFW_KEY_S])
        camera->position = glms_vec3_add(camera->position, glms_vec3_scale_as(camera->front, -speed * dt));
    if (gameState->keycodes[GLFW_KEY_D])
        camera->position = glms_vec3_add(camera->position, glms_vec3_scale_as(camera->right, speed * dt));

    glm_look(camera->position.raw, camera->front.raw, up.raw, camera->lookAt.raw);
}
