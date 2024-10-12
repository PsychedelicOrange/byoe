#ifndef SCRIPTING_H
#define SCRIPTING_H

// void* we will be having the UUID from the header, we will use that to access gameobject inside these functions
// void* gameState, void* entityData
typedef void (*StartFunction)(void*, void*);
typedef void (*UpdateFunction)(void*, void*, float);

void gameobjects_start(void);
void gameobjects_update(float dt);

#endif
