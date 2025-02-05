#include "renderer_sdf.h"

#include "../scene/sdf_scene.h"
#include "frustum.h"
#include "game_state.h"
#include "logging/log.h"
#include "render_utils.h"
#include "rng/rng.h"
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
    uint32_t         numPrimitives;
    uint32_t         width;
    uint32_t         height;
    uint32_t         raymarchShaderID;
    GLFWwindow*      window;
    const SDF_Scene* scene;
    uint64_t         frameCount;
    uint32_t         screen_quad_vao;
    bool             captureSwapchain;
    bool             _pad0[3];
    color_rgba       clearColor;
    mat4s            viewproj;
    texture_readback lastTextureReadback;
} renderer_internal_state;

//---------------------------------------------------------
static renderer_internal_state s_RendererSDFInternalState;
//---------------------------------------------------------
//-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
// Private functions
// Resize callback
static void renderer_internal_sdf_resize(GLFWwindow* window, int width, int height)
{
    (void) window;
    s_RendererSDFInternalState.width            = width;
    s_RendererSDFInternalState.height           = height;
    s_RendererSDFInternalState.frameCount       = 0;
    s_RendererSDFInternalState.captureSwapchain = false;

    glViewport(0, 0, width, height);
}

static void renderer_internal_sdf_hot_reload_shaders(void)
{
    // reload the raymarch shader
    glDeleteProgram(s_RendererSDFInternalState.raymarchShaderID);
    s_RendererSDFInternalState.raymarchShaderID = create_shader("engine/shaders/screen_quad.vert", "engine/shaders/raymarch_sdf_scene.frag");
}

