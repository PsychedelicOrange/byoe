#include "player.h"

uuid_t playerUUID;

void Player_Start(uuid_t uuid, void* gameState, void* gameObjData)
{
    (void) gameState;
    (void) gameObjData;
    (void) uuid;

    PlayerData* data = (PlayerData*)gameObjData;
    glm_vec3_copy((vec3){0.0f, 0.0f, 0.0f}, data->position);
}

void Player_Update(uuid_t uuid, void* gameState, void* gameObjData, float dt)
{
    (void) gameState;
    (void) gameObjData;
    (void) uuid;
        
    printf("[Player Script] deltaTime : %fms \n", dt * 1000.0f);
}