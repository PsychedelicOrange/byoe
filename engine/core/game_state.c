#include "game_state.h"

#include <GLFW/glfw3.h>

GameState gGlobalGameState;

void gamestate_update(struct GLFWwindow* window)
{
    // Update mouse position
    double mouseX, mouseY;
    glfwGetCursorPos(window, &mouseX, &mouseY);
    gGlobalGameState.mousePos[0] = (float) mouseX;
    gGlobalGameState.mousePos[1] = (float) mouseY;

    // update mouse delta
    gGlobalGameState.mouseDelta[0] = gGlobalGameState.lastMousePos[0] - (float) mouseX;
    gGlobalGameState.mouseDelta[1] = gGlobalGameState.lastMousePos[1] - (float) mouseY;

    // Update mouse button state
    gGlobalGameState.isMousePrimaryDown = (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS);

    // Update keyboard state
    gGlobalGameState.isKeyDown = false;    // Reset key down flag
    for (int i = 0; i < GLFW_KEY_LAST; ++i) {
        int state                    = glfwGetKey(window, i);
        gGlobalGameState.keycodes[i] = (state == GLFW_PRESS || state == GLFW_REPEAT);

        if (gGlobalGameState.keycodes[i]) {
            gGlobalGameState.isKeyDown = true;    // At least one key is down
        }
    }

    gGlobalGameState.lastMousePos[0] = (float) mouseX;
    gGlobalGameState.lastMousePos[1] = (float) mouseY;
}

GameState* gamestate_get_global_instance(void)
{
    return &gGlobalGameState;
}