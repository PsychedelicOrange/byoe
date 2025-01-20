#pragma once

#include <engine/core/gameobject.h>

typedef enum camera_mode
{
    FPS,
    PlayerTarget
} camera_mode;

void Camera_Start(random_uuid_t* uuid);
void Camera_Update(random_uuid_t* uuid, float dt);

// Utility functions for cross-scripting referencing
void Camera_set_player_uuid(random_uuid_t goUUID);
void Camera_set_camera_mode(camera_mode mode);