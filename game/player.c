#include <engine/core/gameobject.h>

#include<cglm/cglm.h>

typedef struct PlayerData
{
    vec3 position;
    vec3 rotation;
    uint32_t health;
} PlayerData;

void Player_Start(void* gameState, void* gameObjData)
{
    PlayerData* data = (PlayerData*)gameObjData;
    glm_vec3_copy((vec3){0.0f, 0.0f, 0.0f}, data->position);
}

void Player_Update(void* gameState, void* gameObjData, float dt)
{

}

uuid_t playerUUID;

void init()
{
    playerUUID = registerGameObjectType("Player", sizeof(PlayerData), Player_Start, Player_Update);
}
