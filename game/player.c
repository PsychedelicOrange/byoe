#include "player.h"

#include <logging/log.h>
#include <core/rng/rng.h>

#include <assert.h>


#define FNV_offset_basis    14695981039346656037UL
#define FNV_prime           1099511628211UL

// static uint64_t fnv1a_hash(uint8_t* key)
// {
//     uint64_t hash = FNV_offset_basis;
//     while (*key) {
//         hash ^= (uint64_t)(unsigned char*)(key); // bitwise XOR
//         hash *= FNV_prime;                       // multiply
//         key++;
//     }
//     return hash;
// }

void Player_Start(random_uuid_t* uuid)
{
    (void)uuid;

    // Ex. API usage: 
    // gameobject_mark_as_renderable(*uuid, true);
    // gameobject_set_scale(*uuid, newScale);

    // Generate random movement offsets for x, y, z
    float offsetX = (float)((int32_t)rng_range(0, 50) * 2 - 50) / 10.0f;
    float offsetY = (float)((int32_t)rng_range(0, 70) * 2 - 70) / 10.0f;
    //float offsetZ = ((rand() % 200) - 100) / 100.0f;

    // Calculate new chaotic position based on offsets and dt
    vec3 newPosition = {
        offsetX,
        offsetY,
        0
    };;

    // uint64_t hash = fnv1a_hash((uint8_t*)uuid->data);
    // size_t index = (size_t)(hash & (uint64_t)(MAX_OBJECTS - 1));
    // LOG_WARN("Player UUID Key index: %zu", index);

    // LOG_INFO("Player pos : (%f, %f, %f)\n", newPosition[0], newPosition[1], newPosition[2]);

    gameobject_set_position(*uuid, newPosition);
    // LOG_SUCCESS("-----------------------------\n");
}

void Player_Update(random_uuid_t* uuid, float dt)
{
    (void)uuid;
    (void)dt;



}