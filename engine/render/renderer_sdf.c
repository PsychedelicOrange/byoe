#include "renderer_sdf.h"

#include "rng/rng.h"
#include "render_utils.h"
#include "game_state.h"
#include "shader.h"
#include "frustum.h"

#include "logging/log.h"

// Put them at last: causing some weird errors while compiling on MSVC with APIENTRY define
// Also follow this order as it will cause openlg include error
// clang-format off
#include <glad/glad.h>
#include <GLFW/glfw3.h>
// clang-format on

// All rendering API and GLFW code goes in here

typedef struct renderer_internal_state
{
    uint32_t numPrimitives;
    uint32_t width;
    uint32_t height;
    uint32_t raymarchShaderID;
    GLFWwindow* window;
    uint32_t screen_quad_vao;
    uint64_t frameCount;
}renderer_internal_state;

//---------------------------------------------------------
static renderer_internal_state g_RendererSDFInternalState;
//---------------------------------------------------------
//-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- 
// Private functions 
// Resize callback
static void renderer_internal_sdf_resize(GLFWwindow* window, int width, int height)
{
    (void) window;
    g_RendererSDFInternalState.width = width;
    g_RendererSDFInternalState.height = height;
    g_RendererSDFInternalState.frameCount = 0;
    
    glViewport(0, 0, width, height);
}

static void renderer_internal_sdf_hot_reload_shaders(void)
{
    // reload the raymarch shader
    glDeleteProgram(g_RendererSDFInternalState.raymarchShaderID);
    g_RendererSDFInternalState.raymarchShaderID = create_shader("engine/shaders/simple_vert", "engine/shaders/raymarch");
}

static void renderer_internal_sdf_clear_screen(color_rgba color)
{
    glClearColor(color.r, color.g, color.b, color.a);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

//-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- 
bool renderer_sdf_init(renderer_desc desc)
{
    // use the desc to init the internal state
    (void) desc;

    g_RendererSDFInternalState.numPrimitives = 0;
    g_RendererSDFInternalState.width = desc.width;
    g_RendererSDFInternalState.height = desc.height;
    g_RendererSDFInternalState.window = desc.window;
    g_RendererSDFInternalState.frameCount = 0;

    g_RendererSDFInternalState.raymarchShaderID = create_shader("engine/shaders/simple_vert", "engine/shaders/raymarch");
    g_RendererSDFInternalState.screen_quad_vao = render_utils_setup_screen_quad();

    glfwSetFramebufferSizeCallback(g_RendererSDFInternalState.window, renderer_internal_sdf_resize);

    rng_generate();
    for (int i = 0; i < MAX_ROCKS_COUNT; i++) {
        gamestate_get_global_instance()->rocks[i][0] = (float) rng_range(1, 10);
        gamestate_get_global_instance()->rocks[i][1] = (float) rng_range(1, 10);
        gamestate_get_global_instance()->rocks[i][2] = (float) rng_range(1, 10);
        gamestate_get_global_instance()->rocks[i][0] -= 5;
        gamestate_get_global_instance()->rocks[i][1] -= 5;
        gamestate_get_global_instance()->rocks[i][2] -= 5;
        gamestate_get_global_instance()->rocks[i][3] = 0.5;
    }
    gamestate_get_global_instance()->rocks[1][0] = 1;
    gamestate_get_global_instance()->rocks[1][1] = 1;
    gamestate_get_global_instance()->rocks[1][2] = -2;
    gamestate_get_global_instance()->rocks[0][0] = 1;
    gamestate_get_global_instance()->rocks[0][1] = 1;
    gamestate_get_global_instance()->rocks[0][2] = -6;

    return true;
}

void renderer_sdf_destroy(void)
{
    // glfw: terminate, clearing all previously allocated GLFW resources.
    // ------------------------------------------------------------------
    glfwTerminate();
}

void renderer_sdf_render(void)
{
    g_RendererSDFInternalState.frameCount++;

    const GameState* game_state = gamestate_get_global_instance();

    if (game_state->keycodes[GLFW_KEY_ESCAPE] == GLFW_PRESS)
        glfwSetWindowShouldClose(g_RendererSDFInternalState.window, true);

    if (game_state->keycodes[GLFW_KEY_SPACE] == GLFW_PRESS)
        renderer_internal_sdf_hot_reload_shaders();

    // clear with a pink color
    renderer_internal_sdf_clear_screen((color_rgba){0.0f, 0.0f, 0.0f, 1.0f});

    const Camera camera = gamestate_get_global_instance()->camera;

    int resolution[2] = {g_RendererSDFInternalState.width, g_RendererSDFInternalState.height};
    setUniformVec2Int(g_RendererSDFInternalState.raymarchShaderID, resolution, "resolution");

    mat4s projection    = glms_perspective(camera.fov, (float) g_RendererSDFInternalState.width / (float) g_RendererSDFInternalState.height, camera.near_plane, camera.far_plane);
    mat4s viewproj      = glms_mul(projection, camera.lookAt);
    uint32_t rocks_visible_count = cull_rocks(MAX_ROCKS_COUNT, gamestate_get_global_instance()->rocks, gamestate_get_global_instance()->rocks_visible, viewproj.raw);
    {
        glUseProgram(g_RendererSDFInternalState.raymarchShaderID);
        int loc = glGetUniformLocation(g_RendererSDFInternalState.raymarchShaderID, "rocks");
        glUniform4fv(loc, rocks_visible_count, &gamestate_get_global_instance()->rocks_visible[0][0]);

        setUniformMat4(g_RendererSDFInternalState.raymarchShaderID, viewproj, "viewproj");

        for (uint32_t i = 0; i < 1; i++) {
            glUseProgram(g_RendererSDFInternalState.raymarchShaderID);
            loc = glGetUniformLocation(g_RendererSDFInternalState.raymarchShaderID, "rocks_idx");
            glUniform1i(loc, i);

            glBindVertexArray(g_RendererSDFInternalState.screen_quad_vao);
            glDrawArrays(GL_TRIANGLES, 0, 6);    // Drawing 6 vertices to form the quad
        }
    }

    // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
    // -------------------------------------------------------------------------------
    glfwSwapBuffers(g_RendererSDFInternalState.window);
    glfwPollEvents();
}
