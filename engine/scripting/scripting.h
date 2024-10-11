#ifndef SCRIPTING_H
#define SCRIPTING_H

// void* we will be passing the GameObject* itself, emulating "this"
// void* gameState, void* entityData
typedef void (*StartFunction)(void*, void*);
typedef void (*UpdateFunction)(void*, void*, float);

void gameobjects_start(void);
void gameobjects_update(float dt);

#endif
