#include "renderer_sdf.h"

#include "render_utils.h"
#include "game_state.h"
#include "shader.h"

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
    
    glViewport(0, 0, width, height);
}

static void renderer_internal_sdf_hot_reload_shaders(void)
{
    // reload the raymarch shader
    glDeleteProgram(g_RendererSDFInternalState.raymarchShaderID);
    g_RendererSDFInternalState.raymarchShaderID = create_shader("engine/shaders/simple_vert", "engine/shaders/raymarch");
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

    g_RendererSDFInternalState.screen_quad_vao = render_utils_setup_screen_quad();

    glfwSetFramebufferSizeCallback(g_RendererSDFInternalState.window, renderer_internal_sdf_resize);

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
    const GameState* game_state = gamestate_get_global_instance();

    if (game_state->keycodes[GLFW_KEY_ESCAPE] == GLFW_PRESS)
        glfwSetWindowShouldClose(g_RendererSDFInternalState.window, true);

    if (game_state->keycodes[GLFW_KEY_SPACE] == GLFW_PRESS)
        renderer_internal_sdf_hot_reload_shaders();

    glClearColor(1.0f, 0.0f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // int resolution[2] = {INIT_SCR_WIDTH, INIT_SCR_HEIGHT};
        // setUniformVec2Int(raymarchshader, resolution, "resolution");

        // mat4s projection    = glms_perspective(camera->fov, (float) SCR_WIDTH / (float) SCR_HEIGHT, camera->near_plane, camera->far_plane);
        // mat4s viewproj      = glms_mul(projection, gamestate_get_global_instance()->camera.lookAt);
        // rocks_visible_count = cull_rocks(MAX_ROCKS_COUNT, rocks, rocks_visible, viewproj.raw);

        // glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        // glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        // {
        //     glUseProgram(raymarchshader);
        //     int loc = glGetUniformLocation(raymarchshader, "rocks");
        //     glUniform4fv(loc, rocks_visible_count, &rocks_visible[0][0]);
        //     setUniformMat4(raymarchshader, viewproj, "viewproj");

        //     for (int i = 0; i < rocks_visible_count; i++) {
        //         glUseProgram(raymarchshader);
        //         loc = glGetUniformLocation(raymarchshader, "rocks_idx");
        //         glUniform1i(loc, i);

        //         glBindVertexArray(screen_quad_vao);
        //         glDrawArrays(GL_TRIANGLES, 0, 6);    // Drawing 6 vertices to form the quad
        //     }
        // }

    // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
    // -------------------------------------------------------------------------------
    glfwSwapBuffers(g_RendererSDFInternalState.window);
    glfwPollEvents();
}
