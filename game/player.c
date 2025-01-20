
#include "player.h"

#include <core/rng/rng.h>
#include <engine/core/game_state.h>
#include <logging/log.h>

#include <GLFW/glfw3.h>
#include <assert.h>

#include "camera_controller.h"

static int PLAYER_SPEED = 10;

void Player_Start(random_uuid_t* uuid)
{
    (void) uuid;

    // Ex. API usage:
    // gameobject_mark_as_renderable(*uuid, true);
    // gameobject_set_scale(*uuid, newScale);

    Camera_set_player_uuid(*uuid);
}

void Player_Update(random_uuid_t* uuid, float dt)
{
    (void) uuid;
    (void) dt;

    GameState* gameState      = gamestate_get_global_instance();
    vec3s      playerPosition = {0};
    gameobject_get_position(*uuid, &playerPosition.raw);
    if (gameState->keycodes[GLFW_KEY_DOWN])
        playerPosition.z += PLAYER_SPEED * dt;
    if (gameState->keycodes[GLFW_KEY_UP])
        playerPosition.z -= PLAYER_SPEED * dt;

    //playerPosition.z = -5.0f;

    gameobject_set_position(*uuid, playerPosition.raw);
}