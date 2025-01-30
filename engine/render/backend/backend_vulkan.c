#include "backend_vulkan.h"

#include "../core/common.h"
#include "../core/containers/typed_growable_array.h"
#include "../core/logging/log.h"

#include "../core/shader.h"

#define VK_NO_PROTOTYPES
#include <volk.h>
#include <vulkan/vulkan.h>

#ifdef _WIN32
    #include <windows.h>

    #include <vulkan/vulkan_win32.h>
#endif

#include <GLFW/glfw3.h>

#include <stdlib.h>
#include <string.h>    // memset

DEFINE_CLAMP(int)

//--------------------------------------------------------

#define VK_CHECK_RESULT(x, msg) \
    if (x != VK_SUCCESS) { LOG_ERROR(msg); }

#define VK_LAYER_KHRONOS_VALIDATION_NAME "VK_LAYER_KHRONOS_validation"

#define VK_TAG_OBJECT(name, type, handle) vulkan_internal_tag_object(name, type, handle);

//--------------------------------------------------------
// Internal Types
//--------------------------------------------------------

// TODO:
// - [x] Complete Swapchain: sync primitives API + acquire/present/submit API
// - [x] command pool in renderer_sdf
// - [x] command buffers X2
// - [x] basic rendering using begin rendering API for drawing a screen quad without vertices --> KHR_dynamic_rendering renderpass API
//      - [x] shader loading API
//      - [x] pipeline API
//      - [x] draw API
// - [x] Descriptors API + pipeline layout handling etc.
// - [ ] Texture API for binding the r/w texture to composition pass
//      - [x] basic API to create types of textures
//      - [ ] resource view creation API --> VkImageView(s)
//      - [ ] sampler API
//      - [ ] create a 2D RW texure resource and attach it to a CS using descriptors API and check on renderdoc
//      - [ ] fix any issues with the descriptor API
//      - [ ] bind this to the screen_quad pass
// - [ ] Single time command buffers
// - [ ] Barriers (image memory) and Transition layout API
// - [ ] UBOs + Push constants API + setup descriptor sets for the resources x2
//      - [ ] UBO resource API
//      - [ ] hook this up with resource views and descriptor table API
//      - [ ] use above API to bind Push Constant and UBO to upload SDF_NodeGPUData and curr_node_draw_idx
// - [ ] CS dispatch -> SDF raymarching shader
//      - [ ] bring descriptors, resource and views and rhi binding APIs together
//      - [ ] Debug, Test and Fix Issues
// ----------------------> renderer_backend Draft-1
// Draft-2 Goals: resource memory pool RAAI kinda simulation + backend* design consistency using macros + MSAA

typedef struct swapchain_backend
{
    VkSwapchainKHR     swapchain;
    VkSwapchainKHR     old_swapchain;
    VkSurfaceFormatKHR format;
    VkPresentModeKHR   present_mode;
    VkExtent2D         extents;
    uint32_t           image_count;
    VkImage            backbuffers[MAX_BACKBUFFERS];
    VkImageView        backbuffer_views[MAX_BACKBUFFERS];
} swapchain_backend;

typedef struct cmd_pool_backend
{
    VkCommandPool pool;
} cmd_pool_backend;

//---------------------------------
// Utils structs

//-------------------------
// TODO: Combing these 2 structs
typedef struct queue_indices
{
    uint32_t gfx;
    uint32_t present;
    uint32_t async_compute;
} queue_indices;

typedef struct queue_backend
{
    VkQueue gfx;
    VkQueue present;
    VkQueue async_compute;
} queue_backend;
//-------------------------

typedef struct QueueFamPropsArrayView
{
    VkQueueFamilyProperties* arr;
    uint32_t                 size;
} QueueFamPropsArrayView;

typedef struct device_create_info_ex
{
    VkPhysicalDevice   gpu;
    TypedGrowableArray queue_cis;
} device_create_info_ex;

//---------------------------------
typedef struct vulkan_context
{
    GLFWwindow*                      window;
    VkInstance                       instance;
    VkDebugUtilsMessengerEXT         debug_messenger;
    VkSurfaceKHR                     surface;
    VkPhysicalDevice                 gpu;
    VkDevice                         logical_device;
    VkPhysicalDeviceProperties       props;
    VkPhysicalDeviceMemoryProperties mem_props;
    VkExtensionProperties*           supported_extensions;
    queue_indices                    queue_idxs;
    queue_backend                    queues;
} vulkan_context;

static vulkan_context s_VkCtx;

#define VKINSTANCE s_VkCtx.instance
#define VKDEVICE   s_VkCtx.logical_device
#define VKGPU      s_VkCtx.gpu
#define VKSURFACE  s_VkCtx.surface

//--------------------------------------------------------
// Util functions
static VkPrimitiveTopology vulkan_util_draw_type_translate(gfx_draw_type draw_type)
{
    switch (draw_type) {
        case GFX_DRAW_TYPE_POINT: return VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
        case GFX_DRAW_TYPE_LINE: return VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
        case GFX_DRAW_TYPE_TRIANGLE: return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        default: return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    }
}

static VkBlendOp vulkan_util_blend_op_translate(gfx_blend_op blend_op)
{
    switch (blend_op) {
        case GFX_BLEND_OP_ADD: return VK_BLEND_OP_ADD;
        case GFX_BLEND_OP_SUBTRACT: return VK_BLEND_OP_SUBTRACT;
        case GFX_BLEND_OP_REVERSE_SUBTRACT: return VK_BLEND_OP_REVERSE_SUBTRACT;
        case GFX_BLEND_OP_MIN: return VK_BLEND_OP_MIN;
        case GFX_BLEND_OP_MAX: return VK_BLEND_OP_MAX;
        default: return VK_BLEND_OP_ADD;
    }
}

static VkBlendFactor vulkan_util_blend_factor_translate(gfx_blend_factor blend_factor)
{
    switch (blend_factor) {
        case GFX_BLEND_FACTOR_ZERO: return VK_BLEND_FACTOR_ZERO;
        case GFX_BLEND_FACTOR_ONE: return VK_BLEND_FACTOR_ONE;
        case GFX_BLEND_FACTOR_SRC_COLOR: return VK_BLEND_FACTOR_SRC_COLOR;
        case GFX_BLEND_FACTOR_ONE_MINUS_SRC_COLOR: return VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR;
        case GFX_BLEND_FACTOR_DST_COLOR: return VK_BLEND_FACTOR_DST_COLOR;
        case GFX_BLEND_FACTOR_ONE_MINUS_DST_COLOR: return VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR;
        case GFX_BLEND_FACTOR_SRC_ALPHA: return VK_BLEND_FACTOR_SRC_ALPHA;
        case GFX_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA: return VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        case GFX_BLEND_FACTOR_DST_ALPHA: return VK_BLEND_FACTOR_DST_ALPHA;
        case GFX_BLEND_FACTOR_ONE_MINUS_DST_ALPHA: return VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;
        case GFX_BLEND_FACTOR_CONSTANT_COLOR: return VK_BLEND_FACTOR_CONSTANT_COLOR;
        case GFX_BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR: return VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR;
        case GFX_BLEND_FACTOR_CONSTANT_ALPHA: return VK_BLEND_FACTOR_CONSTANT_ALPHA;
        case GFX_BLEND_FACTOR_ONE_MINUS_CONSTANT_ALPHA: return VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_ALPHA;
        case GFX_BLEND_FACTOR_SRC_ALPHA_SATURATE: return VK_BLEND_FACTOR_SRC_ALPHA_SATURATE;
        default: return VK_BLEND_FACTOR_ONE;
    }
}

static VkPolygonMode vulkan_util_polygon_mode_translate(gfx_polygon_mode polygon_mode)
{
    switch (polygon_mode) {
        case GFX_POLYGON_MODE_FILL: return VK_POLYGON_MODE_FILL;
        case GFX_POLYGON_MODE_WIRE: return VK_POLYGON_MODE_LINE;
        case GFX_POLYGON_MODE_POINT: return VK_POLYGON_MODE_POINT;
        default: return VK_POLYGON_MODE_FILL;
    }
}

static VkCullModeFlags vulkan_util_cull_mode_translate(gfx_cull_mode cull_mode)
{
    switch (cull_mode) {
        case GFX_CULL_MODE_BACK: return VK_CULL_MODE_BACK_BIT;
        case GFX_CULL_MODE_FRONT: return VK_CULL_MODE_FRONT_BIT;
        case GFX_CULL_MODE_BACK_AND_FRONT: return VK_CULL_MODE_FRONT_AND_BACK;
        case GFX_CULL_MODE_NO_CULL: return VK_CULL_MODE_NONE;
        default: return VK_CULL_MODE_NONE;
    }
}

static VkCompareOp vulkan_util_compare_op_translate(gfx_compare_op compare_op)
{
    switch (compare_op) {
        case GFX_COMPARE_OP_NEVER: return VK_COMPARE_OP_NEVER;
        case GFX_COMPARE_OP_LESS: return VK_COMPARE_OP_LESS;
        case GFX_COMPARE_OP_EQUAL: return VK_COMPARE_OP_EQUAL;
        case GFX_COMPARE_OP_LESS_OR_EQUAL: return VK_COMPARE_OP_LESS_OR_EQUAL;
        case GFX_COMPARE_OP_GREATER: return VK_COMPARE_OP_GREATER;
        case GFX_COMPARE_OP_NOT_EQUAL: return VK_COMPARE_OP_NOT_EQUAL;
        case GFX_COMPARE_OP_GREATER_OR_EQUAL: return VK_COMPARE_OP_GREATER_OR_EQUAL;
        case GFX_COMPARE_OP_ALWAYS: return VK_COMPARE_OP_ALWAYS;
        default: return VK_COMPARE_OP_ALWAYS;
    }
}

static VkFormat vulkan_util_format_translate(gfx_format format)
{
    switch (format) {
        case GFX_FORMAT_NONE: return VK_FORMAT_UNDEFINED;
        case GFX_FORMAT_R8INT: return VK_FORMAT_R8_SINT;
        case GFX_FORMAT_R8UINT: return VK_FORMAT_R8_UINT;
        case GFX_FORMAT_R8F: return VK_FORMAT_R8_UNORM;
        case GFX_FORMAT_R16F: return VK_FORMAT_R16_SFLOAT;
        case GFX_FORMAT_R32F: return VK_FORMAT_R32_SFLOAT;
        case GFX_FORMAT_R32UINT: return VK_FORMAT_R32_UINT;
        case GFX_FORMAT_R32INT: return VK_FORMAT_R32_SINT;
        case GFX_FORMAT_RGBINT: return VK_FORMAT_R8G8B8_SINT;
        case GFX_FORMAT_RGBUINT: return VK_FORMAT_R8G8B8_UINT;
        case GFX_FORMAT_RGBUNORM: return VK_FORMAT_R8G8B8_UNORM;
        case GFX_FORMAT_RGB32F: return VK_FORMAT_R32G32B32_SFLOAT;
        case GFX_FORMAT_RGBAINT: return VK_FORMAT_R8G8B8A8_SINT;
        case GFX_FORMAT_RGBAUNORM: return VK_FORMAT_R8G8B8A8_UNORM;
        case GFX_FORMAT_RGBA32F: return VK_FORMAT_R32G32B32A32_SFLOAT;
        case GFX_FORMAT_DEPTH32F: return VK_FORMAT_D32_SFLOAT;
        case GFX_FORMAT_DEPTH16UNORM: return VK_FORMAT_D16_UNORM;
        case GFX_FORMAT_DEPTHSTENCIL: return VK_FORMAT_D24_UNORM_S8_UINT;
        case GFX_FORMAT_SCREEN: return VK_FORMAT_B8G8R8A8_UNORM;
        default: return VK_FORMAT_UNDEFINED;
    }
}

