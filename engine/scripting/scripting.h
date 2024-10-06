#ifndef SCRIPTING_H
#define SCRIPTING_H

// void* we will be passing the GameObject* itself, emulating "this"
typedef void (*StartFunction)(void*);
typedef void (*UpdateFunction)(void*, float);

#endif