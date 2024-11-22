#include <cglm/cglm.h>   /* for inline */
#include <cglm/struct.h> /* struct api */
#include <stddef.h>
#include <stdio.h>
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
#include "utils.h"

// Put them at last: causing some weird errors while compiling on MSVC with APIENTRY define
// Also follow this order as it will cause openlg include error
// clang-format off
#include <glad/glad.h>
#include <GLFW/glfw3.h>
// clang-format on
// max values
#define MAX_ROCKS_COUNT 100

extern int game_main(void);
// -- -- -- -- -- -- Game state -- -- -- -- -- --- --
// refactor this later into global state/ or expose to script
vec4 rocks[100];
vec4 rockVelocities[100];
// -- -- -- -- -- -- Constants -- -- -- -- -- --- --
// settings
unsigned int SCR_WIDTH  = 800;
unsigned int SCR_HEIGHT = 600;

// TODO: remove this later
unsigned int raymarchshader;

// -- -- -- -- -- -- Game state -- -- -- -- -- --- --
// refactor this later into global state/ or expose to script
vec4 rocks[MAX_ROCKS_COUNT];
vec4 rocks_visible[MAX_ROCKS_COUNT];
int  rocks_visible_count = 0;

// -- -- function declare
//
GLFWwindow* create_glfw_window(void);
void        framebuffer_size_callback(GLFWwindow* window, int width, int height);
void        processInput(GLFWwindow* window);
void        key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);

// -- -- function define
//
GLFWwindow* create_glfw_window(void)
{
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "psychspiration", NULL, NULL);
    if (window == NULL) {
        crash_game("Unable to create glfw window");
        glfwTerminate();
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    if (glfwRawMouseMotionSupported())
        glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
    //glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    // Disable V-Sync
    glfwSwapInterval(0);
    return window;
}

void gl_settings(void)
{
    glEnable(GL_DEPTH_TEST);
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    (void) window;
    SCR_WIDTH  = width;
    SCR_HEIGHT = height;
    // make sure the viewport matches the new window dimensions; note that width and
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}

void processInput(GLFWwindow* window)
{
    (void) window;
     if(glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_SPACE)) {
        glDeleteProgram(raymarchshader);
        raymarchshader = create_shader("engine/shaders/simple_vert", "engine/shaders/raymarch");
    }
}

// ig we can just define this function from script and set callback from the script
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    (void) window;
    (void) key;
    (void) scancode;
    (void) action;
    (void) mods;
}
// -- -- -- -- -- -- -- -- --

void print_vec3(float* vec)
{
    printf("(%f,%f,%f)", vec[0], vec[1], vec[2]);
}
// temp move to scripting side
void debug_randomize_rocks(void)
{
    rng_generate();
    for (int i = 0; i < MAX_ROCKS_COUNT; i++) {
        rocks[i][0] = (float) rng_range(1, 10);
        rocks[i][1] = (float) rng_range(1, 10);
        rocks[i][2] = (float) rng_range(1, 10);
        rocks[i][0] -= 5;
        rocks[i][1] -= 5;
        rocks[i][2] -= 5;
        rocks[i][3] = 0.5;
    }
    rocks[1][0] = 1;
    rocks[1][1] = 1;
    rocks[1][2] = -2;
    rocks[0][0] = 1;
    rocks[0][1] = 1;
    rocks[0][2] = -6;
}

int main(int argc, char** argv)
{
    (void) argc;
    (void) argv;

    srand((unsigned int) time(NULL));    // Seed for random number generator

    cpu_caps_print_info();
    os_caps_print_info();

    init_glfw();
    GLFWwindow* window = create_glfw_window();
    init_glad();
    gl_settings();
    raymarchshader = create_shader("engine/shaders/simple_vert", "engine/shaders/raymarch");
    // set constant shader variables
    {
        int resolution[2] = {SCR_WIDTH, SCR_HEIGHT};
        setUniformVec2Int(raymarchshader, resolution, "resolution");
    }

    GLuint screen_quad_vao = setup_screen_quad();

    debug_randomize_rocks();

    ////////////////////////////////////////////////////////
    // START GAME RUNTIME
    init_game_registry();

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

    while (!glfwWindowShouldClose(window)) {
        // Get current time
        float currentFrame = (float) glfwGetTime();
        deltaTime          = currentFrame - lastFrame;    // Calculate delta time
        // calculate FPS
        FPS = (int) (1.0f / deltaTime);

        elapsedTime += deltaTime;
        if (elapsedTime > 1.0f) {
            char windowTitle[250];
            sprintf(windowTitle, "BYOE Game: byoe_ghost_asteroids | FPS: %d | render time: %2.2fms", FPS, deltaTime * 1000.0f);
            glfwSetWindowTitle(window, windowTitle);
            elapsedTime = 0.0f;
        }

        processInput(window);

        // Update the global game state
        gamestate_update(window);

        // Game scripts update loop
        gameobjects_update(deltaTime);

        // Render
        // ------
        // frustum cull

        Camera* camera = &gamestate_get_global_instance()->camera;

        int resolution[2] = {SCR_WIDTH, SCR_HEIGHT};
        setUniformVec2Int(raymarchshader, resolution, "resolution");

        mat4s projection    = glms_perspective(camera->fov, (float) SCR_WIDTH / (float) SCR_HEIGHT, camera->near_plane, camera->far_plane);
        mat4s viewproj      = glms_mul(projection, gamestate_get_global_instance()->camera.lookAt);
        rocks_visible_count = cull_rocks(MAX_ROCKS_COUNT, rocks, rocks_visible, viewproj.raw);

        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        {
            glUseProgram(raymarchshader);
            int loc = glGetUniformLocation(raymarchshader, "rocks");
            glUniform4fv(loc, rocks_visible_count, &rocks_visible[0][0]);
            setUniformMat4(raymarchshader, viewproj, "viewproj");

            for (int i = 0; i < rocks_visible_count; i++) {
                glUseProgram(raymarchshader);
                loc = glGetUniformLocation(raymarchshader, "rocks_idx");
                glUniform1i(loc, i);

                glBindVertexArray(screen_quad_vao);
                glDrawArrays(GL_TRIANGLES, 0, 6);    // Drawing 6 vertices to form the quad
            }
        }

        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        // -------------------------------------------------------------------------------
        glfwSwapBuffers(window);
        glfwPollEvents();

        lastFrame = currentFrame;    // Update last frame time
    }

    cleanup_game_registry();

    // glfw: terminate, clearing all previously allocated GLFW resources.
    // ------------------------------------------------------------------
    glfwTerminate();
    printf("\nbye!\n");
    return 0;
}
