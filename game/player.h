#pragma once 

#include<cglm/cglm.h>

#include <engine/core/gameobject.h>

typedef struct PlayerData
{
    uint32_t health;
} PlayerData;

void Player_Start(void* gameState, void* gameObjData);
void Player_Update(void* gameState, void* gameObjData, float dt);