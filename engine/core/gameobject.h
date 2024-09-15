#ifndef GAMEOBJECTS_H
#define GAMEOBJECTS_H

#include <stdint.h>

#define MAX_OBJECTS 100

typedef void (*StartFunction)(void*);
typedef void (*UpdateFunction)(void*, float);

typedef struct GameObject
{
    const char* name;
    const char* typeName;
    void* data;
    StartFunction start;
    UpdateFunction update;
} GameObject;


GameObject gGameRegistry[MAX_OBJECTS];
uint32_t gNumObjects = 0;

#endif
