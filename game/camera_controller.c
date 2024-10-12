#include "camera_controller.h"

uuid_t cameraUUID;

void Camera_Start(uuid_t uuid, void* gameState, void* gameObjData)
{
    (void) gameState;
    (void) gameObjData;
    (void) uuid;
}

void Camera_Update(uuid_t uuid, void* gameState, void* gameObjData, float dt)
{
    (void) gameState;
    (void) gameObjData;
    (void) dt;
    (void) uuid;
    
    printf("[Camera Script] deltaTime : %fms \n", dt * 1000);
}
