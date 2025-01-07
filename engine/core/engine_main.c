#include <cglm/cglm.h>   /* for inline */
#include <cglm/struct.h> /* struct api */
#include <stddef.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>

#include "frustum.h"
#include "game_registry.h"
#include "game_state.h"
#include "gameobject.h"
#include "logging/log.h"
#include "rng/rng.h"
#include "scripting.h"
#include "shader.h"
#include "simd/platform_caps.h"
#include "../render/render_utils.h"
#include "../render/renderer_sdf.h"

#include <GLFW/glfw3.h>

// Forward declarations
static GLFWwindow* g_GameWindow;

// -- -- -- -- -- -- GAME MAIN -- -- -- -- -- --
// This is called by the client (game) to register game objects and their scripts
extern int game_main(void);
// -- -- -- -- -- -- Constants -- -- -- -- -- --
// settings
const uint32_t INIT_SCR_WIDTH  = 800;
const uint32_t INIT_SCR_HEIGHT = 600;
// -- -- -- -- -- -- -- -- --
int main(int argc, char** argv)
{
    (void) argc;
    (void) argv;

    srand((unsigned int) time(NULL));    // Seed for random number generator

    cpu_caps_print_info();
    os_caps_print_info();

    // Follow this order strictly to avoid load crashing
    render_utils_init_glfw();
    g_GameWindow = render_utils_create_glfw_window("BYOE Game: Spooky Asteroids!", INIT_SCR_WIDTH, INIT_SCR_HEIGHT);
    render_utils_init_glad();

    renderer_desc desc;
    desc.width = INIT_SCR_WIDTH;
    desc.height = INIT_SCR_HEIGHT;
    desc.window = g_GameWindow;
    bool success = renderer_sdf_init(desc);
    if(!success){
        LOG_ERROR("Error initializing SDF renderer");
        return -1;
    }

    ////////////////////////////////////////////////////////
    // START GAME RUNTIME
    game_registry_init();

    // register game objects on run-time side
    game_main();

    // game scripting start function
    gameobjects_start();
    ////////////////////////////////////////////////////////

    // render loop
    // -----------
    float deltaTime;             // Time between current frame and last frame
    float lastFrame   = 0.0f;    // Time of the last frame
    float elapsedTime = 0;

    int FPS = 0;

    while (!glfwWindowShouldClose(g_GameWindow)) {
        // Get current time
        float currentFrame = (float) glfwGetTime();
        deltaTime          = currentFrame - lastFrame;    // Calculate delta time
        // calculate FPS
        FPS = (int) (1.0f / deltaTime);

        elapsedTime += deltaTime;
        if (elapsedTime > 1.0f) {
            char windowTitle[250];
            sprintf(windowTitle, "BYOE Game: spooky asteroids! | FPS: %d | render dt: %2.2fms", FPS, deltaTime * 1000.0f);
            glfwSetWindowTitle(g_GameWindow, windowTitle);
            elapsedTime = 0.0f;
        }

        // Update the global game state
        gamestate_update(g_GameWindow);

        // Game scripts update loop
        gameobjects_update(deltaTime);

        // Render
        // ------
        renderer_sdf_render();
        // ------

        lastFrame = currentFrame;    // Update last frame time
    }
    renderer_sdf_destroy();
    game_registry_destroy();

    LOG_SUCCESS("Exiting game...");
    return 0;
}
