#include "player.h"

void Player_Start(void* gameState, void* gameObjData)
{
    PlayerData* data = (PlayerData*)gameObjData;
    glm_vec3_copy((vec3){0.0f, 0.0f, 0.0f}, data->position);
}

void Player_Update(void* gameState, void* gameObjData, float dt)
{
    printf("deltaTime : %f", dt);
}