#include "camera_controller.h"

#include <logging/log.h>

#include <engine/core/game_state.h>

#include <GLFW/glfw3.h>

void Camera_Start(random_uuid_t* uuid)
{
    (void) uuid;

    GameState* gameState = gamestate_get_global_instance();
    Camera*    camera    = &gameState->camera;

    camera->position.x = 0;
    camera->position.y = 0;
    camera->position.z = 3;

    camera->front.x = 0;
    camera->front.y = 0;
    camera->front.z = -1;

    camera->right.x = 1;
    camera->right.y = 0;
    camera->right.z = 0;

    camera->yaw   = -90.0f;
    camera->pitch = 0.0f;
}

void Camera_Update(random_uuid_t* uuid, float dt)
{
    (void) uuid;
    (void) dt;

    vec3s worldUp     = {{0, 1, 0}};    // World up direction (Y axis)
    float speed       = 2.5f;
    float sensitivity = 0.1f;    // Mouse sensitivity for rotation

    GameState* gameState = gamestate_get_global_instance();
    Camera*    camera    = &gameState->camera;

    // Mouse delta (change in mouse position)
    static double lastMouseX = 0.0, lastMouseY = 0.0;
    double        mouseX = gameState->mousePos[0];
    double        mouseY = gameState->mousePos[1];
    double        deltaX = lastMouseX - mouseX;
    double        deltaY = mouseY - lastMouseY;

    lastMouseX = mouseX;
    lastMouseY = mouseY;

    if (gameState->isMouseDown) {
        // Update camera yaw and pitch based on mouse movement
        camera->yaw += (float) deltaX * sensitivity;
        camera->pitch += (float) deltaY * sensitivity;

        // Clamp the pitch to avoid flipping the camera
        if (camera->pitch > 89.0f)
            camera->pitch = 89.0f;
        if (camera->pitch < -89.0f)
            camera->pitch = -89.0f;
    }

    // Calculate new front vector using yaw and pitch
    vec3s front;
    front.x       = cosf(glm_rad(camera->yaw)) * cosf(glm_rad(camera->pitch));
    front.y       = sinf(glm_rad(camera->pitch));
    front.z       = sinf(glm_rad(camera->yaw)) * cosf(glm_rad(camera->pitch));
    camera->front = glms_vec3_normalize(front);

    // Recalculate the camera's right vector after the front vector is updated
    camera->right = glms_vec3_normalize(glms_vec3_cross(camera->front, worldUp));
    camera->up    = glms_vec3_normalize(glms_vec3_cross(camera->right, camera->front));    // Up vector (recalculated)

    // Update camera position with keyboard input
    if (gameState->keycodes[GLFW_KEY_W])
        camera->position = glms_vec3_add(camera->position, glms_vec3_scale_as(camera->front, speed * dt));
    if (gameState->keycodes[GLFW_KEY_A])
        camera->position = glms_vec3_add(camera->position, glms_vec3_scale_as(camera->right, -speed * dt));
    if (gameState->keycodes[GLFW_KEY_S])
        camera->position = glms_vec3_add(camera->position, glms_vec3_scale_as(camera->front, -speed * dt));
    if (gameState->keycodes[GLFW_KEY_D])
        camera->position = glms_vec3_add(camera->position, glms_vec3_scale_as(camera->right, speed * dt));

    // Update the camera's lookAt matrix
    glm_look(camera->position.raw, camera->front.raw, worldUp.raw, camera->lookAt.raw);
}
