#pragma once 

#include<cglm/cglm.h>

#include <engine/core/gameobject.h>

typedef struct PlayerData
{
    uint32_t health;
} PlayerData;

void Player_Start(uuid_t* uuid);
void Player_Update(uuid_t* uuid, float dt);