static VkDescriptorType vulkan_util_descriptor_type_translate(gfx_resource_type descriptor_type)
{
    switch (descriptor_type) {
        case GFX_RESORUCE_TYPE_SAMPLER: return VK_DESCRIPTOR_TYPE_SAMPLER;
        case GFX_RESOURCE_TYPE_COMBINED_IMAGE_SAMPLER: return VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        case GFX_RESOURCE_TYPE_SAMPLED_IMAGE: return VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        case GFX_RESOURCE_TYPE_STORAGE_IMAGE: return VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        case GFX_RESOURCE_TYPE_UNIFORM_BUFFER: return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        case GFX_RESOURCE_TYPE_STORAGE_BUFFER: return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        case GFX_RESOURCE_TYPE_UNIFORM_TEXEL_BUFFER: return VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;
        case GFX_RESOURCE_TYPE_STORAGE_TEXEL_BUFFER: return VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;
        case GFX_RESOURCE_TYPE_INPUT_ATTACHMENT: return VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
        default: return VK_DESCRIPTOR_TYPE_MAX_ENUM;    // Use the maximum value to indicate an invalid type
    }
}

static VkShaderStageFlagBits vulkan_util_shader_stage_bits(gfx_shader_stage shader_stage)
{
    switch (shader_stage) {
        case GFX_SHADER_STAGE_VS: return VK_SHADER_STAGE_VERTEX_BIT;
        case GFX_SHADER_STAGE_PS: return VK_SHADER_STAGE_FRAGMENT_BIT;
        case GFX_SHADER_STAGE_CS: return VK_SHADER_STAGE_COMPUTE_BIT;
        case GFX_SHADER_STAGE_GS: return VK_SHADER_STAGE_GEOMETRY_BIT;
        case GFX_SHADER_STAGE_TES: return VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
        case GFX_SHADER_STAGE_TCS: return VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
        case GFX_SHADER_STAGE_MS: return VK_SHADER_STAGE_MESH_BIT_NV;
        case GFX_SHADER_STAGE_AS: return VK_SHADER_STAGE_TASK_BIT_NV;
        case GFX_SHADER_STAGE_RAY_HIT: return VK_SHADER_STAGE_RAYGEN_BIT_KHR;
        case GFX_SHADER_STAGE_RAY_ANY_HIT: return VK_SHADER_STAGE_ANY_HIT_BIT_KHR;
        case GFX_SHADER_STAGE_RAY_CLOSEST_HIT: return VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
        default: return VK_SHADER_STAGE_FLAG_BITS_MAX_ENUM;
    }
}

VkImageType vulkan_util_texture_image_type_translate(gfx_texture_type type)
{
    switch (type) {
        case GFX_TEXTURE_TYPE_1D: return VK_IMAGE_TYPE_1D;
        case GFX_TEXTURE_TYPE_2D:
        case GFX_TEXTURE_TYPE_CUBEMAP:
        case GFX_TEXTURE_TYPE_2D_ARRAY: return VK_IMAGE_TYPE_2D;
        case GFX_TEXTURE_TYPE_3D: return VK_IMAGE_TYPE_3D;
        default: return VK_IMAGE_TYPE_MAX_ENUM;
    }
}

VkImageViewType vulkan_util_texture_view_type_translate(gfx_texture_type type)
{
    switch (type) {
        case GFX_TEXTURE_TYPE_1D: return VK_IMAGE_VIEW_TYPE_1D;
        case GFX_TEXTURE_TYPE_2D: return VK_IMAGE_VIEW_TYPE_2D;
        case GFX_TEXTURE_TYPE_3D: return VK_IMAGE_VIEW_TYPE_3D;
        case GFX_TEXTURE_TYPE_CUBEMAP: return VK_IMAGE_VIEW_TYPE_CUBE;
        case GFX_TEXTURE_TYPE_2D_ARRAY: return VK_IMAGE_VIEW_TYPE_2D_ARRAY;
        default: return VK_IMAGE_VIEW_TYPE_MAX_ENUM;
    }
}

//--------------------------------------------------------

static VkResult vulkan_internal_tag_object(const char* name, VkObjectType type, uint64_t handle)
{
    VkDebugUtilsObjectNameInfoEXT info = {
        .sType        = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
        .pObjectName  = name,
        .objectType   = type,
        .objectHandle = (uint64_t) handle};

    PFN_vkSetDebugUtilsObjectNameEXT func = (PFN_vkSetDebugUtilsObjectNameEXT) vkGetInstanceProcAddr(VKINSTANCE, "vkSetDebugUtilsObjectNameEXT");
    if (func != NULL)
        return func(VKDEVICE, &info);
    else

        return VK_ERROR_EXTENSION_NOT_PRESENT;
}

static VkResult vulkan_internal_create_debug_utils_messenger(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger)
{
    PFN_vkCreateDebugUtilsMessengerEXT func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    if (func != NULL)
        return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    else
        return VK_ERROR_EXTENSION_NOT_PRESENT;
}

static void vulkan_internal_destroy_debug_utils_messenger(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator)
{
    PFN_vkDestroyDebugUtilsMessengerEXT func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != NULL)
        func(instance, debugMessenger, pAllocator);
}

// Debug callback
static VkBool32 vulkan_backend_debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageTypes, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData)
{
    (void) messageSeverity;
    (void) messageTypes;
    (void) pCallbackData;
    (void) pUserData;

    if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) LOG_ERROR("%s", pCallbackData->pMessage);
    if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) LOG_WARN("%s", pCallbackData->pMessage);

    return VK_TRUE;
}

static void vulkan_internal_cmd_begin_rendering(VkCommandBuffer commandBuffer, const VkRenderingInfo* pRenderingInfo)
{
    PFN_vkCmdBeginRenderingKHR func = (PFN_vkCmdBeginRenderingKHR) vkGetDeviceProcAddr(VKDEVICE, "vkCmdBeginRenderingKHR");
    if (func != NULL)
        func(commandBuffer, pRenderingInfo);
    else
        LOG_ERROR("VkCmdBeginRenderingKHR Function not found");
}

static void vulkan_internal_cmd_end_rendering(VkCommandBuffer commandBuffer)
{
    PFN_vkCmdEndRenderingKHR func = (PFN_vkCmdEndRenderingKHR) vkGetDeviceProcAddr(VKDEVICE, "vkCmdEndRenderingKHR");
    if (func != NULL)
        func(commandBuffer);
    else
        LOG_ERROR("VkCmdEndRenderingKHR Function not found");
}

//--------------------------------------------------------
// Context
//--------------------------------------------------------

static VkInstance vulkan_internal_create_instance(void)
{
    VkDebugUtilsMessengerCreateInfoEXT debug_ci = {
        .sType           = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
        .pNext           = NULL,
        .pUserData       = NULL,
        .flags           = 0,
        .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                           VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
                           VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                           VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
        .messageType =
            VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_TYPE_DEVICE_ADDRESS_BINDING_BIT_EXT,
        .pfnUserCallback = vulkan_backend_debug_callback};

    VkApplicationInfo app_info = {
        .sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pNext              = NULL,
        .apiVersion         = VK_API_VERSION_1_2,
        .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
        .pApplicationName   = "byoe_ghost_asteroids",
        .engineVersion      = VK_MAKE_VERSION(1, 0, 0),
        .pEngineName        = "byoe"};

    const char* instance_layers[] = {VK_LAYER_KHRONOS_VALIDATION_NAME};

    uint32_t     glfwExtensionsCount = 0;
    const char** glfwExtensions;
    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionsCount);
    LOG_WARN("[Vulkan] GLFW loaded extensions count : %u", glfwExtensionsCount);

    for (uint32_t i = 0; i < glfwExtensionsCount; ++i) {
        LOG_INFO("GLFW required extension: %s", glfwExtensions[i]);
    }

    const char* base_extensions[] = {
#ifdef _WIN32
        VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
#elif __APPLE__
        VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME,
#endif
        VK_KHR_SURFACE_EXTENSION_NAME,
        VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME,
        VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
        "VK_EXT_debug_report",
    };

    uint32_t baseExtensionsCount  = sizeof(base_extensions) / sizeof(base_extensions[0]);
    uint32_t totalExtensionsCount = baseExtensionsCount + glfwExtensionsCount;

    const uint32_t EXTENSION_MAX_CHAR_LEN = 256;
    const char**   instance_extensions    = calloc(totalExtensionsCount, sizeof(char) * EXTENSION_MAX_CHAR_LEN);
    if (!instance_extensions) {
        LOG_ERROR("[MemoryAllocFailure] Failed to allocate memory for instance extensions.");
    }

    for (uint32_t i = 0; i < baseExtensionsCount; ++i) {
        instance_extensions[i] = base_extensions[i];
    }

    for (uint32_t i = 0; i < glfwExtensionsCount; ++i) {
        instance_extensions[baseExtensionsCount + i] = glfwExtensions[i];
    }

    LOG_INFO("Total Vulkan instance extensions requested:");
    for (uint32_t i = 0; i < totalExtensionsCount; ++i) {
        LOG_INFO("\t%s", instance_extensions[i]);
    }

    // TODO: Use a StringView DS for this to work fine
    //    TypedGrowableArray instance_extensions = typed_growable_array_create(sizeof(const char*) * 255, glfwExtensionsCount);

    VkInstanceCreateInfo info_ci = {
        .sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pNext                   = &debug_ci,    // struct injection
        .flags                   = 0,
        .pApplicationInfo        = &app_info,
        .enabledLayerCount       = sizeof(instance_layers) / sizeof(const char*),
        .ppEnabledLayerNames     = instance_layers,
        .enabledExtensionCount   = totalExtensionsCount,
        .ppEnabledExtensionNames = instance_extensions};

#ifdef __APPLE__
    info_ci.flags = VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
#endif

    VkInstance instance;
    VK_CHECK_RESULT(vkCreateInstance(&info_ci, NULL, &instance), "Failed to create vulkan instance");

    free(instance_extensions);

    vulkan_internal_create_debug_utils_messenger(instance, &debug_ci, NULL, &s_VkCtx.debug_messenger);
    return instance;
}

static VkSurfaceKHR vulkan_internal_create_surface(GLFWwindow* window, VkInstance instance)
{
    VkSurfaceKHR surface;
    glfwCreateWindowSurface(instance, (GLFWwindow*) window, NULL, &surface);
    return surface;
}

