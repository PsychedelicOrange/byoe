#include "renderer_sdf.h"

#include <GLFW/glfw3.h>

#include "frustum.h"
#include "game_state.h"
#include "logging/log.h"
#include "render_utils.h"
#include "rng/rng.h"
#include "shader.h"

#include "../scene/sdf_scene.h"

#include "frontend/gfx_frontend.h"

// Image transition TODO:
// - [P] swapchain to present src before presentation
// - [P] swapchain to color attachment after presentation
// - [x] sdf_storage_image to general before sdf_scene pass
// - [x] sdf_storage_image to shader read after sdf_scene pass

// Image memory barrier
// - [ ] sdf_scene_texture image memory pipeline barrier before screen quad pass

typedef struct SDFPushConstant
{
    mat4s view_proj;
    ivec2 resolution;
    ivec2 _pad0;
    vec3s dir_light_pos;
    int   curr_draw_node_idx;
} SDFPushConstant;

typedef struct sdf_resources
{
    gfx_resource         scene_texture;
    gfx_resource_view    scene_cs_write_view;
    gfx_resource         scene_nodes_uniform_buffer;
    gfx_resource_view    scene_nodes_ubo_view;
    gfx_shader           shader;
    gfx_pipeline         pipeline;
    gfx_root_signature   root_sig;
    gfx_descriptor_table table;
    SDFPushConstant      pc_data;
} sdf_resources;

typedef struct screen_quad_resoruces
{
    gfx_resource         scene_tex_sampler;
    gfx_resource_view    sampler_view;
    gfx_resource_view    shader_read_view;
    gfx_shader           shader;
    gfx_pipeline         pipeline;
    gfx_root_signature   root_sig;
    gfx_descriptor_table table;
} screen_quad_resoruces;

typedef struct renderer_internal_state
{
    uint32_t              numPrimitives;
    uint32_t              width;
    uint32_t              height;
    uint32_t              raymarchShaderID;
    GLFWwindow*           window;
    const SDF_Scene*      scene;
    uint64_t              frameCount;
    bool                  captureSwapchain;
    bool                  _pad0[3];
    mat4s                 viewproj;
    texture_readback      lastTextureReadback;
    gfx_context           gfxcontext;
    screen_quad_resoruces screen_quad_resources;
    sdf_resources         sdfscene_resources;
} renderer_internal_state;

//---------------------------------------------------------
static renderer_internal_state s_RendererSDFInternalState;
//---------------------------------------------------------
// Private functions
// Resize callback
static void renderer_internal_sdf_resize(GLFWwindow* window, int width, int height)
{
    (void) window;
    s_RendererSDFInternalState.width            = width;
    s_RendererSDFInternalState.height           = height;
    s_RendererSDFInternalState.frameCount       = 0;
    s_RendererSDFInternalState.captureSwapchain = false;

    gfx_flush_gpu_work();

    gfx_ctx_recreate_frame_sync(&s_RendererSDFInternalState.gfxcontext);
    rhi_resize_swapchain(&s_RendererSDFInternalState.gfxcontext.swapchain, width, height);
}

static void renderer_internal_create_shaders(void)
{
    s_RendererSDFInternalState.sdfscene_resources.shader    = gfx_create_compute_shader("./game/shaders_built/raymarch_sdf_scene.comp.spv");
    s_RendererSDFInternalState.screen_quad_resources.shader = gfx_create_vs_ps_shader("./game/shaders_built/screen_quad.vert.spv", "./game/shaders_built/screen_quad.frag.spv");
}

static void renderer_internal_destroy_shaders(void)
{
    gfx_destroy_compute_shader(&s_RendererSDFInternalState.sdfscene_resources.shader);
    gfx_destroy_vs_ps_shader(&s_RendererSDFInternalState.screen_quad_resources.shader);
}

