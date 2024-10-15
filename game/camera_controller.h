#pragma once

#include <engine/core/gameobject.h>

extern uuid_t cameraUUID;

void Camera_Start(void* gameState, void* gameObjData);
void Camera_Update(void* gameState, void* gameObjData, float dt);