static bool vulkan_internal_is_best_GPU(VkPhysicalDevice gpu)
{
    VkPhysicalDeviceProperties props = {0};
    vkGetPhysicalDeviceProperties(gpu, &props);
    if (props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
        return true;

    return false;
}

static VkPhysicalDevice vulkan_internal_select_best_gpu(VkInstance instance)
{
    uint32_t total_gpus_count = 0;
    vkEnumeratePhysicalDevices(instance, &total_gpus_count, NULL);
    if (total_gpus_count > 0)
        LOG_INFO("GPUs found: %d", total_gpus_count);
    else
        LOG_ERROR("No GPUs found!");

    VkPhysicalDevice* gpus = calloc(total_gpus_count, sizeof(VkPhysicalDevice));
    vkEnumeratePhysicalDevices(instance, &total_gpus_count, gpus);

    if (!gpus)
        return VK_NULL_HANDLE;

    VkPhysicalDevice candidate_gpu = {0};
    for (uint32_t i = 0; i < total_gpus_count; ++i) {
        if (vulkan_internal_is_best_GPU(gpus[i]) || total_gpus_count == 1) {
            candidate_gpu = gpus[i];
            break;
        }
    }

    free(gpus);

    return candidate_gpu;
}

static void vulkan_internal_query_device_props(VkPhysicalDevice gpu, vulkan_context* ctx)
{
    vkGetPhysicalDeviceMemoryProperties(gpu, &ctx->mem_props);
    vkGetPhysicalDeviceProperties(gpu, &ctx->props);
}

static void vulkan_internal_print_gpu_stats(VkPhysicalDevice gpu)
{
    VkPhysicalDeviceProperties props = {0};
    vkGetPhysicalDeviceProperties(gpu, &props);

    // Print the GPU details
    LOG_INFO("[Vulkan] Vulkan API Version : %d.%d.%d", VK_VERSION_MAJOR(props.apiVersion), VK_VERSION_MINOR(props.apiVersion), VK_VERSION_PATCH(props.apiVersion));
    LOG_INFO("[Vulkan] GPU Name           : %s", props.deviceName);
    LOG_INFO("[Vulkan] Vendor ID          : %d", props.vendorID);
    LOG_INFO("[Vulkan] Device Type        : %d", props.deviceType);
    LOG_INFO("[Vulkan] Driver Version     : %d.%d.%d", VK_VERSION_MAJOR(props.driverVersion), VK_VERSION_MINOR(props.driverVersion), VK_VERSION_PATCH(props.driverVersion));
}

static VkExtensionProperties* vulkan_internal_query_supported_device_extensions(VkPhysicalDevice gpu)
{
    uint32_t extCount = 0;
    vkEnumerateDeviceExtensionProperties(gpu, NULL, &extCount, NULL);
    if (extCount > 0) {
        VkExtensionProperties* supported_extensions = calloc(extCount, sizeof(VkExtensionProperties));
        if (vkEnumerateDeviceExtensionProperties(gpu, NULL, &extCount, supported_extensions) == VK_SUCCESS) {
            LOG_INFO("Selected physical device has %d extensions", extCount);
            for (uint32_t i = 0; i < extCount; ++i) {
                LOG_INFO("  %s", supported_extensions[i].extensionName);
            }
        }
        return supported_extensions;
    }
    return NULL;
}

static QueueFamPropsArrayView vulkan_internal_query_queue_props(VkPhysicalDevice gpu)
{
    uint32_t queueFamilyCount;
    vkGetPhysicalDeviceQueueFamilyProperties(gpu, &queueFamilyCount, NULL);

    VkQueueFamilyProperties* q_fam_props = calloc(queueFamilyCount, sizeof(VkQueueFamilyProperties));
    vkGetPhysicalDeviceQueueFamilyProperties(gpu, &queueFamilyCount, q_fam_props);

    QueueFamPropsArrayView view = {0};
    view.arr                    = q_fam_props;
    view.size                   = queueFamilyCount;

    return view;
}

static queue_indices vulkan_internal_get_queue_family_indices(QueueFamPropsArrayView props, VkPhysicalDevice gpu, VkSurfaceKHR surface)
{
    queue_indices indices = {0};

    for (uint32_t i = 0; i < props.size; ++i) {
        VkQueueFamilyProperties queue_prop = props.arr[i];

        if ((queue_prop.queueFlags & VK_QUEUE_GRAPHICS_BIT) && (queue_prop.queueFlags & VK_QUEUE_COMPUTE_BIT))
            indices.gfx = i;

        VkBool32 presentationSupported = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(gpu, indices.gfx, surface, &presentationSupported);

        if (presentationSupported)
            indices.present = i;

        if (queue_prop.queueFlags & VK_QUEUE_COMPUTE_BIT && !(queue_prop.queueFlags & VK_QUEUE_GRAPHICS_BIT))
            indices.async_compute = i;

        //if (indices.gfx != UINT32_MAX && indices.present != UINT32_MAX && indices.async_compute != UINT32_MAX)
        //    break;
    }
    return indices;
}

static TypedGrowableArray vulkan_internal_create_queue_family_infos(queue_indices indices)
{
    const uint32_t     NUM_QUEUES       = 3;    // gfx, present, async
    TypedGrowableArray queueFamilyInfos = typed_growable_array_create(sizeof(VkDeviceQueueCreateInfo), NUM_QUEUES);

    static float queuePriority = 1.0f;

    VkDeviceQueueCreateInfo* gfxQueueInfo = malloc(sizeof(VkDeviceQueueCreateInfo));
    *gfxQueueInfo                         = (VkDeviceQueueCreateInfo){
                                .sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
                                .queueFamilyIndex = indices.gfx,
                                .queueCount       = 1,
                                .pQueuePriorities = &queuePriority};
    typed_growable_array_append(&queueFamilyInfos, gfxQueueInfo);

    if (indices.gfx != indices.present) {
        VkDeviceQueueCreateInfo* presentQueueInfo = malloc(sizeof(VkDeviceQueueCreateInfo));
        *presentQueueInfo                         = (VkDeviceQueueCreateInfo){
                                    .sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
                                    .queueFamilyIndex = indices.present,
                                    .queueCount       = 1,
                                    .pQueuePriorities = &queuePriority};
        typed_growable_array_append(&queueFamilyInfos, presentQueueInfo);
    }

    if (indices.async_compute != indices.gfx) {
        VkDeviceQueueCreateInfo* computeQueueInfo = malloc(sizeof(VkDeviceQueueCreateInfo));
        *computeQueueInfo                         = (VkDeviceQueueCreateInfo){
                                    .sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
                                    .queueFamilyIndex = indices.async_compute,
                                    .queueCount       = 1,
                                    .pQueuePriorities = &queuePriority};
        typed_growable_array_append(&queueFamilyInfos, computeQueueInfo);
    }

    LOG_ERROR("queue create info count: %d", queueFamilyInfos.size);

    for (uint32_t i = 0; i < queueFamilyInfos.size; i++) {
        VkDeviceQueueCreateInfo* info = (VkDeviceQueueCreateInfo*) typed_growable_array_get_element(&queueFamilyInfos, i);
        LOG_INFO("%d", info->queueFamilyIndex);
    }

    return queueFamilyInfos;
}

static VkDevice vulkan_internal_create_logical_device(device_create_info_ex info)
{
    (void) info;

    VkPhysicalDeviceDynamicRenderingFeaturesKHR dynamicRenderingFeatures = {
        .sType            = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES_KHR,
        .dynamicRendering = VK_TRUE};

    VkPhysicalDeviceFeatures2 device_features = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
        .pNext = &dynamicRenderingFeatures};
    vkGetPhysicalDeviceFeatures2(info.gpu, &device_features);

#ifndef __APPLE__
    device_features.features.samplerAnisotropy       = VK_TRUE;
    device_features.features.pipelineStatisticsQuery = VK_TRUE;
#endif
    device_features.features.sampleRateShading = VK_TRUE;

    const char* device_extensions[] = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME,
        VK_KHR_MAINTENANCE1_EXTENSION_NAME,
        VK_KHR_MAINTENANCE2_EXTENSION_NAME,
        VK_KHR_DEPTH_STENCIL_RESOLVE_EXTENSION_NAME,
        VK_KHR_CREATE_RENDERPASS_2_EXTENSION_NAME,
        VK_EXT_SHADER_VIEWPORT_INDEX_LAYER_EXTENSION_NAME,
        VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME,
        VK_EXT_MEMORY_BUDGET_EXTENSION_NAME,
#ifdef __APPLE__
        VK_EXT_ROBUSTNESS_2_EXTENSION_NAME,
        VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME,
        "VK_KHR_portability_subset",
#endif
    };

    VkDeviceCreateInfo device_ci = {
        .sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pNext                   = &device_features,
        .queueCreateInfoCount    = info.queue_cis.size,
        .pQueueCreateInfos       = info.queue_cis.data,
        .enabledExtensionCount   = sizeof(device_extensions) / sizeof(const char*),
        .ppEnabledExtensionNames = device_extensions,
        .enabledLayerCount       = 0};

    VkDevice device = VK_NULL_HANDLE;

    VK_CHECK_RESULT(vkCreateDevice(info.gpu, &device_ci, NULL, &device), "Failed to create VkDevice");

    return device;
}

static queue_backend vulkan_internal_create_queues(queue_indices indices)
{
    queue_backend backend = {0};

    vkGetDeviceQueue(VKDEVICE, indices.gfx, 0, &backend.gfx);
    vkGetDeviceQueue(VKDEVICE, indices.present, 0, &backend.present);
    vkGetDeviceQueue(VKDEVICE, indices.async_compute, 0, &backend.async_compute);

    return backend;
}

//--------------------------------------------------------

gfx_context vulkan_ctx_init(GLFWwindow* window, uint32_t width, uint32_t height)
{
    (void) width;
    (void) height;

    gfx_context ctx = {0};

    memset(&s_VkCtx, 0, sizeof(vulkan_context));
    s_VkCtx.window = window;

    if (volkInitialize() != VK_SUCCESS) {
        LOG_ERROR("Failed to initialize Volk");
        return ctx;
    }

    VKINSTANCE = vulkan_internal_create_instance();

    volkLoadInstance(VKINSTANCE);

    s_VkCtx.surface = vulkan_internal_create_surface(window, VKINSTANCE);

    // query physical GPUs and create a logical device
    VKGPU = vulkan_internal_select_best_gpu(VKINSTANCE);
    if (VKGPU == VK_NULL_HANDLE) {
        LOG_ERROR("No suitable GPU found!");
        return ctx;
    }

    vulkan_internal_query_device_props(VKGPU, &s_VkCtx);
    vulkan_internal_print_gpu_stats(VKGPU);

    s_VkCtx.supported_extensions = vulkan_internal_query_supported_device_extensions(VKGPU);

    QueueFamPropsArrayView queue_fam_props = vulkan_internal_query_queue_props(VKGPU);
    s_VkCtx.queue_idxs                     = vulkan_internal_get_queue_family_indices(queue_fam_props, VKGPU, s_VkCtx.surface);

    device_create_info_ex device_ci_ex = {0};
    device_ci_ex.gpu                   = VKGPU;
    device_ci_ex.queue_cis             = vulkan_internal_create_queue_family_infos(s_VkCtx.queue_idxs);
    VKDEVICE                           = vulkan_internal_create_logical_device(device_ci_ex);

    if (VKDEVICE == VK_NULL_HANDLE)
        return ctx;

    s_VkCtx.queues = vulkan_internal_create_queues(s_VkCtx.queue_idxs);

    uuid_generate(&ctx.uuid);
    ctx.backend = &s_VkCtx;

    return ctx;
}

