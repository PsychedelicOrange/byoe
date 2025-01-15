#pragma once

#include <engine/core/gameobject.h>

// Defines several possible options for camera movement. Used as abstraction to stay away from window-system specific input methods
enum Camera_Movement_Direction
{
    FORWARD,
    BACKWARD,
    LEFT,
    RIGHT,
    UP,
    DOWN
};

typedef enum camera_mode
{
    FPS,
    PlayerTarget
} camera_mode;

void Camera_Start(random_uuid_t* uuid);
void Camera_Update(random_uuid_t* uuid, float dt);

// Utility functions for cross-scripting references
void Camera_set_player_uuid(random_uuid_t goUUID);