#include "player.h"

#include <logging/log.h>

void Player_Start(uuid_t* uuid)
{
    (void)uuid;

    gameobject_mark_as_renderable(*uuid, true);

    // Generate random movement offsets for x, y, z
    float offsetX = ((rand() % 200) - 100) / 10.0f;
    float offsetY = ((rand() % 200) - 100) / 10.0f;
    //float offsetZ = ((rand() % 200) - 100) / 100.0f;

    // Calculate new chaotic position based on offsets and dt
    vec3 newPosition = {
        offsetX,
        offsetY,
        0
    };;

    gameobject_set_position(*uuid, newPosition);
    //gameobject_set_scale(*uuid, newScale);
}

void Player_Update(uuid_t* uuid, float dt)
{
    (void)uuid;
    (void)dt;

    vec3 position;
    gameobject_get_position(*uuid, &position);
}