void vulkan_ctx_destroy(gfx_context* ctx)
{
    (void) ctx;
    uuid_destroy(&ctx->uuid);

    free(s_VkCtx.supported_extensions);

    vkDestroySurfaceKHR(VKINSTANCE, s_VkCtx.surface, NULL);
    vkDestroyDevice(s_VkCtx.logical_device, NULL);
    vulkan_internal_destroy_debug_utils_messenger(VKINSTANCE, s_VkCtx.debug_messenger, NULL);
    vkDestroyInstance(VKINSTANCE, NULL);
}

void vulkan_flush_gpu_work(void)
{
    vkDeviceWaitIdle(VKDEVICE);
}

//--------------------------------------------------------

static VkSurfaceCapabilitiesKHR vulkan_internal_query_swap_surface_caps(void)
{
    VkSurfaceCapabilitiesKHR surface_caps = {0};
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(VKGPU, VKSURFACE, &surface_caps);
    return surface_caps;
}

static VkSurfaceFormatKHR vulkan_internal_choose_surface_format(void)
{
    uint32_t formatsCount = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(VKGPU, VKSURFACE, &formatsCount, NULL);

    VkSurfaceFormatKHR* formats = calloc(formatsCount, sizeof(VkSurfaceFormatKHR));
    vkGetPhysicalDeviceSurfaceFormatsKHR(VKGPU, VKSURFACE, &formatsCount, formats);

    for (uint32_t i = 0; i < formatsCount; i++) {
        VkSurfaceFormatKHR format = formats[i];
        if (format.format == VK_FORMAT_B8G8R8A8_UNORM && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
            return format;
    }
    return formats[0];
}

static VkPresentModeKHR vulkan_internal_choose_present_mode(void)
{
    uint32_t presentModesCount = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(VKGPU, VKSURFACE, &presentModesCount, NULL);

    VkPresentModeKHR* present_modes = calloc(presentModesCount, sizeof(VkPresentModeKHR));
    vkGetPhysicalDeviceSurfacePresentModesKHR(VKGPU, VKSURFACE, &presentModesCount, present_modes);

    for (uint32_t i = 0; i < presentModesCount; i++) {
        VkPresentModeKHR mode = present_modes[i];
        if (mode == VK_PRESENT_MODE_MAILBOX_KHR)
            return mode;
    }
    return VK_PRESENT_MODE_FIFO_KHR;    // V-Sync
}

static VkExtent2D vulkan_internal_choose_extents(void)
{
    VkSurfaceCapabilitiesKHR caps = vulkan_internal_query_swap_surface_caps();
    if (caps.currentExtent.width != UINT32_MAX)
        return caps.currentExtent;
    else {
        int width, height;
        glfwGetFramebufferSize((GLFWwindow*) s_VkCtx.window, &width, &height);

        VkExtent2D actualExtent = {
            (uint32_t) (width),
            (uint32_t) (height)};

        actualExtent.width  = clamp_int(actualExtent.width, caps.minImageExtent.width, caps.maxImageExtent.width);
        actualExtent.height = clamp_int(actualExtent.height, caps.minImageExtent.height, caps.maxImageExtent.height);

        return actualExtent;
    }
}

static void vulkan_internal_retrieve_swap_images(swapchain_backend* backend)
{
    uint32_t swapImageCount = 0;
    vkGetSwapchainImagesKHR(VKDEVICE, backend->swapchain, &swapImageCount, NULL);

    VK_CHECK_RESULT(vkGetSwapchainImagesKHR(VKDEVICE, backend->swapchain, &swapImageCount, backend->backbuffers), "[Vulkan] Cannot retrieve swapchain images!");
}

static void vulkan_internal_retrieve_swap_image_views(swapchain_backend* backend)
{
    for (uint32_t i = 0; i < backend->image_count; i++) {
        VkImageViewCreateInfo back_buffer_view_ci = {
            .sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image                           = backend->backbuffers[i],
            .viewType                        = VK_IMAGE_VIEW_TYPE_2D,
            .format                          = backend->format.format,
            .components.r                    = VK_COMPONENT_SWIZZLE_IDENTITY,
            .components.g                    = VK_COMPONENT_SWIZZLE_IDENTITY,
            .components.b                    = VK_COMPONENT_SWIZZLE_IDENTITY,
            .components.a                    = VK_COMPONENT_SWIZZLE_IDENTITY,
            .subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
            .subresourceRange.baseMipLevel   = 0,
            .subresourceRange.levelCount     = 1,
            .subresourceRange.baseArrayLayer = 0,
            .subresourceRange.layerCount     = 1};
        VK_CHECK_RESULT(vkCreateImageView(VKDEVICE, &back_buffer_view_ci, NULL, &backend->backbuffer_views[i]), "[Vulkan] Cannot create swap image view!");
    }
}

static void vulkan_internal_destroy_backbuffers(swapchain_backend* backend)
{
    for (uint32_t i = 0; i < backend->image_count; i++) {
        vkDestroyImageView(VKDEVICE, backend->backbuffer_views[i], NULL);
        backend->backbuffer_views[i] = VK_NULL_HANDLE;
    }
}

static void vulkan_internal_transition_image_layout_with_command_buffer(VkDevice device, VkQueue queue, VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout)
{
    VkCommandPool           commandPool;
    VkCommandPoolCreateInfo poolInfo = {
        .sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags            = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT,
        .queueFamilyIndex = s_VkCtx.queue_idxs.gfx,
    };

    if (vkCreateCommandPool(device, &poolInfo, NULL, &commandPool) != VK_SUCCESS) {
        fprintf(stderr, "Failed to create temporary command pool\n");
        return;
    }

    VkCommandBuffer             commandBuffer;
    VkCommandBufferAllocateInfo allocInfo = {
        .sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool        = commandPool,
        .level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1,
    };

    if (vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer) != VK_SUCCESS) {
        fprintf(stderr, "Failed to allocate temporary command buffer\n");
        vkDestroyCommandPool(device, commandPool, NULL);
        return;
    }

    VkCommandBufferBeginInfo beginInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
    };

    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    VkImageMemoryBarrier barrier = {
        .sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .oldLayout           = oldLayout,
        .newLayout           = newLayout,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image               = image,
        .subresourceRange    = {
               .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
               .baseMipLevel   = 0,
               .levelCount     = 1,
               .baseArrayLayer = 0,
               .layerCount     = 1,
        },
        .srcAccessMask = 0,
        .dstAccessMask = 0,
    };

    VkPipelineStageFlags sourceStage;
    VkPipelineStageFlags destinationStage;

    if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        sourceStage           = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage      = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    } else if (oldLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR) {
        barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        barrier.dstAccessMask = 0;
        sourceStage           = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        destinationStage      = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    } else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
        sourceStage           = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage      = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    } else {
        LOG_ERROR("Unsupported layout transition");
        vkDestroyCommandPool(device, commandPool, NULL);
        return;
    }

    vkCmdPipelineBarrier(
        commandBuffer,
        sourceStage,
        destinationStage,
        0,
        0,
        NULL,
        0,
        NULL,
        1,
        &barrier);

    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo = {
        .sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .commandBufferCount = 1,
        .pCommandBuffers    = &commandBuffer,
    };

    vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(queue);

    vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
    vkDestroyCommandPool(device, commandPool, NULL);
}

gfx_swapchain vulkan_device_create_swapchain(uint32_t width, uint32_t height)
{
    gfx_swapchain swapchain = {0};
    uuid_generate(&swapchain.uuid);

    swapchain.width  = width;
    swapchain.height = height;

    swapchain_backend* backend = malloc(sizeof(swapchain_backend));
    if (!backend) {
        LOG_ERROR("error malloc swapchain_backend");
        uuid_destroy(&swapchain.uuid);
        return swapchain;
    }
    memset(backend, 0, sizeof(swapchain_backend));
    swapchain.backend = backend;

    VkSurfaceCapabilitiesKHR caps = vulkan_internal_query_swap_surface_caps();

    backend->format       = vulkan_internal_choose_surface_format();
    backend->present_mode = vulkan_internal_choose_present_mode();
    backend->extents      = vulkan_internal_choose_extents();

    backend->image_count = caps.minImageCount + 1;

    VkSwapchainCreateInfoKHR sc_ci = {
        .sType                 = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface               = VKSURFACE,
        .minImageCount         = backend->image_count,
        .imageFormat           = backend->format.format,
        .imageColorSpace       = backend->format.colorSpace,
        .imageExtent           = backend->extents,
        .imageArrayLayers      = 1,
        .imageUsage            = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        .imageSharingMode      = VK_SHARING_MODE_EXCLUSIVE,
        .compositeAlpha        = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .queueFamilyIndexCount = 0,
        .pQueueFamilyIndices   = NULL,
        .clipped               = VK_TRUE,
        .preTransform          = caps.currentTransform,
        .oldSwapchain          = backend->old_swapchain};

    VK_CHECK_RESULT(vkCreateSwapchainKHR(VKDEVICE, &sc_ci, NULL, &backend->swapchain), "[Vulkan] Failed to create swapchain handle");

    if (backend->old_swapchain != VK_NULL_HANDLE) {
        vkDestroySwapchainKHR(VKDEVICE, backend->old_swapchain, VK_NULL_HANDLE);
        backend->old_swapchain = VK_NULL_HANDLE;
    }

    vulkan_internal_retrieve_swap_images(backend);
    vulkan_internal_retrieve_swap_image_views(backend);

    for (uint32_t i = 0; i < MAX_BACKBUFFERS; i++) {
        vulkan_internal_transition_image_layout_with_command_buffer(VKDEVICE, s_VkCtx.queues.gfx, backend->backbuffers[i], VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
    }

    return swapchain;
}

void vulkan_device_destroy_swapchain(gfx_swapchain* sc)
{
    if (!uuid_is_null(&sc->uuid)) {
        uuid_destroy(&sc->uuid);
        swapchain_backend* backend = sc->backend;

        vulkan_internal_destroy_backbuffers(backend);

        if (backend->swapchain != VK_NULL_HANDLE)
            vkDestroySwapchainKHR(VKDEVICE, backend->swapchain, NULL);
        free(sc->backend);
    }
}

gfx_cmd_pool vulkan_device_create_gfx_cmd_pool(void)
{
    gfx_cmd_pool pool = {0};
    uuid_generate(&pool.uuid);

    cmd_pool_backend* backend = malloc(sizeof(cmd_pool_backend));
    pool.backend              = backend;
    if (!backend) {
        LOG_ERROR("error malloc cmd_pool_backend");
        uuid_destroy(&pool.uuid);
        return pool;
    }

    VkCommandPoolCreateInfo cmdPoolCI = {0};
    cmdPoolCI.sType                   = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    cmdPoolCI.queueFamilyIndex        = s_VkCtx.queue_idxs.gfx;    // same as present
    cmdPoolCI.flags                   = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

    VK_CHECK_RESULT(vkCreateCommandPool(VKDEVICE, &cmdPoolCI, NULL, &backend->pool), "Cannot create gfx command pool");
    return pool;
}

void vulkan_device_destroy_gfx_cmd_pool(gfx_cmd_pool* pool)
{
    if (!uuid_is_null(&pool->uuid)) {
        uuid_destroy(&pool->uuid);
        vkDestroyCommandPool(VKDEVICE, ((cmd_pool_backend*) pool->backend)->pool, NULL);
        free(pool->backend);
    }
}

gfx_cmd_buf vulkan_device_create_gfx_cmd_buf(gfx_cmd_pool* pool)
{
    VkCommandBufferAllocateInfo alloc = {
        .sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .pNext              = NULL,
        .commandPool        = ((cmd_pool_backend*) (pool->backend))->pool,
        .commandBufferCount = 1,
        .level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY};

    gfx_cmd_buf cmd_buf = {0};
    uuid_generate(&cmd_buf.uuid);
    cmd_buf.backend = malloc(sizeof(VkCommandBuffer));

    VK_CHECK_RESULT(vkAllocateCommandBuffers(VKDEVICE, &alloc, cmd_buf.backend), "[Vulkan] cannot allocate frame buffers");
    return cmd_buf;
}

static void vulkan_internal_create_sema(void* backend)
{
    VkSemaphoreCreateInfo semaphoreInfo = {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
        .pNext = NULL,
        .flags = VK_SEMAPHORE_TYPE_BINARY};

    VK_CHECK_RESULT(vkCreateSemaphore(VKDEVICE, &semaphoreInfo, NULL, backend), "[Vulkan] cannot create semaphore");
}

static void vulkan_internal_destroy_sema(VkSemaphore sema)
{
    vkDestroySemaphore(VKDEVICE, sema, NULL);
}

static void vulkan_internal_create_fence(void* backend)
{
    VkFenceCreateInfo fenceCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .flags = VK_FENCE_CREATE_SIGNALED_BIT};

    VK_CHECK_RESULT(vkCreateFence(VKDEVICE, &fenceCreateInfo, NULL, backend), "[Vulkan] cannot create fence");
}