static void renderer_internal_create_root_sigs(void)
{
    gfx_descriptor_binding sdf_scene_ubo_binding = {
        .location = {
            .binding = 0,
            .set     = 0,
        },
        .count       = 1,
        .type        = GFX_RESOURCE_TYPE_UNIFORM_BUFFER,
        .stage_flags = GFX_SHADER_STAGE_CS,
    };

    gfx_descriptor_binding sdf_scene_tex_binding = {
        .location = {
            .binding = 1,
            .set     = 0,
        },
        .count       = 1,
        .type        = GFX_RESOURCE_TYPE_STORAGE_IMAGE,
        .stage_flags = GFX_SHADER_STAGE_CS,
    };

    gfx_descriptor_binding sdf_bindings[] = {sdf_scene_tex_binding, sdf_scene_ubo_binding};

    gfx_descriptor_set_layout set_layout_0 = {
        .bindings      = sdf_bindings,
        .binding_count = ARRAY_SIZE(sdf_bindings),
    };

    gfx_push_constant_range pc_range = {
        .size   = sizeof(SDFPushConstant),
        .offset = 0,
        .stage  = GFX_SHADER_STAGE_CS};

    s_RendererSDFInternalState.sdfscene_resources.root_sig = gfx_create_root_signature(&set_layout_0, 1, &pc_range, 1);

    //------------------------------------------------

    gfx_descriptor_binding screen_tex_binding = {
        .location = {
            .binding = 0,
            .set     = 0,
        },
        .count       = 1,
        .type        = GFX_RESOURCE_TYPE_SAMPLED_IMAGE,    ///////////////////
        .stage_flags = GFX_SHADER_STAGE_PS,
    };

    gfx_descriptor_binding screen_sampler_binding = {
        .location = {
            .binding = 1,
            .set     = 0,
        },
        .count       = 1,
        .type        = GFX_RESOURCE_TYPE_SAMPLER,
        .stage_flags = GFX_SHADER_STAGE_PS,
    };

    gfx_descriptor_binding screen_bindings[] = {screen_tex_binding, screen_sampler_binding};

    gfx_descriptor_set_layout screen_set_layout_0 = {
        .bindings      = screen_bindings,
        .binding_count = 2,
    };

    s_RendererSDFInternalState.screen_quad_resources.root_sig = gfx_create_root_signature(&screen_set_layout_0, 1, NULL, 0);
}

static void renderer_internal_destroy_root_sigs(void)
{
    gfx_destroy_root_signature(&s_RendererSDFInternalState.sdfscene_resources.root_sig);
    gfx_destroy_root_signature(&s_RendererSDFInternalState.screen_quad_resources.root_sig);
}

static void renderer_internal_create_pipelines(void)
{
    gfx_pipeline_create_info scene_pipeline_ci = {
        .type     = GFX_PIPELINE_TYPE_COMPUTE,
        .root_sig = s_RendererSDFInternalState.sdfscene_resources.root_sig,
        .shader   = s_RendererSDFInternalState.sdfscene_resources.shader,
    };

    s_RendererSDFInternalState.sdfscene_resources.pipeline = gfx_create_pipeline(scene_pipeline_ci);

    //------------------------------------------------

    gfx_pipeline_create_info scrn_pipeline_ci = {
        .type                = GFX_PIPELINE_TYPE_GRAPHICS,
        .root_sig            = s_RendererSDFInternalState.screen_quad_resources.root_sig,
        .shader              = s_RendererSDFInternalState.screen_quad_resources.shader,
        .draw_type           = GFX_DRAW_TYPE_TRIANGLE,
        .polygon_mode        = GFX_POLYGON_MODE_FILL,
        .cull_mode           = GFX_CULL_MODE_NO_CULL,
        .enable_depth_test   = false,
        .enable_depth_write  = false,
        .enable_transparency = false,
        .color_formats_count = 1,
        .color_formats[0]    = GFX_FORMAT_SCREEN,
    };

    s_RendererSDFInternalState.screen_quad_resources.pipeline = gfx_create_pipeline(scrn_pipeline_ci);
}

static void renderer_internal_destroy_pipelines(void)
{
    gfx_destroy_pipeline(&s_RendererSDFInternalState.sdfscene_resources.pipeline);

    gfx_destroy_pipeline(&s_RendererSDFInternalState.screen_quad_resources.pipeline);
}

