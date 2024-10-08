#include <engine/core/gameobject.h>

#include<cglm/cglm.h>


void Camera_Start(void* gameState, void* gameObjData)
{
    
}

void Camera_Update(void* gameState, void* gameObjData, float dt)
{

}


REGISTER_GAME_OBJECT(Camera, camData, Camera_Start, Camera_Update);
