#include <cglm/cglm.h>   /* for inline */
#include <cglm/struct.h> /* struct api */
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

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

extern int game_main(void);
// -- -- -- -- -- -- Game state -- -- -- -- -- --- --
// refactor this later into global state/ or expose to script
vec4 rocks[100];
vec4 rockVelocities[100];
// -- -- -- -- -- -- Constants -- -- -- -- -- --- --
// settings
unsigned int SCR_WIDTH  = 1280;
unsigned int SCR_HEIGHT = 720;
// TODO: remove this later
unsigned int raymarchshader;

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
    //glfwSetKeyCallback(window,key_callback);
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
    if (glfwGetKey(window, GLFW_KEY_GRAVE_ACCENT)) {
        exit(0);
    }
    if (glfwGetKey(window, GLFW_KEY_SPACE)) {
        glDeleteProgram(raymarchshader);
        raymarchshader = create_shader("engine/shaders/simple_vert", "engine/shaders/raymarch");
        {
            int resolution[2] = {SCR_WIDTH, SCR_HEIGHT};
            setUniformVec2Int(raymarchshader, resolution, "resolution");
        }
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

// debug randomize rocks
void debug_randomize_rocks(void)
{
    rng_generate();
    for (int i = 0; i < 100; i++) {
        rocks[i][0] = (float) rng_range(1, 10);
        rocks[i][1] = (float) rng_range(1, 10);
        rocks[i][2] = (float) rng_range(1, 10);
        rocks[i][3] = 0.5;
    }
    rocks[1][0] = 1;
    rocks[1][1] = 6;
    rocks[1][2] = 1;
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
    unsigned int shaderProgram = create_shader("engine/shaders/vertex", "engine/shaders/frag");
    raymarchshader             = create_shader("engine/shaders/simple_vert", "engine/shaders/raymarch");
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
            sprintf(windowTitle, "BYOE Game: byoe_ghost_asteroids | FPS: %d | rendertime: %2.2fms", FPS, deltaTime * 1000.0f);
            glfwSetWindowTitle(window, windowTitle);
            elapsedTime = 0.0f;
        }

        //if (((int) glfwGetTime()) % 10 == 5) {
        //    debug_randomize_rocks();
        //}

        processInput(window);

        // Update the global game state
        gamestate_update(window);

        // Game scripts update loop
        gameobjects_update(deltaTime);

        {
            mat4s projection = glms_perspective(glm_rad(45.0f), (float) SCR_WIDTH / (float) SCR_HEIGHT, 0.1f, 1000.0f);
            setUniformMat4(shaderProgram, projection, "projection");
            setUniformMat4(raymarchshader, projection, "projection");
        }

        // Render
        // ------
        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        {
            glUseProgram(raymarchshader);
            int loc = glGetUniformLocation(raymarchshader, "rocks");
            glUniform4fv(loc, 100, &rocks[0][0]);

            int resolution[2] = {SCR_WIDTH, SCR_HEIGHT};
            setUniformVec2Int(raymarchshader, resolution, "resolution");

            Camera camera = gamestate_get_global_instance()->camera;
            // vec3 camera position
            loc          = glGetUniformLocation(raymarchshader, "cameraPos");
            vec3s camPos = camera.position;
            glUniform3f(loc, (GLfloat) camPos.x, (GLfloat) camPos.y, (GLfloat) camPos.z);

            // camera Pitch
            loc            = glGetUniformLocation(raymarchshader, "cameraPitch");
            float camPitch = glm_rad(camera.pitch);
            glUniform1f(loc, (GLfloat) camPitch);

            // Yaw
            loc          = glGetUniformLocation(raymarchshader, "cameraYaw");
            float camYaw = glm_rad(camera.yaw);
            glUniform1f(loc, (GLfloat) camYaw);

            loc        = glGetUniformLocation(raymarchshader, "cameraForward");
            vec3s camF = camera.front;
            glUniform3f(loc, (GLfloat) camF.x, (GLfloat) camF.y, (GLfloat) camF.z);

            setUniformMat4(raymarchshader, camera.lookAt, "viewMatrix");

            //loc            = glGetUniformLocation(raymarchshader, "cameraRight");
            //vec3s camRight = camera.right;
            //glUniform3f(loc, (GLfloat) camRight.x, (GLfloat) camRight.y, (GLfloat) camRight.z);

            glBindVertexArray(screen_quad_vao);
            glDrawArrays(GL_TRIANGLES, 0, 6);    // Drawing 6 vertices to form the quad
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
