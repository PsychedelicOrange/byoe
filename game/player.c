#include "player.h"

uuid_t playerUUID;

void Player_Start(void* gameState, void* gameObjData)
{
    PlayerData* data = (PlayerData*)gameObjData;
    glm_vec3_copy((vec3){0.0f, 0.0f, 0.0f}, data->position);
}

void Player_Update(void* gameState, void* gameObjData, float dt)
{
    printf("[Player Script] deltaTime : %f \n", dt);
}