static void vulkan_internal_destroy_fence(VkFence fence)
{
    vkDestroyFence(VKDEVICE, fence, NULL);
}

static int frame_sync_alloc_counter = 0;

gfx_frame_sync vulkan_device_create_frame_sync(void)
{
    frame_sync_alloc_counter++;

    // 2 semas and 1 fence
    gfx_frame_sync frame_sync = {0};

    {
        uuid_generate(&frame_sync.image_ready.uuid);
        frame_sync.image_ready.visibility = GFX_FENCE_TYPE_GPU;
        frame_sync.image_ready.value      = UINT32_MAX;
        frame_sync.image_ready.backend    = malloc(sizeof(VkSemaphore));
        vulkan_internal_create_sema(frame_sync.image_ready.backend);
        VK_TAG_OBJECT("image_ready_sema", VK_OBJECT_TYPE_SEMAPHORE, *(uint64_t*) frame_sync.image_ready.backend);
    }

    {
        uuid_generate(&frame_sync.rendering_done.uuid);
        frame_sync.rendering_done.visibility = GFX_FENCE_TYPE_GPU;
        frame_sync.rendering_done.value      = UINT32_MAX;
        frame_sync.rendering_done.backend    = malloc(sizeof(VkSemaphore));
        vulkan_internal_create_sema(frame_sync.rendering_done.backend);
        VK_TAG_OBJECT("rendering_done_sema", VK_OBJECT_TYPE_SEMAPHORE, *(uint64_t*) frame_sync.image_ready.backend);
    }

    if (!g_gfxConfig.use_timeline_semaphores) {
        uuid_generate(&frame_sync.in_flight.uuid);
        frame_sync.in_flight.visibility = GFX_FENCE_TYPE_CPU;
        frame_sync.in_flight.value      = UINT32_MAX;
        frame_sync.in_flight.backend    = malloc(sizeof(VkFence));
        vulkan_internal_create_fence(frame_sync.in_flight.backend);
        VK_TAG_OBJECT("in_flight_fence", VK_OBJECT_TYPE_FENCE, *(uint64_t*) frame_sync.in_flight.backend);
    }
    return frame_sync;
}

void vulkan_device_destroy_frame_sync(gfx_frame_sync* frame_sync)
{
    (void) frame_sync;
    uuid_destroy(&frame_sync->rendering_done.uuid);
    uuid_destroy(&frame_sync->image_ready.uuid);
    uuid_destroy(&frame_sync->in_flight.uuid);

    vulkan_internal_destroy_sema(*(VkSemaphore*) frame_sync->rendering_done.backend);
    vulkan_internal_destroy_sema(*(VkSemaphore*) frame_sync->image_ready.backend);

    if (!g_gfxConfig.use_timeline_semaphores && frame_sync->in_flight.visibility == GFX_FENCE_TYPE_CPU)
        vulkan_internal_destroy_fence(*(VkFence*) frame_sync->in_flight.backend);

    free(frame_sync->rendering_done.backend);
    free(frame_sync->image_ready.backend);
    free(frame_sync->in_flight.backend);
}

typedef struct shader_backend
{
    VkPipelineShaderStageCreateInfo stage_ci;
    gfx_shader_stage                stage;
    union
    {
        VkShaderModule CS;
        struct
        {
            VkShaderModule VS;
            VkShaderModule PS;
        };
    } modules;
} shader_backend;

typedef struct
{
    uint32_t* data;
    size_t    size;
} SPVBuffer;

static SPVBuffer loadSPVFile(const char* filename)
{
    SPVBuffer buffer = {NULL, 0};
    FILE*     file   = fopen(filename, "rb");
    if (!file) {
        fprintf(stderr, "Failed to open file: %s\n", filename);
        return buffer;
    }

    // Get the file size
    fseek(file, 0, SEEK_END);
    unsigned long fileSize = ftell(file);
    fseek(file, 0, SEEK_SET);

    if (fileSize <= 0) {
        fprintf(stderr, "Invalid SPV file size.\n");
        fclose(file);
        return buffer;
    }

    // Allocate memory for the buffer
    buffer.size = fileSize;
    buffer.data = (uint32_t*) malloc(fileSize);
    if (!buffer.data) {
        fprintf(stderr, "Memory allocation failed.\n");
        fclose(file);
        return buffer;
    }

    // Read the file contents into the buffer
    if (fread(buffer.data, 1, fileSize, file) != fileSize) {
        fprintf(stderr, "Failed to read the file.\n");
        free(buffer.data);
        buffer.data = NULL;
        buffer.size = 0;
        fclose(file);
        return buffer;
    }

    fclose(file);
    return buffer;
}

static VkShaderModule vulkan_internal_create_shader_handle(const char* spv_file_path)
{
    SPVBuffer                shader_byte_code = loadSPVFile(spv_file_path);
    VkShaderModuleCreateInfo shader_ci        = {
               .sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
               .pNext    = NULL,
               .codeSize = shader_byte_code.size,
               .pCode    = shader_byte_code.data};

    VkShaderModule module = VK_NULL_HANDLE;
    VK_CHECK_RESULT(vkCreateShaderModule(VKDEVICE, &shader_ci, NULL, &module), "[Vulkan] cannot create shader module");

    free(shader_byte_code.data);

    return module;
}

gfx_shader vulkan_device_create_compute_shader(const char* spv_file_path)
{
    gfx_shader shader = {0};
    uuid_generate(&shader.uuid);

    shader_backend* backend = malloc(sizeof(shader_backend));
    if (backend) {
        shader.stages.CS = backend;
        backend->stage   = GFX_SHADER_STAGE_CS;

        backend->modules.CS = vulkan_internal_create_shader_handle(spv_file_path);

        backend->stage_ci = (VkPipelineShaderStageCreateInfo){
            .sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage  = VK_SHADER_STAGE_COMPUTE_BIT,
            .pName  = "main",
            .module = backend->modules.CS};
    }
    return shader;
}

gfx_shader vulkan_device_create_vs_ps_shader(const char* spv_file_path_vs, const char* spv_file_path_ps)
{
    gfx_shader shader = {0};
    uuid_generate(&shader.uuid);
    (void) spv_file_path_vs;
    shader_backend* backend_vs = malloc(sizeof(shader_backend));
    if (backend_vs) {
        shader.stages.VS  = backend_vs;
        backend_vs->stage = GFX_SHADER_STAGE_VS;

        backend_vs->modules.VS = vulkan_internal_create_shader_handle(spv_file_path_vs);

        backend_vs->stage_ci = (VkPipelineShaderStageCreateInfo){
            .sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage  = VK_SHADER_STAGE_VERTEX_BIT,
            .pName  = "main",
            .module = backend_vs->modules.VS};
    }

    shader_backend* backend_ps = malloc(sizeof(shader_backend));
    if (backend_ps) {
        shader.stages.PS  = backend_ps;
        backend_ps->stage = GFX_SHADER_STAGE_PS;

        backend_ps->modules.PS = vulkan_internal_create_shader_handle(spv_file_path_ps);

        backend_ps->stage_ci = (VkPipelineShaderStageCreateInfo){
            .sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage  = VK_SHADER_STAGE_FRAGMENT_BIT,
            .pName  = "main",
            .module = backend_ps->modules.PS};
    }

    return shader;
}

void vulkan_device_destroy_compute_shader(gfx_shader* shader)
{
    uuid_destroy(&shader->uuid);

    shader_backend* backend = shader->stages.CS;

    vkDestroyShaderModule(VKDEVICE, backend->modules.CS, NULL);

    free(backend);
}

void vulkan_device_destroy_vs_ps_shader(gfx_shader* shader)
{
    uuid_destroy(&shader->uuid);

    shader_backend* backend_vs = shader->stages.VS;
    shader_backend* backend_ps = shader->stages.PS;

    vkDestroyShaderModule(VKDEVICE, backend_vs->modules.VS, NULL);
    vkDestroyShaderModule(VKDEVICE, backend_ps->modules.PS, NULL);

    free(backend_vs);
    free(backend_ps);
}

static gfx_pipeline vulkan_internal_create_compute_pipeline(gfx_pipeline_create_info info)
{
    gfx_pipeline pipeline = {0};
    uuid_generate(&pipeline.uuid);

    (void) info;

    return pipeline;
}

typedef struct pipeline_backend
{
    VkPipeline pipeline;
} pipeline_backend;

#define NUM_DYNAMIC_STATES 2

