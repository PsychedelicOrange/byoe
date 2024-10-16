#include "player.h"

#include <logging/log.h>

uuid_t playerUUID;

void Player_Start(void* gameState, void* gameObjData)
{
    (void)gameState;
    (void)gameObjData;

    gameobject_mark_as_renderable(playerUUID, true);

    // Generate random movement offsets for x, y, z
    float offsetX = ((rand() % 200) - 100) / 10.0f;
    float offsetY = ((rand() % 200) - 100) / 10.0f;
    //float offsetZ = ((rand() % 200) - 100) / 100.0f;

    // Calculate new chaotic position based on offsets and dt
    vec3 newPosition = {
        offsetX,
        offsetY,
        0
    };

    gameobject_set_position(playerUUID, newPosition);
}

void Player_Update(void* gameState, void* gameObjData, float dt)
{
    (void)gameState;
    (void)gameObjData;
    (void)dt;

    vec3 position;
    gameobject_get_position(playerUUID, &position);

    LOG_WARN("[Player Script] position: (%f, %f, %f)", position[0], position[1], position[2]);
}