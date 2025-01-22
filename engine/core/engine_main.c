#include <cglm/cglm.h>   /* for inline */
#include <cglm/struct.h> /* struct api */
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include "../engine.h"

#include <GLFW/glfw3.h>

// Forward declarations
static GLFWwindow* g_GameWindow;

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

    engine_init(&g_GameWindow, INIT_SCR_WIDTH, INIT_SCR_HEIGHT);

    // render loop
    // -----------
    engine_run();

    engine_destroy();

    LOG_SUCCESS("Exiting game...");
    return 0;
}
