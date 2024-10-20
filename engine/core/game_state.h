#pragma once

#include <cglm/cglm.h>
#include <cglm/struct.h> /* struct api */

struct GLFWwindow;

// Note:- keep everything aligned to /16 bytes

typedef struct Camera
{
    mat4s lookAt;
    vec3s position;
    vec3s right;
    vec3s front;
} Camera;

typedef struct GameState
{
    // Mouse + Keyboard
    vec2     mousePos;
    bool     isMouseDown;
    bool     isKeyDown;
    bool     _padding1[2];
    uint32_t _padding2;
    bool     keycodes[256];
    Camera   camera;
    // TBD...
} GameState;

void       gamestate_update(struct GLFWwindow* window);
GameState* gamestate_get_global_instance(void);
