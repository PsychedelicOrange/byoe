#include "gameobject.h"

const GameObject* registerGameObjectType(const char* typeName, void* gameObjectData, StartFunction start, UpdateFunction update)
{
    // TODO: either use sutom byow_malloc or re-use memory by pre-allocating object in gGameRegistry array/hash_map
    // This is just a test not the right way to allocate memory.
    GameObject* obj = (GameObject*)malloc(sizeof(GameObject));

    obj->typeName = typeName;
    obj->name = "GameObject";
    obj->startFn = start;
    obj->updateFn = update;

    return obj;
}

void unRegisterGameObjectType(GameObject* obj)
{
    // TODO...
}