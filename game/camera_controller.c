#include "camera_controller.h"

#include <logging/log.h>
#include <engine/core/game_state.h>

#include <GLFW/glfw3.h>

// Default camera values
static const float YAW         = -90.0f;
static const float PITCH       = 0.0f;
static const float SPEED       = 2.25f;
static const float SENSITIVITY = 0.25f;
static vec3s       WorldUp     = {{0, 1, 0}};    // World up direction (Y axis)

typedef struct camera_internal_state
{
    camera_mode   cameraMode;
    random_uuid_t playerUUID;
    GameObject*   playerGameObject;
} camera_internal_state;

camera_internal_state s_cameraState;

//------------------------------------------------
// Private
void process_keyboard(Camera* camera, enum Camera_Movement_Direction direction, float dt)
{
    (void) dt;
    (void) camera;
    (void) direction;

    float velocity = SPEED * dt;

    vec3s CameraMovement = {GLM_VEC3_ZERO_INIT};

    if (direction == FORWARD)
        CameraMovement = glms_vec3_add(CameraMovement, camera->front);
    if (direction == BACKWARD)
        CameraMovement = glms_vec3_sub(CameraMovement, camera->front);

    if (direction == RIGHT)
        CameraMovement = glms_vec3_add(CameraMovement, camera->right);
    if (direction == LEFT)
        CameraMovement = glms_vec3_sub(CameraMovement, camera->right);

    if (direction == UP)
        CameraMovement = glms_vec3_add(CameraMovement, camera->up);
    if (direction == DOWN)
        CameraMovement = glms_vec3_sub(CameraMovement, camera->up);

    CameraMovement = glms_vec3_scale(CameraMovement, velocity);

    camera->position = glms_vec3_add(camera->position, CameraMovement);

    LOG_WARN("%f", camera->position.z);
}

void process_mouse_movement(Camera* camera, float xoffset, float yoffset)
{
    xoffset *= SENSITIVITY;
    yoffset *= SENSITIVITY;

    camera->yaw += xoffset;
    camera->pitch += yoffset;

    if (camera->pitch > 89.0f)
        camera->pitch = 89.0f;
    if (camera->pitch < -89.0f)
        camera->pitch = -89.0f;

    // Update Front, Right and Up Vectors using the updated Euler angles
    //updateCameraVectors();
}

void update_camera_vectors(Camera* camera)
{
    // Calculate new front vector using yaw and pitch
    vec3s front   = {0};
    front.x       = cosf(glm_rad(camera->yaw)) * cosf(glm_rad(camera->pitch));
    front.y       = sinf(glm_rad(camera->pitch));
    front.z       = sinf(glm_rad(camera->yaw)) * cosf(glm_rad(camera->pitch));
    camera->front = glms_vec3_normalize(front);

    // Recalculate the camera's right vector after the front vector is updated
    camera->right = glms_vec3_normalize(glms_vec3_cross(camera->front, WorldUp));
    camera->up    = glms_vec3_normalize(glms_vec3_cross(camera->right, camera->front));    // Up vector (recalculated)

    // LOG_INFO("camera pos: (%f, %f, %f)\n", camera->position.x, camera->position.y, camera->position.z);

    // Update the camera's lookAt matrix
    glm_look(camera->position.raw, camera->front.raw, WorldUp.raw, camera->lookAt.raw);
}

//------------------------------------------------
// Public

void Camera_Start(random_uuid_t* uuid)
{
    (void) uuid;

    GameState* gameState = gamestate_get_global_instance();
    Camera*    camera    = &gameState->camera;

    // Init some default values for the camera
    camera->position.x = 0;
    camera->position.y = 0;
    camera->position.z = 5;

    camera->front.x = 0;
    camera->front.y = 0;
    camera->front.z = 1;

    camera->up.x = WorldUp.x;
    camera->up.y = WorldUp.y;
    camera->up.z = WorldUp.z;

    camera->right.x = 1;
    camera->right.y = 0;
    camera->right.z = 0;

    camera->yaw   = YAW;
    camera->pitch = PITCH;

    camera->near_plane = 0.01f;
    camera->far_plane  = 100.0f;
    camera->fov        = 45.0f;

    s_cameraState.cameraMode       = PlayerTarget;
    s_cameraState.playerGameObject = NULL;
}

void Camera_Update(random_uuid_t* uuid, float dt)
{
    (void) uuid;
    (void) dt;

    GameState* gameState = gamestate_get_global_instance();
    Camera*    camera    = &gameState->camera;

    if (s_cameraState.cameraMode == FPS) {
        // Update camera position with keyboard input
        if (gameState->keycodes[GLFW_KEY_W])
            process_keyboard(camera, FORWARD, dt);
        if (gameState->keycodes[GLFW_KEY_S])
            process_keyboard(camera, BACKWARD, dt);

        if (gameState->keycodes[GLFW_KEY_D])
            process_keyboard(camera, RIGHT, dt);
        if (gameState->keycodes[GLFW_KEY_A])
            process_keyboard(camera, LEFT, dt);

        if (gameState->isMousePrimaryDown) {
            process_mouse_movement(camera, -gameState->mouseDelta[0], gameState->mouseDelta[1]);
        }
    } else {    // Follow the player

        if (!s_cameraState.playerGameObject)
            s_cameraState.playerGameObject = game_registry_get_gameobject_by_uuid(s_cameraState.playerUUID);

        vec3s playerPosition = {0};
        gameobject_get_position(s_cameraState.playerUUID, &playerPosition.raw);
        // LOG_INFO("%f", playerPosition.z);
        playerPosition.z += 3;
        playerPosition.y += 1;
        glm_vec3_copy(playerPosition.raw, camera->position.raw);
        // LOG_WARN("%f", camera->position.z);
        camera->position = glms_vec3_add(camera->position, playerPosition);

    }

    update_camera_vectors(camera);
}

void Camera_set_player_uuid(random_uuid_t goUUID)
{
    s_cameraState.playerUUID = goUUID;
}