static void renderer_internal_create_scene_pass_descriptor_table(void)
{
    //--------------------------------------------------
    // create the root signature and table to hold the resources/
    // in this pass it's an RW texture2d and it's view
    s_RendererSDFInternalState.sdfscene_resources.scene_texture = gfx_create_texture_resource((gfx_texture_create_info){
        .tex_type = GFX_TEXTURE_TYPE_2D,
        .res_type = GFX_RESOURCE_TYPE_STORAGE_IMAGE,
        .width    = 800,
        .height   = 600,
        .format   = GFX_FORMAT_RGBA32F,
        .depth    = 1});

    s_RendererSDFInternalState.sdfscene_resources.scene_cs_write_view = gfx_create_texture_resource_view((gfx_resource_view_create_info){
        .resource = &s_RendererSDFInternalState.sdfscene_resources.scene_texture,
        .texture  = {
             .layer_count  = 1,
             .base_layer   = 0,
             .mip_levels   = 1,
             .base_mip     = 0,
             .format       = GFX_FORMAT_RGBA32F,
             .texture_type = GFX_TEXTURE_TYPE_2D,
        },
        .res_type = GFX_RESOURCE_TYPE_STORAGE_IMAGE});

    s_RendererSDFInternalState.sdfscene_resources.scene_nodes_uniform_buffer = gfx_create_uniform_buffer_resource(MAX_GPU_NODES_SIZE);

    s_RendererSDFInternalState.sdfscene_resources.scene_nodes_ubo_view = gfx_create_uniform_buffer_resource_view(&s_RendererSDFInternalState.sdfscene_resources.scene_nodes_uniform_buffer, MAX_GPU_NODES_SIZE, 0);

    s_RendererSDFInternalState.sdfscene_resources.table = gfx_create_descriptor_table(&s_RendererSDFInternalState.sdfscene_resources.root_sig);
    gfx_descriptor_table_entry table_entries[]         = {
        (gfx_descriptor_table_entry){&s_RendererSDFInternalState.sdfscene_resources.scene_nodes_uniform_buffer, &s_RendererSDFInternalState.sdfscene_resources.scene_nodes_ubo_view, {0, 0}},
        (gfx_descriptor_table_entry){&s_RendererSDFInternalState.sdfscene_resources.scene_texture, &s_RendererSDFInternalState.sdfscene_resources.scene_cs_write_view, {0, 1}},
    };
    gfx_update_descriptor_table(&s_RendererSDFInternalState.sdfscene_resources.table, table_entries, ARRAY_SIZE(table_entries));
}

static void renderer_internal_create_screen_pass_descriptor_table(void)
{
    s_RendererSDFInternalState.screen_quad_resources.shader_read_view = gfx_create_texture_resource_view((gfx_resource_view_create_info){
        .resource = &s_RendererSDFInternalState.sdfscene_resources.scene_texture,
        .texture  = {
             .layer_count  = 1,
             .base_layer   = 0,
             .mip_levels   = 1,
             .base_mip     = 0,
             .format       = GFX_FORMAT_RGBA32F,
             .texture_type = GFX_TEXTURE_TYPE_2D,
        },
        .res_type = GFX_RESOURCE_TYPE_SAMPLED_IMAGE});

    s_RendererSDFInternalState.screen_quad_resources.scene_tex_sampler = gfx_create_sampler((gfx_sampler_create_info){
        .min_filter     = GFX_FILTER_MODE_LINEAR,
        .mag_filter     = GFX_FILTER_MODE_LINEAR,
        .min_lod        = 0.0f,
        .max_lod        = 1.0f,
        .max_anisotropy = 1.0f,
        .wrap_mode      = GFX_WRAP_MODE_CLAMP_TO_BORDER,
    });

    s_RendererSDFInternalState.screen_quad_resources.sampler_view = gfx_create_sampler_resource_view((gfx_resource_view_create_info){
        .resource = &s_RendererSDFInternalState.screen_quad_resources.scene_tex_sampler,
        .res_type = GFX_RESOURCE_TYPE_SAMPLER});

    s_RendererSDFInternalState.screen_quad_resources.table = gfx_create_descriptor_table(&s_RendererSDFInternalState.screen_quad_resources.root_sig);
    gfx_descriptor_table_entry screen_table_entries[]      = {
        (gfx_descriptor_table_entry){&s_RendererSDFInternalState.sdfscene_resources.scene_texture, &s_RendererSDFInternalState.screen_quad_resources.shader_read_view, {0, 0}},
        (gfx_descriptor_table_entry){&s_RendererSDFInternalState.screen_quad_resources.scene_tex_sampler, &s_RendererSDFInternalState.screen_quad_resources.sampler_view, {0, 1}},
    };
    gfx_update_descriptor_table(&s_RendererSDFInternalState.screen_quad_resources.table, screen_table_entries, ARRAY_SIZE(screen_table_entries));
}

