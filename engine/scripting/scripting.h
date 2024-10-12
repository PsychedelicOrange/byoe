#ifndef SCRIPTING_H
#define SCRIPTING_H

struct uuid_t;

// void* we will be passing the GameObject* itself, emulating "this"
// void* gameState, void* entityData
typedef void (*StartFunction)(struct uuid_t uuid, void*, void*);
typedef void (*UpdateFunction)(struct uuid_t uuid, void*, void*, float);

void gameobjects_start(void);
void gameobjects_update(float dt);

#endif
