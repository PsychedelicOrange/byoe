#include "player.h"

#include <core/rng/rng.h>
#include <logging/log.h>

#include <assert.h>

void Player_Start(random_uuid_t* uuid)
{
    (void) uuid;

    // Ex. API usage:
    // gameobject_mark_as_renderable(*uuid, true);
    // gameobject_set_scale(*uuid, newScale);
}

void Player_Update(random_uuid_t* uuid, float dt)
{
    (void) uuid;
    (void) dt;

    // Generate random movement offsets for x, y, z
    // float offsetX = (float) ((int32_t) rng_range(0, 50) * 2 - 50) / 10.0f;
    // float offsetY = (float) ((int32_t) rng_range(0, 70) * 2 - 70) / 10.0f;
    //float offsetZ = ((rand() % 200) - 100) / 100.0f;

    // Calculate new chaotic position based on offsets and dt
    // vec3 newPosition = {
    //     sin(offsetX * dt * 25),
    //     cos(offsetY * dt * 25),
    //     0};

    // gameobject_set_position(*uuid, newPosition);
    gameobject_set_scale(*uuid, (vec3){0.25f, 0.1f, 0.5f});
}