#ifndef ENGINE_H
#define ENGINE_H

#include <stdbool.h>
#include <stdio.h>

// Include all engine related includes here
#include "core/common.h"

#include "core/frustum.h"
#include "core/game_registry.h"
#include "core/game_state.h"
#include "core/gameobject.h"
#include "core/shader.h"

#include "core/containers/hash_map.h"
#include "core/logging/log.h"
#include "core/memory/memalign.h"
#include "core/rng/rng.h"
#include "core/simd/compiler_defs.h"
#include "core/simd/intrinsics.h"
#include "core/simd/platform_caps.h"
#include "core/uuid/uuid.h"

#include "render/render_structs.h"
#include "render/render_utils.h"
#include "render/renderer_sdf.h"

#include "scene/camera.h"
#include "scene/sdf_scene.h"
#include "scene/transform.h"

#include "scripting/scripting.h"

struct GLFWwindow;

// -- -- -- -- -- -- GAME MAIN -- -- -- -- -- --
// This is called by the client (game) to register game objects and their scripts
extern int game_main(void);
// -- -- -- -- -- -- GAME MAIN -- -- -- -- -- --

typedef struct engine_version
{
    alignas(16) uint8_t major;
    uint8_t minor;
    uint8_t patch;
    char    build[13];    // Optional build metadata (e.g., "alpha", "beta", "rc")
} engine_version;

void engine_init(struct GLFWwindow** gameWindow, uint32_t width, uint32_t height);

void engine_destroy(void);

bool engine_should_quit(void);

void engine_request_quit(void);

void engine_run(void);

float engine_get_delta_time(void);

float engine_get_last_frame(void);

float engine_get_elapsed_time(void);

uint64_t engine_get_fps(void);

uint64_t engine_get_avg_fps(void);

engine_version engine_get_version(void);

const char* engine_get_version_string(void);

uint16_t engine_get_major_version(void);

uint16_t engine_get_minor_version(void);

uint16_t engine_get_patch_version(void);

const char* engine_get_build_metadata(void);

#endif