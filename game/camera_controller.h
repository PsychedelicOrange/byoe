#pragma once
#include <engine/core/gameobject.h>
#include<cglm/cglm.h>

typedef struct CamData
{
    vec3 camUp;
}CamData;

extern uuid_t cameraUUID;

void Camera_Start(uuid_t uuid, void* gameState, void* gameObjData);
void Camera_Update(uuid_t uuid, void* gameState, void* gameObjData, float dt);