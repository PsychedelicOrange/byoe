#include "player.h"

#include <logging/log.h>

void Player_Start(uuid_t* uuid)
{
    (void)uuid;

    gameobject_mark_as_renderable(*uuid, true);

    // Generate random movement offsets for x, y, z
    float offsetX = ((rand() % 200) - 100) / 100.0f;
    float offsetY = ((rand() % 200) - 100) / 100.0f;
    //float offsetZ = ((rand() % 200) - 100) / 100.0f;

    // Calculate new chaotic position based on offsets and dt
    vec3 newPosition = {
        offsetX,
        offsetY,
        0
    };

    LOG_INFO("position: (%f, %f, %f)\n", newPosition[0], newPosition[1], newPosition[2]);

    gameobject_set_position(*uuid, newPosition);
}

void Player_Update(uuid_t* uuid, float dt)
{
    (void)uuid;
    (void)dt;

    vec3 position;
    gameobject_get_position(*uuid, &position);
}