static gfx_pipeline vulkan_internal_create_gfx_pipeline(gfx_pipeline_create_info info)
{
    gfx_pipeline pipeline = {0};
    uuid_generate(&pipeline.uuid);

    //----------------------------
    // Vertex Input Layout Stage
    //----------------------------
    // NULLED FOR TESTING SCREEN QUAD
    //VkVertexInputBindingDescription vertex_binding_description = {
    //    .binding   = 0,
    //    .inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
    //    .stride    = 0};

    //VkVertexInputAttributeDescription input_attrib = {
    //    .binding  = 0,
    //    .location = 0,
    //    .format   = VK_FORMAT_R32G32B32A32_SFLOAT,
    //    .offset   = 0};

    VkPipelineVertexInputStateCreateInfo vertex_input_sci = {
        .sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .pNext                           = NULL,
        .vertexBindingDescriptionCount   = 0,
        .pVertexBindingDescriptions      = NULL,
        .vertexAttributeDescriptionCount = 0,
        .pVertexAttributeDescriptions    = NULL};

    //----------------------------
    // Input Assembly Stage
    //----------------------------
    VkPipelineInputAssemblyStateCreateInfo input_assembly_sci = {
        .sType                  = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .pNext                  = NULL,
        .primitiveRestartEnable = VK_FALSE,
        .topology               = vulkan_util_draw_type_translate(info.draw_type)};

    //----------------------------
    // Viewport and Dynamic states
    //----------------------------
    VkPipelineViewportStateCreateInfo viewport_sci = {
        .sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .pNext         = NULL,
        .viewportCount = 1,
        .scissorCount  = 1};

    VkDynamicState dynamic_state_descriptors[NUM_DYNAMIC_STATES] = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR};

    VkPipelineDynamicStateCreateInfo dynamic_state_ci = {
        .sType             = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .pNext             = NULL,
        .dynamicStateCount = NUM_DYNAMIC_STATES,
        .pDynamicStates    = dynamic_state_descriptors};

    //----------------------------
    // Rasterizer Stage
    //----------------------------
    VkPipelineRasterizationStateCreateInfo rasterization_sci = {
        .sType                   = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .pNext                   = NULL,
        .cullMode                = vulkan_util_cull_mode_translate(info.cull_mode),
        .depthClampEnable        = VK_FALSE,
        .rasterizerDiscardEnable = VK_FALSE,
        .polygonMode             = vulkan_util_polygon_mode_translate(info.polygon_mode),
        .frontFace               = VK_FRONT_FACE_CLOCKWISE,
        .lineWidth               = 1.0f};

    //----------------------------
    // Color Blend State
    //----------------------------
    VkPipelineColorBlendAttachmentState color_blend_attachment = {
        .blendEnable         = info.enable_transparency,
        .srcColorBlendFactor = vulkan_util_blend_factor_translate(info.src_color_blend_factor),
        .dstColorBlendFactor = vulkan_util_blend_factor_translate(info.dst_color_blend_factor),
        .colorBlendOp        = vulkan_util_blend_op_translate(info.color_blend_op),
        .srcAlphaBlendFactor = vulkan_util_blend_factor_translate(info.src_alpha_blend_factor),
        .dstAlphaBlendFactor = vulkan_util_blend_factor_translate(info.dst_alpha_blend_factor),
        .alphaBlendOp        = vulkan_util_blend_op_translate(info.alpha_blend_op),
        .colorWriteMask      = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                          VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT};

    VkPipelineColorBlendStateCreateInfo color_blend_sci = {
        .sType           = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .pNext           = NULL,
        .attachmentCount = 1,
        .pAttachments    = &color_blend_attachment,
        .logicOpEnable   = VK_FALSE};

    //----------------------------
    // Depth Stencil Stage
    //----------------------------
    VkPipelineDepthStencilStateCreateInfo depthStencilSCI = {
        .sType                 = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
        .pNext                 = NULL,
        .depthTestEnable       = info.enable_depth_test,
        .depthWriteEnable      = info.enable_depth_write,
        .depthCompareOp        = vulkan_util_compare_op_translate(info.depth_compare),
        .depthBoundsTestEnable = VK_FALSE,
        .stencilTestEnable     = VK_FALSE,
        .back.failOp           = VK_STENCIL_OP_KEEP,
        .back.passOp           = VK_STENCIL_OP_KEEP,
        .back.compareOp        = VK_COMPARE_OP_ALWAYS,
        .back.compareMask      = 0,
        .back.reference        = 0,
        .back.depthFailOp      = VK_STENCIL_OP_KEEP,
        .back.writeMask        = 0,
        .minDepthBounds        = 0,
        .maxDepthBounds        = 0,
        .front.failOp          = VK_STENCIL_OP_KEEP,
        .front.passOp          = VK_STENCIL_OP_KEEP,
        .front.compareOp       = VK_COMPARE_OP_ALWAYS,
        .front.compareMask     = 0,
        .front.reference       = 0,
        .front.depthFailOp     = VK_STENCIL_OP_KEEP};

    //----------------------------
    // Multi sample State (MSAA)
    //----------------------------
    VkPipelineMultisampleStateCreateInfo multiSampleSCI = {
        .sType                 = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .pNext                 = NULL,
        .pSampleMask           = NULL,
        .rasterizationSamples  = VK_SAMPLE_COUNT_1_BIT,
        .sampleShadingEnable   = VK_FALSE,
        .alphaToCoverageEnable = VK_FALSE,
        .alphaToOneEnable      = VK_FALSE,
        .minSampleShading      = 0.5};

    //----------------------------
    // Dynamic Rendering KHR
    //----------------------------

    VkPipelineRenderingCreateInfoKHR pipeline_rendering_ci = {
        .sType                   = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR,
        .colorAttachmentCount    = info.color_formats_count,
        .pColorAttachmentFormats = NULL,
        .depthAttachmentFormat   = vulkan_util_format_translate(info.depth_format)};

    VkFormat colorAttachmentFormats[MAX_RT] = {0};
    for (uint32_t i = 0; i < info.color_formats_count; i++) {
        colorAttachmentFormats[i] = vulkan_util_format_translate(info.color_formats[i]);
    }
    pipeline_rendering_ci.pColorAttachmentFormats = colorAttachmentFormats;

    //----------------------------
    // Gfx Pipeline
    //----------------------------

    uint32_t stage_count = 0;
    if (info.shader.stages.VS) stage_count++;
    if (info.shader.stages.PS) stage_count++;
    VkPipelineShaderStageCreateInfo* stages        = (VkPipelineShaderStageCreateInfo*) malloc(stage_count * sizeof(VkPipelineShaderStageCreateInfo));
    uint32_t                         current_stage = 0;
    if (info.shader.stages.VS) {
        shader_backend* vs_backend = (shader_backend*) info.shader.stages.VS;
        stages[current_stage++]    = vs_backend->stage_ci;
    }

    if (info.shader.stages.PS) {
        shader_backend* ps_backend = (shader_backend*) info.shader.stages.PS;
        stages[current_stage++]    = ps_backend->stage_ci;
    }

    pipeline_backend* backend = malloc(sizeof(pipeline_backend));
    memset(backend, 0, sizeof(pipeline_backend));

    VkGraphicsPipelineCreateInfo graphics_pipeline_ci = {
        .sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .pNext               = &pipeline_rendering_ci,
        .layout              = *((VkPipelineLayout*) (info.root_sig.backend)),
        .basePipelineHandle  = VK_NULL_HANDLE,
        .basePipelineIndex   = -1,
        .pVertexInputState   = &vertex_input_sci,
        .pInputAssemblyState = &input_assembly_sci,
        .pRasterizationState = &rasterization_sci,
        .pColorBlendState    = &color_blend_sci,
        .pTessellationState  = NULL,
        .pMultisampleState   = &multiSampleSCI,
        .pDynamicState       = &dynamic_state_ci,
        .pViewportState      = &viewport_sci,
        .pDepthStencilState  = &depthStencilSCI,
        .pStages             = stages,
        .stageCount          = stage_count,
        .renderPass          = VK_NULL_HANDLE};    // renderPass is NULL since we are using VK_KHR_dynamic_rendering extension

    VK_CHECK_RESULT(vkCreateGraphicsPipelines(VKDEVICE, VK_NULL_HANDLE, 1, &graphics_pipeline_ci, NULL, &backend->pipeline), "[Vulkan] could not create grapihcs pipeline");

    free(stages);

    pipeline.backend = backend;

    return pipeline;
}

gfx_pipeline vulkan_device_create_pipeline(gfx_pipeline_create_info info)
{
    if (info.type == GFX_PIPELINE_TYPE_GRAPHICS) return vulkan_internal_create_gfx_pipeline(info);
    else
        return vulkan_internal_create_compute_pipeline(info);
}

void vulkan_device_destroy_pipeline(gfx_pipeline* pipeline)
{
    uuid_destroy(&pipeline->uuid);
    vkDestroyPipeline(VKDEVICE, ((pipeline_backend*) (pipeline->backend))->pipeline, NULL);
    free(pipeline->backend);
    pipeline->backend = NULL;
}

typedef struct root_signature_backend
{
    VkPipelineLayout       pipeline_layout;
    VkDescriptorSetLayout* vk_descriptor_set_layouts;
} root_signature_backend;

gfx_root_signature vulkan_device_create_root_signature(const gfx_descriptor_set_layout* set_layouts, uint32_t set_layout_count, const gfx_push_constant_range* push_constants, uint32_t push_constant_count)
{
    gfx_root_signature root_sig = {0};
    uuid_generate(&root_sig.uuid);
    root_signature_backend* backend = malloc(sizeof(root_signature_backend));
    root_sig.backend                = backend;

    backend->vk_descriptor_set_layouts = malloc(sizeof(VkDescriptorSetLayout) * set_layout_count);
    for (uint32_t i = 0; i < set_layout_count; ++i) {
        const gfx_descriptor_set_layout* set_layout = &set_layouts[i];

        VkDescriptorSetLayoutBinding* vk_bindings = malloc(sizeof(VkDescriptorSetLayoutBinding) * set_layout->binding_count);

        for (uint32_t j = 0; j < set_layout->binding_count; ++j) {
            const gfx_descriptor_binding binding = set_layout->bindings[j];
            vk_bindings[j]                       = (VkDescriptorSetLayoutBinding){
                                      .binding            = binding.binding,
                                      .descriptorType     = vulkan_util_descriptor_type_translate(binding.type),
                                      .descriptorCount    = binding.count,
                                      .stageFlags         = vulkan_util_shader_stage_bits(binding.stage_flags),
                                      .pImmutableSamplers = NULL,
            };
        }

        VkDescriptorSetLayoutCreateInfo set_layout_ci = {
            .sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
            .pNext        = NULL,
            .flags        = 0,
            .bindingCount = set_layout->binding_count,
            .pBindings    = vk_bindings,
        };

        VK_CHECK_RESULT(vkCreateDescriptorSetLayout(VKDEVICE, &set_layout_ci, NULL, &backend->vk_descriptor_set_layouts[i]), "[Vulkan] cannot create descriptor set layout");
        free(vk_bindings);
    }

    VkPushConstantRange* vk_push_constants = NULL;
    if (push_constant_count > 0) {
        vk_push_constants = malloc(sizeof(VkPushConstantRange) * push_constant_count);

        for (uint32_t i = 0; i < push_constant_count; ++i) {
            const gfx_push_constant_range push_constant = push_constants[i];
            vk_push_constants[i]                        = (VkPushConstantRange){
                                       .offset     = push_constant.offset,
                                       .size       = push_constant.size,
                                       .stageFlags = vulkan_util_shader_stage_bits(push_constant.stage),
            };
        }
    }

    VkPipelineLayoutCreateInfo pipeline_layout_ci = {
        .sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .pNext                  = NULL,
        .flags                  = 0,
        .setLayoutCount         = set_layout_count,
        .pSetLayouts            = backend->vk_descriptor_set_layouts,
        .pushConstantRangeCount = push_constant_count,
        .pPushConstantRanges    = vk_push_constants,
    };

    VK_CHECK_RESULT(vkCreatePipelineLayout(VKDEVICE, &pipeline_layout_ci, NULL, &((root_signature_backend*) root_sig.backend)->pipeline_layout), "[Vulkan] cannot create pipeline layout");
    free(vk_push_constants);

    root_sig.descriptor_set_layouts  = (gfx_descriptor_set_layout*) set_layouts;
    root_sig.descriptor_layout_count = set_layout_count;
    root_sig.push_constants          = (gfx_push_constant_range*) push_constants;
    root_sig.push_constant_count     = push_constant_count;

    return root_sig;
}

