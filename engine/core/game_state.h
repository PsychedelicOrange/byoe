#pragma once

#include <cglm/cglm.h>
#include <cglm/struct.h> /* struct api */

#include "../scene/camera.h"

struct GLFWwindow;

#define MAX_ROCKS_COUNT 100

// Note:- keep everything aligned to /16 bytes

typedef struct GameState
{
    // Mouse + Keyboard
    vec2     mousePos;
    vec2     lastMousePos;
    vec2     mouseDelta;
    bool     isMousePrimaryDown;
    bool     isKeyDown;
    bool     _padding1[2];
    uint32_t _padding2;
    bool     keycodes[512];
    Camera   camera;
    // Game context specific stuff
    vec4     rocks[MAX_ROCKS_COUNT];
    vec4     rocks_visible[MAX_ROCKS_COUNT];
    int      rocks_visible_count;
} GameState;

void       gamestate_update(struct GLFWwindow* window);
GameState* gamestate_get_global_instance(void);
