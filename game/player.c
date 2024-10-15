#include "player.h"

#include <logging/log.h>

uuid_t playerUUID;

void Player_Start(void* gameState, void* gameObjData)
{
    (void) gameState;
    (void) gameObjData;

    PlayerData* data = (PlayerData*)gameObjData;
    glm_vec3_copy((vec3){0.0f, 0.0f, 0.0f}, data->position);
}

void Player_Update(void* gameState, void* gameObjData, float dt)
{
    (void) gameState;
    (void) gameObjData;

    LOG_WARN("[Player Script] deltaTime: %fms", dt * 1000.0f);
}