static void renderer_internal_hot_reload_shaders(void)
{
    gfx_flush_gpu_work();

    renderer_internal_destroy_shaders();
    renderer_internal_destroy_pipelines();

    renderer_internal_create_shaders();
    renderer_internal_create_pipelines();

    gfx_flush_gpu_work();
}

static bool render_internal_sdf_init_gfx_ctx(uint32_t width, uint32_t height)
{
    s_RendererSDFInternalState.gfxcontext = gfx_ctx_init(s_RendererSDFInternalState.window, width, height);
    if (uuid_is_null(&s_RendererSDFInternalState.gfxcontext.uuid)) {
        LOG_ERROR("Failed to create vulkan backend!");
        return false;
    } else {
        s_RendererSDFInternalState.gfxcontext.swapchain = gfx_create_swapchain(width, height);
        if (uuid_is_null(&s_RendererSDFInternalState.gfxcontext.swapchain.uuid)) {
            LOG_ERROR("Failed to create gfx_swapchain!");
            return false;
        }

        gfx_ctx_ignite(&s_RendererSDFInternalState.gfxcontext);

        s_RendererSDFInternalState.gfxcontext.cmd_queue.cmds       = calloc(MAX_FRAME_INFLIGHT, sizeof(gfx_cmd_buf*));
        s_RendererSDFInternalState.gfxcontext.cmd_queue.cmds_count = MAX_FRAME_INFLIGHT;

        return true;
    }
}

//----------------------------------------------------------------

static void renderer_internal_create_sdf_pass_resources(void)
{
    // create the shaders and pipeline
    renderer_internal_create_shaders();
    renderer_internal_create_root_sigs();
    renderer_internal_create_pipelines();

    renderer_internal_create_scene_pass_descriptor_table();
    renderer_internal_create_screen_pass_descriptor_table();

    gfx_flush_gpu_work();
}

static void renderer_internal_destroy_sdf_pass_resources(void)
{
    renderer_internal_destroy_shaders();
    renderer_internal_destroy_root_sigs();
    renderer_internal_destroy_pipelines();

    gfx_destroy_texture_resource(&s_RendererSDFInternalState.sdfscene_resources.scene_texture);
    gfx_destroy_texture_resource_view(&s_RendererSDFInternalState.sdfscene_resources.scene_cs_write_view);
    gfx_destroy_descriptor_table(&s_RendererSDFInternalState.sdfscene_resources.table);
    gfx_destroy_uniform_buffer_resource(&s_RendererSDFInternalState.sdfscene_resources.scene_nodes_uniform_buffer);
    gfx_destroy_uniform_buffer_resource_view(&s_RendererSDFInternalState.sdfscene_resources.scene_nodes_ubo_view);

    gfx_destroy_texture_resource_view(&s_RendererSDFInternalState.screen_quad_resources.shader_read_view);
    gfx_destroy_sampler_resource_view(&s_RendererSDFInternalState.screen_quad_resources.sampler_view);
    gfx_destroy_sampler(&s_RendererSDFInternalState.screen_quad_resources.scene_tex_sampler);
    gfx_destroy_descriptor_table(&s_RendererSDFInternalState.screen_quad_resources.table);
}

bool renderer_sdf_init(renderer_desc desc)
{
    // use the desc to init the internal state
    (void) desc;

    s_RendererSDFInternalState.numPrimitives = 0;
    s_RendererSDFInternalState.width         = desc.width;
    s_RendererSDFInternalState.height        = desc.height;
    s_RendererSDFInternalState.window        = desc.window;
    s_RendererSDFInternalState.frameCount    = 0;

    glfwSetWindowSizeCallback(s_RendererSDFInternalState.window, renderer_internal_sdf_resize);

    bool success = render_internal_sdf_init_gfx_ctx(desc.width, desc.height);

    renderer_internal_create_sdf_pass_resources();

    return success;
}

