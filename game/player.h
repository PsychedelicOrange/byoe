#pragma once

#include <cglm/cglm.h>

#include <engine/core/gameobject.h>

typedef struct PlayerData
{
    uint32_t health;
} PlayerData;

void Player_Start(random_uuid_t* uuid);
void Player_Update(random_uuid_t* uuid, float dt);