#include "game_state.h"

#include <GLFW/glfw3.h>

static GameState s_GlobalGameState;

void gamestate_update(struct GLFWwindow* window)
{
    // Update mouse position
    double mouseX, mouseY;
    glfwGetCursorPos(window, &mouseX, &mouseY);
    s_GlobalGameState.mousePos[0] = (float) mouseX;
    s_GlobalGameState.mousePos[1] = (float) mouseY;

    // update mouse delta
    s_GlobalGameState.mouseDelta[0] = s_GlobalGameState.lastMousePos[0] - (float) mouseX;
    s_GlobalGameState.mouseDelta[1] = s_GlobalGameState.lastMousePos[1] - (float) mouseY;

    // Update mouse button state
    s_GlobalGameState.isMousePrimaryDown = (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS);

    // Update keyboard state
    s_GlobalGameState.isKeyDown = false;    // Reset key down flag
    for (int i = 0; i < GLFW_KEY_LAST; ++i) {
        int state                     = glfwGetKey(window, i);
        s_GlobalGameState.keycodes[i] = (state == GLFW_PRESS || state == GLFW_REPEAT);

        if (s_GlobalGameState.keycodes[i]) {
            s_GlobalGameState.isKeyDown = true;    // At least one key is down
        }
    }

    s_GlobalGameState.lastMousePos[0] = (float) mouseX;
    s_GlobalGameState.lastMousePos[1] = (float) mouseY;
}

GameState* gamestate_get_global_instance(void)
{
    return &s_GlobalGameState;
}