void renderer_sdf_destroy(void)
{
    gfx_flush_gpu_work();

    // clean up
    renderer_internal_destroy_sdf_pass_resources();

    gfx_destroy_gfx_cmd_pool(&s_RendererSDFInternalState.gfxcontext.draw_cmds_pool);
    for (uint32_t i = 0; i < MAX_FRAME_INFLIGHT; i++) {
        //free(s_RendererSDFInternalState.gfxcontext.cmd_queue.cmds[i]);
        gfx_destroy_frame_sync(&s_RendererSDFInternalState.gfxcontext.frame_sync[i]);
    }

    gfx_destroy_swapchain(&s_RendererSDFInternalState.gfxcontext.swapchain);
    gfx_ctx_destroy(&s_RendererSDFInternalState.gfxcontext);

    if (s_RendererSDFInternalState.lastTextureReadback.pixels)
        free(s_RendererSDFInternalState.lastTextureReadback.pixels);

    // glfw: terminate, clearing all previously allocated GLFW resources.
    // ------------------------------------------------------------------
    glfwTerminate();
}

static void renderer_internal_scene_draw_pass(gfx_cmd_buf* cmd_buff)
{
    const SDF_Scene* scene = s_RendererSDFInternalState.scene;

    if (!scene)
        return;

    gfx_render_pass scene_draw_pass = {.is_compute_pass = true};
    rhi_begin_render_pass(cmd_buff, scene_draw_pass, s_RendererSDFInternalState.gfxcontext.swapchain.current_backbuffer_idx);
    {
        rhi_bind_root_signature(cmd_buff, &s_RendererSDFInternalState.sdfscene_resources.root_sig);
        rhi_bind_compute_pipeline(cmd_buff, s_RendererSDFInternalState.sdfscene_resources.pipeline);

        rhi_bind_descriptor_table(cmd_buff, &s_RendererSDFInternalState.sdfscene_resources.table, GFX_PIPELINE_TYPE_COMPUTE);

        const Camera camera     = gamestate_get_global_instance()->camera;
        mat4s        projection = glms_perspective(camera.fov, (float) s_RendererSDFInternalState.width / (float) s_RendererSDFInternalState.height, camera.near_plane, camera.far_plane);

        s_RendererSDFInternalState.sdfscene_resources.pc_data.view_proj     = glms_mul(projection, camera.lookAt);
        s_RendererSDFInternalState.sdfscene_resources.pc_data.resolution[0] = s_RendererSDFInternalState.width;
        s_RendererSDFInternalState.sdfscene_resources.pc_data.resolution[1] = s_RendererSDFInternalState.height;
        s_RendererSDFInternalState.sdfscene_resources.pc_data.dir_light_pos = (vec3s){{1.0f, 1.0f, 1.0f}};

        void* scene_node_update_data = sdf_scene_get_scene_nodes_gpu_data(scene);
        gfx_update_uniform_buffer(&s_RendererSDFInternalState.sdfscene_resources.scene_nodes_uniform_buffer, MAX_GPU_NODES_SIZE, 0, scene_node_update_data);

        for (uint32_t i = 0; i < scene->current_node_head; ++i) {
            if (scene->nodes[i].is_ref_node) continue;

            s_RendererSDFInternalState.sdfscene_resources.pc_data.curr_draw_node_idx = i;
            gfx_push_constant pc                                                     = {.stage = GFX_SHADER_STAGE_CS, .size = sizeof(SDFPushConstant), .offset = 0, .data = &s_RendererSDFInternalState.sdfscene_resources.pc_data};
            rhi_bind_push_constant(cmd_buff, &s_RendererSDFInternalState.sdfscene_resources.root_sig, pc);

            rhi_dispatch(cmd_buff, (s_RendererSDFInternalState.width + DISPATCH_LOCAL_DIM) / DISPATCH_LOCAL_DIM, (s_RendererSDFInternalState.height + DISPATCH_LOCAL_DIM) / DISPATCH_LOCAL_DIM, 1);
        }
    }
    rhi_end_render_pass(cmd_buff, scene_draw_pass);
}

