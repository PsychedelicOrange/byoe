#include <engine/core/gameobject.h>

#include<cglm/cglm.h>

void Camera_Start(void* gameState, void* gameObjData)
{
    
}

void Camera_Update(void* gameState, void* gameObjData, float dt)
{

}

typedef struct CamData
{
    vec3 camUp;
}CamData;

// void autoRegisterGameObject()
// {
//     uuid_t cameraUUID = REGISTER_GAME_OBJECT("Camera", CamData, Camera_Start, Camera_Update);
// }