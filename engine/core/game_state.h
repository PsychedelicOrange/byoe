#pragma once

#include <stdalign.h>

#include <cglm/cglm.h>
#include <cglm/struct.h> /* struct api */

#include "../scene/camera.h"

struct GLFWwindow;

// TODO: Combine mousePos and lastMousePos into vec4 before and after perf-sweep for cache misses
typedef struct GameState
{
    // Mouse + Keyboard
    vec2   mousePos;
    vec2   lastMousePos;
    vec2   mouseDelta;
    bool   isMousePrimaryDown;
    bool   isKeyDown;
    bool   _pad0[6];
    bool   keycodes[512];
    Camera camera;
} GameState;

void       gamestate_update(struct GLFWwindow* window);
GameState* gamestate_get_global_instance(void);
