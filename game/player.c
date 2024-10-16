#include "player.h"

#include <logging/log.h>
#include <core/rng/rng.h>

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
    float offsetX = (float)((int32_t)rng_range(0, 50) * 2 - 50) / 10.0f ;
    float offsetY = (float)((int32_t)rng_range(0, 70) * 2 - 70) / 10.0f;
    //float offsetZ = ((rand() % 200) - 100) / 100.0f;

    // Calculate new chaotic position based on offsets and dt
    vec3 newPosition = {
        offsetX * dt * 25,
        offsetY * dt * 25,
        0
    };;

    gameobject_set_position(*uuid, newPosition);
}