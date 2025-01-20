#include "render_utils.h"

#include "logging/log.h"

#include <stdio.h>
#include <stdlib.h>

// clang-format off
#include <glad/glad.h>
#include <GLFW/glfw3.h>
// clang-format on

void render_utils_init_glfw(void)
{
    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

#ifdef __APPLE__
    glfwWindowHint(GLFW_COCOA_RETINA_FRAMEBUFFER, GLFW_FALSE);
#endif
}

void render_utils_crash_game(char* msg)
{
    LOG_ERROR("Game crashed: %s", msg);
    fflush(stdout);
    exit(1);
}

GLFWwindow* render_utils_create_glfw_window(const char* title, uint32_t width, uint32_t height)
{
    GLFWwindow* window = glfwCreateWindow(width, height, title, NULL, NULL);
    if (window == NULL) {
        render_utils_crash_game("Unable to create glfw window");
        glfwTerminate();
    }
    glfwMakeContextCurrent(window);
    if (glfwRawMouseMotionSupported())
        glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
    // Enable this if you don't want the cursor to be shown
    //glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    // Disable V-Sync
    glfwSwapInterval(0);
    return window;
}