#pragma once

#include "gameobject.h"

typedef struct GameState
{
    vec2 mousePos;
} GameState;

GameState gGlobalGameState;

static void startGameObjects()
{
    // Call start functions for all registered game objects
    for (int i = 0; i < gNumObjects; i++) {
        gGameRegistry[i].value.startFn(&gGlobalGameState, gGameRegistry[i].value.gameObjectData); // Pass game state and game object data as needed
    }
}

static void updateGameObjects(float dt)
{
    // Call start functions for all registered game objects
    for (int i = 0; i < gNumObjects; i++) {
        gGameRegistry[i].value.updateFn(&gGlobalGameState, gGameRegistry[i].value.gameObjectData, dt);
    }
}