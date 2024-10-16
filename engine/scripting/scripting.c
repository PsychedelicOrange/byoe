#include "scripting.h"

#include "gameobject.h"
#include "game_registry.h"
#include "game_state.h"

void gameobjects_start(void)
{
    for (size_t i = 0; i < game_registry_get_instance()->capacity; i++)
    {
       hash_map_pair_t pair = game_registry_get_instance()->entries[i];
       if(pair.key && pair.value) {
            GameObject* go = (GameObject*)pair.value;
            go->startFn(go->uuid);
       }
    }
}

void gameobjects_update(float dt)
{
    for (size_t i = 0; i < game_registry_get_instance()->capacity; i++)
    {
       hash_map_pair_t pair = game_registry_get_instance()->entries[i];
       if(pair.key && pair.value) {
            GameObject* go = (GameObject*)pair.value;
            go->updateFn(go->uuid, dt);
       }
    }
}
