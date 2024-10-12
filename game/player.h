#pragma once 

#include<cglm/cglm.h>

#include <engine/core/gameobject.h>

typedef struct PlayerData
{
    vec3 position;
    vec3 rotation;
    uint32_t health;
} PlayerData;

extern uuid_t playerUUID;

void Player_Start(void* gameState, void* gameObjData);
void Player_Update(void* gameState, void* gameObjData, float dt);