static void renderer_internal_sdf_screen_quad_pass(gfx_cmd_buf* cmd_buff)
{
    rhi_insert_image_layout_barrier(cmd_buff, &s_RendererSDFInternalState.sdfscene_resources.scene_texture, GFX_IMAGE_LAYOUT_GENERAL, GFX_IMAGE_LAYOUT_SHADER_READ_ONLY);

    color_rgba      clear_color       = {{{1.0f, (float) sin(0.0025f * (float) s_RendererSDFInternalState.frameCount), 1.0f, 1.0f}}};
    gfx_render_pass clear_screen_pass = {
        .is_swap_pass            = true,
        .swapchain               = &s_RendererSDFInternalState.gfxcontext.swapchain,
        .extents                 = {{(float) s_RendererSDFInternalState.width, (float) s_RendererSDFInternalState.height}},
        .color_attachments_count = 1,
        .color_attachments[0]    = {
               .clear       = true,
               .clear_color = clear_color}};
    rhi_begin_render_pass(cmd_buff, clear_screen_pass, s_RendererSDFInternalState.gfxcontext.swapchain.current_backbuffer_idx);
    {
        rhi_bind_root_signature(cmd_buff, &s_RendererSDFInternalState.screen_quad_resources.root_sig);
        rhi_bind_gfx_pipeline(cmd_buff, s_RendererSDFInternalState.screen_quad_resources.pipeline);

        rhi_set_viewport(cmd_buff, (gfx_viewport){.x = 0, .y = 0, .width = s_RendererSDFInternalState.width, .height = s_RendererSDFInternalState.height, .min_depth = 0, .max_depth = 1});
        rhi_set_scissor(cmd_buff, (gfx_scissor){.x = 0, .y = 0, .width = s_RendererSDFInternalState.width, .height = s_RendererSDFInternalState.height});

        rhi_bind_descriptor_table(cmd_buff, &s_RendererSDFInternalState.screen_quad_resources.table, GFX_PIPELINE_TYPE_GRAPHICS);

        rhi_draw(cmd_buff, 3, 1, 0, 0);
    }
    rhi_end_render_pass(cmd_buff, clear_screen_pass);

    rhi_insert_image_layout_barrier(cmd_buff, &s_RendererSDFInternalState.sdfscene_resources.scene_texture, GFX_IMAGE_LAYOUT_SHADER_READ_ONLY, GFX_IMAGE_LAYOUT_GENERAL);
}

void renderer_sdf_render(void)
{
    s_RendererSDFInternalState.frameCount++;

    const GameState* game_state = gamestate_get_global_instance();

    if (game_state->keycodes[GLFW_KEY_ESCAPE] == GLFW_PRESS)
        glfwSetWindowShouldClose(s_RendererSDFInternalState.window, true);

    if (game_state->keycodes[GLFW_KEY_SPACE] == GLFW_PRESS)
        renderer_internal_hot_reload_shaders();

    // Scene Culling is done before any rendering begins (might move it to update part of engine loop)

    renderer_sdf_draw_scene(s_RendererSDFInternalState.scene);

    glfwPollEvents();
}

const SDF_Scene* renderer_sdf_get_scene(void)
{
    return s_RendererSDFInternalState.scene;
}

void renderer_sdf_set_scene(const SDF_Scene* scene)
{
    s_RendererSDFInternalState.scene = scene;
}

void renderer_sdf_draw_scene(const SDF_Scene* scene)
{
    if (!scene)
        return;

    gfx_frame_sync* frame_sync = rhi_frame_begin(&s_RendererSDFInternalState.gfxcontext);
    {
        gfx_cmd_buf* cmd_buff = &s_RendererSDFInternalState.gfxcontext.draw_cmds[s_RendererSDFInternalState.gfxcontext.frame_idx];

        rhi_begin_gfx_cmd_recording(cmd_buff);

        {
            // Pass_1: SDF CS rendering
            renderer_internal_scene_draw_pass(cmd_buff);

            // Pass_2: draw the sdf scene texture to screen quad
            renderer_internal_sdf_screen_quad_pass(cmd_buff);
        }

        rhi_end_gfx_cmd_recording(cmd_buff);

        rhi_gfx_cmd_enque_submit(&s_RendererSDFInternalState.gfxcontext.cmd_queue, cmd_buff);
        rhi_gfx_cmd_submit_queue(&s_RendererSDFInternalState.gfxcontext.cmd_queue, frame_sync);
    }
    rhi_frame_end(&s_RendererSDFInternalState.gfxcontext);
}

void renderer_sdf_set_capture_swapchain_ready(void)
{
    s_RendererSDFInternalState.captureSwapchain = true;
}

texture_readback renderer_sdf_read_swapchain(void)
{
    texture_readback result = {0};
    return result;
}

texture_readback renderer_sdf_get_last_swapchain_readback(void)
{
    return s_RendererSDFInternalState.lastTextureReadback;
}