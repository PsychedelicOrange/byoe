#include "scripting.h"

#include "gameobject.h"
#include "game_registry.h"
#include "game_state.h"

void gameobjects_start(void)
{
    hash_map_iterator_t it = hash_map_iterator_begin(gGameRegistry);
    while (hash_map_parse_next(&it))
    {
        GameObject* go = (GameObject*)it.current_pair.value;
        if (go)
            go->startFn(&gGlobalGameState, go->gameObjectData);
    }
}

void gameobjects_update(float dt)
{
    hash_map_iterator_t it = hash_map_iterator_begin(gGameRegistry);
    while (hash_map_parse_next(&it))
    {
        GameObject* go = (GameObject*)it.current_pair.value;
        if (go)
            go->updateFn(&gGlobalGameState, go->gameObjectData, dt);
    }
}
