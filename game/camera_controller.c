#include "camera_controller.h"

uuid_t cameraUUID;

void Camera_Start(void* gameState, void* gameObjData)
{
    (void) gameState;
    (void) gameObjData;
}

void Camera_Update(void* gameState, void* gameObjData, float dt)
{
    (void) gameState;
    (void) gameObjData;
    (void) dt;
    
    printf("[Camera Script] deltaTime : %f \n", dt);
}
