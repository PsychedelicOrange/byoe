#include "engine.h"

#include <GLFW/glfw3.h>

static float       deltaTime;
static float       lastFrame       = 0.0f;
static float       elapsedTime     = 0;
static int         FPS             = 0;
static GLFWwindow* g_GameWindowRef = NULL;

static engine_version s_EngineVersion = {0, 1, 0, ""};
static char           s_VersionString[64];

void engine_init(struct GLFWwindow** gameWindow, uint32_t width, uint32_t height)
{
    LOG_SUCCESS("Welcome to Build your own engine! BYOE!!");
    LOG_SUCCESS("\tversion: %s", engine_get_version_string());

    // cpu_caps_print_info();
    // os_caps_print_info();

    // Follow this order strictly to avoid load crashing
    render_utils_init_glfw();
    *gameWindow = render_utils_create_glfw_window("BYOE Game: Spooky Asteroids!", width, height);
    render_utils_init_glad();

    renderer_desc desc = {0};
    desc.width         = width;
    desc.height        = height;
    desc.window        = *gameWindow;
    bool success       = renderer_sdf_init(desc);
    if (!success) {
        LOG_ERROR("Error initializing SDF renderer");
        exit(-1);
    }

    g_GameWindowRef = *gameWindow;

    ////////////////////////////////////////////////////////
    // START GAME RUNTIME
    game_registry_init();

    // register game objects on run-time side
    game_main();

    gameobjects_start();
    ////////////////////////////////////////////////////////
}

void engine_destroy(void)
{
    renderer_sdf_destroy();
    game_registry_destroy();
    LOG_SUCCESS("Exiting BYOE...Byee!");
}

bool engine_should_quit(void)
{
    return glfwWindowShouldClose(g_GameWindowRef);
}

void engine_request_quit(void)
{
    if (g_GameWindowRef)
        glfwSetWindowShouldClose(g_GameWindowRef, true);
}

void engine_run(void)
{
    while (!engine_should_quit()) {
        float currentFrame = (float) glfwGetTime();
        deltaTime          = currentFrame - lastFrame;
        FPS                = (int) (1.0f / deltaTime);

        elapsedTime += deltaTime;
        if (elapsedTime > 1.0f) {
            char windowTitle[250];
            sprintf(windowTitle, "BYOE Game: spooky asteroids! | FPS: %d | render dt: %2.2fms", FPS, deltaTime * 1000.0f);
            glfwSetWindowTitle(g_GameWindowRef, windowTitle);
            elapsedTime = 0.0f;
        }

        gamestate_update(g_GameWindowRef);

        gameobjects_update(deltaTime);

        // ------
        renderer_sdf_render();
        // ------

        lastFrame = currentFrame;
    }
}

float engine_get_delta_time(void)
{
    return deltaTime;
}

float engine_get_last_frame(void)
{
    return lastFrame;
}

float engine_get_elapsed_time(void)
{
    return elapsedTime;
}

int engine_get_fps(void)
{
    return FPS;
}

engine_version engine_get_version(void)
{
    return s_EngineVersion;
}

const char* engine_get_version_string(void)
{
    // Format the version string
    if (s_EngineVersion.build[0] != '\0') {
        snprintf(s_VersionString, sizeof(s_VersionString), "%u.%u.%u-%s", s_EngineVersion.major, s_EngineVersion.minor, s_EngineVersion.patch, s_EngineVersion.build);
    } else {
        snprintf(s_VersionString, sizeof(s_VersionString), "%u.%u.%u", s_EngineVersion.major, s_EngineVersion.minor, s_EngineVersion.patch);
    }
    return s_VersionString;
}

// Get the major version
uint16_t engine_get_major_version(void)
{
    return s_EngineVersion.major;
}

// Get the minor version
uint16_t engine_get_minor_version(void)
{
    return s_EngineVersion.minor;
}

// Get the patch version
uint16_t engine_get_patch_version(void)
{
    return s_EngineVersion.patch;
}

// Get the build metadata
const char* engine_get_build_metadata(void)
{
    return s_EngineVersion.build;
}