void vulkan_device_destroy_root_signature(gfx_root_signature* root_sig)
{
    uuid_destroy(&root_sig->uuid);
    for (uint32_t i = 0; i < root_sig->descriptor_layout_count; i++) {
        vkDestroyDescriptorSetLayout(VKDEVICE, ((root_signature_backend*) (root_sig->backend))->vk_descriptor_set_layouts[i], NULL);
    }

    vkDestroyPipelineLayout(VKDEVICE, ((root_signature_backend*) (root_sig->backend))->pipeline_layout, NULL);
    VkDescriptorSetLayout* layouts = ((root_signature_backend*) (root_sig->backend))->vk_descriptor_set_layouts;
    if (layouts)
        free(layouts);
    free(root_sig->backend);
    root_sig->backend = NULL;
}

typedef struct descriptor_table_backend
{
    VkPipelineLayout pipeline_layout_ref_handle;
    VkDescriptorPool pool;
    VkDescriptorSet* sets;
    uint32_t         num_sets;
} descriptor_table_backend;

gfx_descriptor_table vulkan_device_create_descriptor_table(const gfx_root_signature* root_signature)
{
    gfx_descriptor_table descriptor_table = {0};
    uuid_generate(&descriptor_table.uuid);

    descriptor_table_backend* backend = malloc(sizeof(descriptor_table_backend));

    // cache pipeline layout from root_signature vulkan backend
    backend->pipeline_layout_ref_handle = ((root_signature_backend*) (root_signature->backend))->pipeline_layout;
    backend->num_sets                   = root_signature->descriptor_layout_count;

    // TODO: either expose customization options or make it generic enough without affecting perf!
    VkDescriptorPoolSize pool_sizes[] = {
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 128},
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 128},
        {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 64},
    };

    VkDescriptorPoolCreateInfo pool_ci = {
        .sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .pNext         = NULL,
        .flags         = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
        .maxSets       = root_signature->descriptor_layout_count,
        .poolSizeCount = sizeof(pool_sizes) / sizeof(pool_sizes[0]),
        .pPoolSizes    = pool_sizes,
    };

    VK_CHECK_RESULT(vkCreateDescriptorPool(VKDEVICE, &pool_ci, NULL, &backend->pool), "[Vulkan] Failed to allocate descriptor pool");

    VkDescriptorSetAllocateInfo alloc_info = {
        .sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .pNext              = NULL,
        .descriptorPool     = backend->pool,
        .descriptorSetCount = root_signature->descriptor_layout_count,
        .pSetLayouts        = (const VkDescriptorSetLayout*) root_signature->descriptor_set_layouts,
    };

    VK_CHECK_RESULT(vkAllocateDescriptorSets(VKDEVICE, &alloc_info, backend->sets), "[Vulkan] Failed to allocated descriptor sets");

    descriptor_table.backend   = backend;
    descriptor_table.set_count = root_signature->descriptor_layout_count;

    return descriptor_table;
}

void vulkan_device_destroy_descriptor_table(gfx_descriptor_table* descriptor_table)
{
    (void) descriptor_table;
    uuid_destroy(&descriptor_table->uuid);
    vkDestroyDescriptorPool(VKDEVICE, ((descriptor_table_backend*) (descriptor_table->backend))->pool, NULL);
    free(descriptor_table->backend);
    descriptor_table->backend = NULL;
}

