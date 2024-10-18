#include "player.h"

#include <logging/log.h>
#include <core/rng/rng.h>

#include <assert.h>

void Player_Start(uuid_t* uuid)
{
    (void)uuid;

    gameobject_mark_as_renderable(*uuid, true);

    // Ex. API usage: 
    // gameobject_set_scale(*uuid, newScale);
}

void Player_Update(uuid_t* uuid, float dt)
{
    (void)uuid;
    (void)dt;

    vec3 position;
    gameobject_get_position(*uuid, &position);

     // Generate random movement offsets for x, y, z
    float offsetX = (float)((int32_t)rng_range(20, 50) * 2 - 50) / 10.0f ;
    float offsetY = (float)((int32_t)rng_range(20, 70) * 2 - 70) / 10.0f;
    //float offsetZ = ((rand() % 200) - 100) / 100.0f;

    // Calculate new chaotic position based on offsets and dt
    vec3 newPosition = {
        offsetX * 25,
        offsetY * 25,
        0
    };;

    LOG_INFO("Player pos : (%f, %f, %f)\n", newPosition[0], newPosition[1], newPosition[2]);

    uuid_print(uuid);
    gameobject_set_position(*uuid, newPosition);
}