#pragma once

#include <cglm/cglm.h>

// Note:- keep everything aligned to /16 bytes

typedef struct GameState
{
    // Mouse + Keyboard
    vec2 mousePos;
    bool isMouseDown;
    bool isKeyDown;
    bool _padding1[2];
    uint32_t _padding2;
    bool keycodes[256];
    // TBD...
} GameState;

GameState gGlobalGameState;

void update_game_state(void);