#include "scripting.h"

#include "gameobject.h"
#include "game_registry.h"
#include "game_state.h"

void gameobjects_start(void)
{
    for (size_t i = 0; i < gGameRegistry->capacity; i++)
    {
       hash_map_pair_t pair = gGameRegistry->entries[i];
       if(pair.key && pair.value) {
            GameObject* go = (GameObject*)pair.value;
            go->startFn(go->uuid, &gGlobalGameState, go->gameObjectData);
       }
    }
}

void gameobjects_update(float dt)
{
    for (size_t i = 0; i < gGameRegistry->capacity; i++)
    {
       hash_map_pair_t pair = gGameRegistry->entries[i];
       if(pair.key && pair.value) {
            GameObject* go = (GameObject*)pair.value;
            go->updateFn(go->uuid, &gGlobalGameState, go->gameObjectData, dt);
       }
    }
}
