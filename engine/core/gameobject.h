#ifndef GAMEOBJECTS_H
#define GAMEOBJECTS_H

#include <stdint.h>

#include "../scripting/scripting.h"

#define MAX_OBJECTS 100

typedef struct GameObject
{
    const char* name;
    const char* typeName;
    void* data;
    StartFunction start;
    UpdateFunction update;
} GameObject;


static GameObject gGameRegistry[MAX_OBJECTS];
static uint32_t gNumObjects = 0;

/*******************************/
// GameObjects Registry
/*******************************/
void registerGameObjectType(const char* typeName, StartFunction start, UpdateFunction update);

#endif