void vulkan_device_update_descriptor_table(gfx_descriptor_table* descriptor_table, gfx_resource* resources, uint32_t num_resources)
{
    descriptor_table_backend* backend = (descriptor_table_backend*) (descriptor_table->backend);
    VkDescriptorSet*          sets    = backend->sets;

    VkWriteDescriptorSet* writes = malloc(sizeof(VkWriteDescriptorSet) * num_resources);

    for (uint32_t i = 0; i < num_resources; i++) {
        gfx_resource* res = &resources[i];

        writes[i] = (VkWriteDescriptorSet){
            .sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet          = sets[res->set],
            .dstBinding      = res->binding,
            .dstArrayElement = 0,
            .descriptorCount = 1};

        switch (res->type) {
            case GFX_RESOURCE_TYPE_UNIFORM_BUFFER:
            case GFX_RESOURCE_TYPE_STORAGE_BUFFER: {
                VkDescriptorBufferInfo buffer_info = {
                    .buffer = (VkBuffer) res->ubo.backend,
                    .offset = 0,
                    .range  = VK_WHOLE_SIZE};
                writes[i].descriptorType = (VkDescriptorType) res->type;
                writes[i].pBufferInfo    = &buffer_info;
                break;
            }
            case GFX_RESOURCE_TYPE_SAMPLED_IMAGE:
            case GFX_RESOURCE_TYPE_STORAGE_IMAGE:
            case GFX_RESOURCE_TYPE_COMBINED_IMAGE_SAMPLER: {
                VkDescriptorImageInfo image_info = {
                    .imageView   = (VkImageView) res->texture.backend,
                    .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
                writes[i].descriptorType = (VkDescriptorType) res->type;
                writes[i].pImageInfo     = &image_info;
                break;
            }
            default:
                break;
        }
    }
    vkUpdateDescriptorSets(VKDEVICE, num_resources, writes, 0, NULL);
}

uint32_t vulkan_internal_find_memory_type(VkPhysicalDevice physical_device, uint32_t type_filter, VkMemoryPropertyFlags properties)
{
    VkPhysicalDeviceMemoryProperties memory_properties;
    vkGetPhysicalDeviceMemoryProperties(physical_device, &memory_properties);

    for (uint32_t i = 0; i < memory_properties.memoryTypeCount; i++) {
        if ((type_filter & (1 << i)) && (memory_properties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }
    return UINT32_MAX;
}

typedef struct texture_backend
{
    VkImage        image;
    VkDeviceMemory memory;
} texture_backend;

gfx_resource vulkan_device_create_texure_resource(gfx_texture_create_desc desc)
{
    gfx_resource resource = {0};
    uuid_generate(&resource.texture.uuid);

    gfx_texture*     texture = &resource.texture;
    texture_backend* backend = malloc(sizeof(texture_backend));
    texture->backend         = backend;

    VkImageCreateInfo image_info = {
        .sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .pNext         = NULL,
        .flags         = 0,
        .imageType     = vulkan_util_texture_image_type_translate(desc.type),
        .format        = vulkan_util_format_translate(desc.format),
        .extent        = {desc.width, desc.height, desc.depth},
        .mipLevels     = 1,    // Note: No MIPS!
        .arrayLayers   = (desc.type == GFX_TEXTURE_TYPE_CUBEMAP) ? 6 : 1,
        .samples       = VK_SAMPLE_COUNT_1_BIT,
        .tiling        = VK_IMAGE_TILING_OPTIMAL,
        .usage         = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
        .sharingMode   = VK_SHARING_MODE_EXCLUSIVE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
    };

    VK_CHECK_RESULT(vkCreateImage(VKDEVICE, &image_info, NULL, &backend->image), "[Vulkan] cannot create VkImage");

    VkMemoryRequirements mem_requirements;
    vkGetImageMemoryRequirements(VKDEVICE, backend->image, &mem_requirements);

    VkMemoryAllocateInfo alloc_info = {
        .sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize  = mem_requirements.size,
        .memoryTypeIndex = vulkan_internal_find_memory_type(VKGPU, mem_requirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)};

    VK_CHECK_RESULT(vkAllocateMemory(VKDEVICE, &alloc_info, NULL, &backend->memory), "[Vulkan] cannot allocate memory for image");
    vkBindImageMemory(VKDEVICE, backend->image, backend->memory, 0);

    return resource;
}

void vulkan_device_destroy_texure_resource(gfx_resource* resource)
{
    uuid_destroy(&resource->texture.uuid);

    VkImage vk_image = ((texture_backend*) resource->texture.backend)->image;
    vkDestroyImage(VKDEVICE, vk_image, NULL);
    free(resource->texture.backend);
    resource->texture.backend = NULL;
}

typedef struct tex_resource_view_backend
{
    VkImageView view;
} tex_resource_view_backend;

gfx_resource_view vulkan_device_create_texture_res_view(gfx_resource_view_desc desc)
{
    gfx_resource_view view = {0};
    uuid_generate(&view.uuid);

    tex_resource_view_backend* backend = malloc(sizeof(tex_resource_view_backend));
    view.backend                       = backend;

    VkImageViewCreateInfo view_ci = {
        .sType            = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .pNext            = NULL,
        .flags            = 0,
        .image            = VK_NULL_HANDLE,
        .format           = vulkan_util_format_translate(desc.texture.format),
        .viewType         = vulkan_util_texture_view_type_translate(desc.texture.texture_type),
        .components       = (VkComponentMapping){VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A},
        .subresourceRange = {
            .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,    // FIXME: hardcoded shit since depth tex will neeed a VK_IMAGE_ASPECT_DEPTH_BIT
            .baseArrayLayer = desc.texture.base_layer,
            .baseMipLevel   = desc.texture.base_mip,
            .layerCount     = desc.texture.layer_count,
            .levelCount     = VK_REMAINING_MIP_LEVELS}};

    VK_CHECK_RESULT(vkCreateImageView(VKDEVICE, &view_ci, NULL, &backend->view), "[Vulkan] Cannot create vulkan image view");

    return view;
}

void vulkan_device_destroy_texture_res_view(gfx_resource_view* view)
{
    uuid_destroy(&view->uuid);

    vkDestroyImageView(VKDEVICE, ((tex_resource_view_backend*) (view->backend))->view, NULL);
    free(view->backend);
    view->backend = NULL;
}

//--------------------------------------------------------
// RHI
//--------------------------------------------------------

gfx_frame_sync* vulkan_frame_begin(gfx_context* context)
{
    LOG_INFO("curr_in-flight_frame_idx: %d", context->frame_idx);

    gfx_frame_sync* curr_frame_sync = &context->frame_sync[context->frame_idx];

    vulkan_wait_on_previous_cmds(curr_frame_sync);
    vulkan_acquire_image(&context->swapchain, curr_frame_sync);

    memset(context->cmd_queue.cmds, 0, context->cmd_queue.cmds_count * sizeof(gfx_cmd_buf*));
    context->cmd_queue.cmds_count = 0;

    return curr_frame_sync;
}

rhi_error_codes vulkan_begin_render_pass(const gfx_cmd_buf* cmd_buf, gfx_render_pass render_pass, uint32_t backbuffer_index)
{
    VkRenderingAttachmentInfo color_attachments[MAX_RT] = {0};

    if (render_pass.is_swap_pass) render_pass.color_attachments_count = 1;

    for (uint32_t i = 0; i < render_pass.color_attachments_count; i++) {
        gfx_attachment            col_attach  = render_pass.color_attachments[i];
        VkRenderingAttachmentInfo attach_info = {
            .sType       = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
            .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
        memcpy(attach_info.clearValue.color.float32, col_attach.clear_color.raw, sizeof(vec4s));
        if (col_attach.clear) {
            attach_info.loadOp  = VK_ATTACHMENT_LOAD_OP_CLEAR;
            attach_info.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        } else {
            attach_info.loadOp  = VK_ATTACHMENT_LOAD_OP_LOAD;
            attach_info.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        }
        if (render_pass.is_swap_pass) {
            if (!render_pass.swapchain)
                LOG_ERROR("[Vulkan] pass is marked as swap pass but swapchain is empty");
            attach_info.imageView = ((swapchain_backend*) (render_pass.swapchain->backend))->backbuffer_views[backbuffer_index];
        } else {
            // TODO: use gfx_texture->backend as imageview
        }

        color_attachments[i] = attach_info;
    }

    VkRenderingInfo info = {
        .sType      = VK_STRUCTURE_TYPE_RENDERING_INFO,
        .pNext      = NULL,
        .layerCount = 1,
        .renderArea = {
            .offset = {0, 0},
            .extent = {(uint32_t) render_pass.extents.x, (uint32_t) render_pass.extents.y}},
        .colorAttachmentCount = render_pass.color_attachments_count,
        .pColorAttachments    = color_attachments,
        // TODO: Depth Attachments
    };

    if (render_pass.is_swap_pass) {
        info.colorAttachmentCount = 1;
        info.pColorAttachments    = color_attachments;
    }

    vulkan_internal_cmd_begin_rendering(*(VkCommandBuffer*) cmd_buf->backend, &info);

    return Success;
}

rhi_error_codes vulkan_end_render_pass(const gfx_cmd_buf* cmd_buf)
{
    vulkan_internal_cmd_end_rendering(*(VkCommandBuffer*) cmd_buf->backend);
    return Success;
}

rhi_error_codes vulkan_gfx_cmd_enque_submit(gfx_cmd_queue* cmd_queue, const gfx_cmd_buf* cmd_buff)
{
    cmd_queue->cmds[cmd_queue->cmds_count] = cmd_buff;
    cmd_queue->cmds_count++;

    return Success;
}

rhi_error_codes vulkan_frame_end(gfx_context* context)
{
    vulkan_present(&context->swapchain, &context->frame_sync[context->frame_idx]);

    context->frame_idx = (context->frame_idx + 1) % MAX_FRAME_INFLIGHT;

    return Success;
}

//--------------------------------------------------------

// TODO: track fence value using vkGetFenceStatus at wait/reset

rhi_error_codes vulkan_wait_on_previous_cmds(const gfx_frame_sync* in_flight_sync)
{
    VK_CHECK_RESULT(vkWaitForFences(VKDEVICE, 1, (VkFence*) (in_flight_sync->in_flight.backend), true, UINT32_MAX), "cannot wait on in-flight fence");
    VK_CHECK_RESULT(vkResetFences(VKDEVICE, 1, (VkFence*) (in_flight_sync->in_flight.backend)), "cannot reset above in-flight fence");

    return Success;
}

rhi_error_codes vulkan_acquire_image(gfx_swapchain* swapchain, const gfx_frame_sync* in_flight_sync)
{
    VkResult result = vkAcquireNextImageKHR(VKDEVICE, ((swapchain_backend*) (swapchain->backend))->swapchain, UINT32_MAX, *(VkSemaphore*) (in_flight_sync->image_ready.backend), NULL, &swapchain->current_backbuffer_idx);
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
        // TODO: re-create swapchain and acquire again
        LOG_ERROR("[Vulkan] Swapchain out of date or suboptimal...recreating...");
        vkDeviceWaitIdle(VKDEVICE);
        vulkan_resize_swapchain(swapchain, swapchain->width, swapchain->height);
    }
    if (result == VK_SUCCESS)
        return Success;
    else
        return FailedSwapAcquire;
}

rhi_error_codes vulkan_gfx_cmd_submit_queue(const gfx_cmd_queue* cmd_queue, gfx_frame_sync* frame_sync)
{
    // Flatten all the gfx_cmd_buff into a VkCommandBuffer Array
    VkCommandBuffer* vk_cmd_buffs = malloc(cmd_queue->cmds_count * sizeof(VkCommandBuffer));
    for (uint32_t i = 0; i < cmd_queue->cmds_count; i++) {
        vk_cmd_buffs[i] = *((VkCommandBuffer*) (cmd_queue->cmds[i]->backend));
    }

    VkPipelineStageFlags waitStages[1] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};

    VkSubmitInfo submitInfo = {
        .sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .pNext                = NULL,
        .commandBufferCount   = cmd_queue->cmds_count,
        .pCommandBuffers      = vk_cmd_buffs,
        .pWaitDstStageMask    = waitStages,
        .waitSemaphoreCount   = 1,
        .pWaitSemaphores      = (VkSemaphore*) (frame_sync->image_ready.backend),
        .signalSemaphoreCount = 1,
        .pSignalSemaphores    = (VkSemaphore*) (frame_sync->rendering_done.backend),
    };

    VkFence signal_fence = *((VkFence*) (frame_sync->in_flight.backend));

    VK_CHECK_RESULT(vkQueueSubmit(s_VkCtx.queues.gfx, 1, &submitInfo, signal_fence), "Failed to submit command buffers");

    return Success;
}

rhi_error_codes vulkan_present(const gfx_swapchain* swapchain, const gfx_frame_sync* frame_sync)
{
    VkPresentInfoKHR presentInfo = {
        .sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .pNext              = NULL,
        .swapchainCount     = 1,
        .pSwapchains        = &((swapchain_backend*) (swapchain->backend))->swapchain,
        .pImageIndices      = &swapchain->current_backbuffer_idx,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores    = (VkSemaphore*) (frame_sync->rendering_done.backend),
        .pResults           = NULL};

    VkResult result = vkQueuePresentKHR(s_VkCtx.queues.gfx, &presentInfo);

    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        vkDeviceWaitIdle(VKDEVICE);
        LOG_ERROR("[Vulkan] Swapchain out of date");
    } else if (result == VK_SUBOPTIMAL_KHR)
        LOG_ERROR("[Vulkan] Swapchain suboptimal");
    else
        VK_CHECK_RESULT(result, "[Vulkan] Failed to present image to presentation engine.");

    return Success;
}

rhi_error_codes vulkan_resize_swapchain(gfx_swapchain* swapchain, uint32_t width, uint32_t height)
{
    swapchain_backend* backend = (swapchain_backend*) (swapchain->backend);
    backend->old_swapchain     = backend->swapchain;
    vulkan_device_destroy_swapchain(swapchain);

    backend->swapchain = VK_NULL_HANDLE;    // THIS SHOULD CAUSE A CRASH, since backend is NULL
    swapchain->width   = width;
    swapchain->width   = height;

    *swapchain = vulkan_device_create_swapchain(width, height);

    return Success;
}

rhi_error_codes vulkan_begin_gfx_cmd_recording(const gfx_cmd_buf* cmd_buf)
{
    VkCommandBufferBeginInfo begin = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .pNext = NULL,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT};

    vkResetCommandBuffer(*(VkCommandBuffer*) cmd_buf->backend, /*VkCommandBufferResetFlagBits*/ 0);
    VK_CHECK_RESULT(vkBeginCommandBuffer(*(VkCommandBuffer*) cmd_buf->backend, &begin), "[Vulkan] Failed to start recoding command buffer");
    return Success;
}

rhi_error_codes vulkan_end_gfx_cmd_recording(const gfx_cmd_buf* cmd_buf)
{
    VK_CHECK_RESULT(vkEndCommandBuffer(*(VkCommandBuffer*) cmd_buf->backend), "[Vulkan] Failed to end recoding command buffer");
    return Success;
}

//--------------------------------------------------------

rhi_error_codes vulkan_set_viewport(const gfx_cmd_buf* cmd_buf, gfx_viewport viewport)
{
    VkCommandBuffer commandBuffer = *(VkCommandBuffer*) cmd_buf->backend;
    VkViewport      vkViewport    = {
        (float) viewport.x,
        (float) viewport.y,
        (float) viewport.width,
        (float) viewport.height,
        (float) viewport.min_depth,
        (float) viewport.max_depth};

    vkCmdSetViewport(commandBuffer, 0, 1, &vkViewport);
    return Success;
}

rhi_error_codes vulkan_set_scissor(const gfx_cmd_buf* cmd_buf, gfx_scissor scissor)
{
    VkCommandBuffer commandBuffer = *(VkCommandBuffer*) cmd_buf->backend;
    VkRect2D        vkScissor     = {
        {scissor.x, scissor.y},
        {scissor.width, scissor.height}};

    vkCmdSetScissor(commandBuffer, 0, 1, &vkScissor);
    return Success;
}

rhi_error_codes vulkan_bind_gfx_pipeline(const gfx_cmd_buf* cmd_buf, gfx_pipeline pipeline)
{
    vkCmdBindPipeline(*(VkCommandBuffer*) cmd_buf->backend, VK_PIPELINE_BIND_POINT_GRAPHICS, ((pipeline_backend*) (pipeline.backend))->pipeline);
    return Success;
}

rhi_error_codes vulkan_bind_compute_pipeline(const gfx_cmd_buf* cmd_buf, gfx_pipeline pipeline)
{
    vkCmdBindPipeline(*(VkCommandBuffer*) cmd_buf->backend, VK_PIPELINE_BIND_POINT_COMPUTE, ((pipeline_backend*) (pipeline.backend))->pipeline);
    return Success;
}

rhi_error_codes vulkan_draw(const gfx_cmd_buf* cmd_buf, uint32_t vertex_count, uint32_t instance_count, uint32_t first_vertex, uint32_t first_instance)
{
    vkCmdDraw(*(VkCommandBuffer*) cmd_buf->backend, vertex_count, instance_count, first_vertex, first_instance);
    return Success;
}