static void renderer_internal_sdf_clear_screen(color_rgba color)
{
    glClearColor(color.r, color.g, color.b, color.a);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

static void renderer_internal_sdf_swap_backbuffer(void)
{
    // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
    // -------------------------------------------------------------------------------
    glfwSwapBuffers(s_RendererSDFInternalState.window);
    glfwPollEvents();
}

static void renderer_internal_sdf_set_pipeline_settings(void)
{
    // Enable Depth testing
    glEnable(GL_DEPTH_TEST);
}

static void renderer_internal_sdf_set_raymarch_shader_global_uniforms(void)
{
    int resolution[2] = {s_RendererSDFInternalState.width, s_RendererSDFInternalState.height};
    setUniformVec2Int(s_RendererSDFInternalState.raymarchShaderID, resolution, "resolution");

    const Camera camera                 = gamestate_get_global_instance()->camera;
    mat4s        projection             = glms_perspective(camera.fov, (float) s_RendererSDFInternalState.width / (float) s_RendererSDFInternalState.height, camera.near_plane, camera.far_plane);
    s_RendererSDFInternalState.viewproj = glms_mul(projection, camera.lookAt);
    setUniformMat4(s_RendererSDFInternalState.raymarchShaderID, s_RendererSDFInternalState.viewproj, "viewproj");
}

//-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --

bool renderer_sdf_init(renderer_desc desc)
{
    // use the desc to init the internal state
    (void) desc;

    s_RendererSDFInternalState.numPrimitives = 0;
    s_RendererSDFInternalState.width         = desc.width;
    s_RendererSDFInternalState.height        = desc.height;
    s_RendererSDFInternalState.window        = desc.window;
    s_RendererSDFInternalState.frameCount    = 0;
    s_RendererSDFInternalState.clearColor    = (color_rgba){0.0f, 0.0f, 0.0f, 1.0f};

    renderer_internal_sdf_hot_reload_shaders();
    s_RendererSDFInternalState.screen_quad_vao = render_utils_setup_screen_quad();

    glfwSetFramebufferSizeCallback(s_RendererSDFInternalState.window, renderer_internal_sdf_resize);

    return true;
}

void renderer_sdf_destroy(void)
{
    if (s_RendererSDFInternalState.lastTextureReadback.pixels)
        free(s_RendererSDFInternalState.lastTextureReadback.pixels);

    // glfw: terminate, clearing all previously allocated GLFW resources.
    // ------------------------------------------------------------------
    glfwTerminate();
}

void renderer_sdf_render(void)
{
    s_RendererSDFInternalState.frameCount++;

    const GameState* game_state = gamestate_get_global_instance();

    if (game_state->keycodes[GLFW_KEY_ESCAPE] == GLFW_PRESS)
        glfwSetWindowShouldClose(s_RendererSDFInternalState.window, true);

    if (game_state->keycodes[GLFW_KEY_SPACE] == GLFW_PRESS)
        renderer_internal_sdf_hot_reload_shaders();

    // Scene Culling is done before any rendering begins (might move it to update part of engine loop)

    // clear with a pink color
    renderer_internal_sdf_clear_screen(s_RendererSDFInternalState.clearColor);

    renderer_internal_sdf_set_pipeline_settings();

    renderer_internal_sdf_set_raymarch_shader_global_uniforms();

    renderer_sdf_draw_scene(s_RendererSDFInternalState.scene);

    if (s_RendererSDFInternalState.captureSwapchain) {
        s_RendererSDFInternalState.lastTextureReadback = renderer_sdf_read_swapchain();
        s_RendererSDFInternalState.captureSwapchain    = false;
    }

    renderer_internal_sdf_swap_backbuffer();
}

const SDF_Scene* renderer_sdf_get_scene(void)
{
    return s_RendererSDFInternalState.scene;
}

void renderer_sdf_set_scene(const SDF_Scene* scene)
{
    s_RendererSDFInternalState.scene = scene;
}

void renderer_sdf_set_clear_color(color_rgba rgba)
{
    s_RendererSDFInternalState.clearColor = rgba;
}

void renderer_sdf_draw_scene(const SDF_Scene* scene)
{
    if (!scene)
        return;

    glUseProgram(s_RendererSDFInternalState.raymarchShaderID);

    sdf_scene_upload_scene_nodes_to_gpu(s_RendererSDFInternalState.scene);

    sdf_scene_bind_scene_nodes(s_RendererSDFInternalState.raymarchShaderID);

    // TEST! TEST! TEST! TEST! TEST! TEST!
    // Test rendering code, move this to a internal function and hide it
    for (uint32_t i = 0; i < scene->current_node_head; ++i) {
        // q: don't draw ref nodes? a: let the shader do that
        if (scene->nodes[i].is_ref_node) continue;

        setUniformInt(s_RendererSDFInternalState.raymarchShaderID, i, "curr_draw_node_idx");
        // Draw the screen quad
        glDrawArrays(GL_TRIANGLES, 0, 6);
    }
    // TEST! TEST! TEST! TEST! TEST! TEST!
}

void renderer_sdf_set_capture_swapchain_ready(void)
{
    s_RendererSDFInternalState.captureSwapchain = true;
}

texture_readback renderer_sdf_read_swapchain(void)
{
    texture_readback result = {0};

    glFinish();

    glfwMakeContextCurrent(s_RendererSDFInternalState.window);

    GLint viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);
    result.width          = viewport[2];
    result.height         = viewport[3];
    result.bits_per_pixel = 3;

    size_t pixel_data_size = result.width * result.height * result.bits_per_pixel;
    result.pixels          = (char*) malloc(pixel_data_size);
    if (!result.pixels) {
        LOG_ERROR("Failed to allocate memory for pixel readback");
        return result;
    }

    glReadBuffer(GL_COLOR_ATTACHMENT0);

    glReadPixels(0, 0, result.width, result.height, GL_RGB, GL_UNSIGNED_BYTE, result.pixels);

    return result;
}

texture_readback renderer_sdf_get_last_swapchain_readback(void)
{
    return s_RendererSDFInternalState.lastTextureReadback;
}
