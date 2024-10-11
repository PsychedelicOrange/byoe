#include "scripting.h"

#include "gameobject.h"
#include "game_registry.h"
#include "game_state.h"

void gameobjects_start(void)
{
    hash_map_iterator_t it = hash_map_iterator_begin(gGameRegistry);
    for (size_t i = 0; i < gGameRegistry->length; i++)
    {
        hash_map_parse_next(&it);
        GameObject* gameobject = hash_map_get_value(gGameRegistry,  it.current_pair->key);
        if(gameobject)
            gameobject->startFn(&gGlobalGameState, gameobject->gameObjectData);
    }
}

void gameobjects_update(float dt)
{
    hash_map_iterator_t it = hash_map_iterator_begin(gGameRegistry);
    for (size_t i = 0; i < gGameRegistry->length; i++)
    {
        hash_map_parse_next(&it);
        GameObject* gameobject = hash_map_get_value(gGameRegistry,  it.current_pair->key);
        if(gameobject)
            gameobject->updateFn(&gGlobalGameState, gameobject->gameObjectData, dt);
    }
}
