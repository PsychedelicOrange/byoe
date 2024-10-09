#pragma once
#include <engine/core/gameobject.h>
#include<cglm/cglm.h>

typedef struct CamData
{
    vec3 camUp;
}CamData;

extern uuid_t cameraUUID;

void Camera_Start(void* gameState, void* gameObjData);
void Camera_Update(void* gameState, void* gameObjData, float dt);