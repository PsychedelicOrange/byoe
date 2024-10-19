#ifndef SCRIPTING_H
#define SCRIPTING_H

#include "uuid/uuid.h"

// We will use uuid to access gameobject data inside these functions
// gGlobalGameState can be used directly or using gamestate_get_global_instance()
typedef void (*StartFunction)(random_uuid_t*);
typedef void (*UpdateFunction)(random_uuid_t*, float);

void gameobjects_start(void);
void gameobjects_update(float dt);

#endif
