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
// - [ ] basic rendering using begin rendering API for drawing a screen quad without vertices --> KHR_dynamic_rendering renderpass API
//      - [x] shader loading API
//      - [ ] pipeline API
//      - [ ] draw API
// - [ ] Descriptors API + pipeline layout handling etc.
// - [ ] UBOs + Push constants API + setup descriptor sets for the resources x2
// - [ ] texture_2d (rw views) API for binding the r/w texture to composition pass
// - [ ] CS dispatch -> SDF raymarching shader
// - [ ] Barriers (image memory) and Transition layout API + single time command buffers
// ----------------------> renderer_backend Draft-1

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
        case point: return VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
        case line: return VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
        case triangle: return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        default: return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    }
}

// Function to translate blend operations
static VkBlendOp vulkan_util_blend_op_translate(gfx_blend_op blend_op)
{
    switch (blend_op) {
        case add: return VK_BLEND_OP_ADD;
        case subtract: return VK_BLEND_OP_SUBTRACT;
        case reverse_subtract: return VK_BLEND_OP_REVERSE_SUBTRACT;
        case min: return VK_BLEND_OP_MIN;
        case max: return VK_BLEND_OP_MAX;
        default: return VK_BLEND_OP_ADD;
    }
}

// Function to translate blend factors
static VkBlendFactor vulkan_util_blend_factor_translate(gfx_blend_factor blend_factor)
{
    switch (blend_factor) {
        case zero: return VK_BLEND_FACTOR_ZERO;
        case one: return VK_BLEND_FACTOR_ONE;
        case src_color: return VK_BLEND_FACTOR_SRC_COLOR;
        case one_minus_src_color: return VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR;
        case dst_color: return VK_BLEND_FACTOR_DST_COLOR;
        case one_minus_dst_color: return VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR;
        case src_alpha: return VK_BLEND_FACTOR_SRC_ALPHA;
        case one_minus_src_alpha: return VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        case dst_alpha: return VK_BLEND_FACTOR_DST_ALPHA;
        case one_minus_dst_alpha: return VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;
        case constant_color: return VK_BLEND_FACTOR_CONSTANT_COLOR;
        case one_minus_constant_color: return VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR;
        case constant_alpha: return VK_BLEND_FACTOR_CONSTANT_ALPHA;
        case one_minus_constant_alpha: return VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_ALPHA;
        case src_alpha_saturate: return VK_BLEND_FACTOR_SRC_ALPHA_SATURATE;
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
        case back: return VK_CULL_MODE_BACK_BIT;
        case front: return VK_CULL_MODE_FRONT_BIT;
        case back_and_front: return VK_CULL_MODE_FRONT_AND_BACK;
        case none: return VK_CULL_MODE_NONE;
        default: return VK_CULL_MODE_BACK_BIT;
    }
}

static VkCompareOp vulkan_util_compare_op_translate(gfx_compare_op compare_op)
{
    switch (compare_op) {
        case never: return VK_COMPARE_OP_NEVER;
        case less: return VK_COMPARE_OP_LESS;
        case equal: return VK_COMPARE_OP_EQUAL;
        case less_or_equal: return VK_COMPARE_OP_LESS_OR_EQUAL;
        case greater: return VK_COMPARE_OP_GREATER;
        case not_equal: return VK_COMPARE_OP_NOT_EQUAL;
        case greater_or_equal: return VK_COMPARE_OP_GREATER_OR_EQUAL;
        case always: return VK_COMPARE_OP_ALWAYS;
        default: return VK_COMPARE_OP_ALWAYS;    // Fallback to always as default
    }
}

// static VkFormat vulkan_util_format_translate(gfx_format format)
// {
//     switch (format) {
//         case r8int: return VK_FORMAT_R8_SINT;
//         case r8uint: return VK_FORMAT_R8_UINT;
//         case r8f: return VK_FORMAT_R8_UNORM;
//         case r16f: return VK_FORMAT_R16_SFLOAT;
//         case r32f: return VK_FORMAT_R32_SFLOAT;
//         case r32uint: return VK_FORMAT_R32_UINT;
//         case r32int: return VK_FORMAT_R32_SINT;
//         case rgbint: return VK_FORMAT_R8G8B8_SINT;
//         case rgbuint: return VK_FORMAT_R8G8B8_UINT;
//         case rgbunorm: return VK_FORMAT_R8G8B8_UNORM;
//         case rgb32f: return VK_FORMAT_R32G32B32_SFLOAT;
//         case rgbaint: return VK_FORMAT_R8G8B8A8_SINT;
//         case rgbauint: return VK_FORMAT_R8G8B8A8_UINT;
//         case rgbaunorm: return VK_FORMAT_R8G8B8A8_UNORM;
//         case rgba32f: return VK_FORMAT_R32G32B32A32_SFLOAT;
//         case depth32f: return VK_FORMAT_D32_SFLOAT;
//         case depth16unorm: return VK_FORMAT_D16_UNORM;
//         case depthstencil: return VK_FORMAT_D24_UNORM_S8_UINT;
//         case screen: return VK_FORMAT_B8G8R8A8_UNORM;
//         default: return VK_FORMAT_UNDEFINED;
//     }
// }

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
    // Debug create info
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

    const char* instance_extensions[] = {
#ifdef _WIN32
        VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
#endif
        VK_KHR_SURFACE_EXTENSION_NAME,
        VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME,
        VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
        "VK_EXT_debug_report",
    };

    // TODO: Use a StringView DS for this to work fine
    //    TypedGrowableArray instance_extensions = typed_growable_array_create(sizeof(const char*) * 255, glfwExtensionsCount);

    VkInstanceCreateInfo info_ci = {
        .sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pNext                   = &debug_ci,    // struct injection
        .flags                   = 0,
        .pApplicationInfo        = &app_info,
        .enabledLayerCount       = sizeof(instance_layers) / sizeof(const char*),
        .ppEnabledLayerNames     = instance_layers,
        .enabledExtensionCount   = sizeof(instance_extensions) / sizeof(const char*),
        .ppEnabledExtensionNames = instance_extensions};

    VkInstance instance;
    VK_CHECK_RESULT(vkCreateInstance(&info_ci, NULL, &instance), "Failed to create vulkan instance");

    //free(glfwExtensions);
    //typed_growable_array_free(&instance_extensions);

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
    }
    return indices;
}

static TypedGrowableArray vulkan_internal_create_queue_family_infos(queue_indices indices)
{
    const int          NUM_QUEUES       = 3;    // Graphics, Present, Async Compute
    TypedGrowableArray queueFamilyInfos = typed_growable_array_create(sizeof(VkDeviceQueueCreateInfo), NUM_QUEUES);

    static float            queuePriority   = 1.0f;
    VkDeviceQueueCreateInfo queueCreateInfo = {
        .sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .queueFamilyIndex = indices.gfx,
        .queueCount       = 1,
        .pQueuePriorities = &queuePriority};

    VkDeviceQueueCreateInfo* gfx_q_ci = typed_growable_array_get_element(&queueFamilyInfos, 0);
    memcpy(gfx_q_ci, &queueCreateInfo, sizeof(VkDeviceQueueCreateInfo));

    VkDeviceQueueCreateInfo* present_q_ci = typed_growable_array_get_element(&queueFamilyInfos, 1);
    queueCreateInfo.queueFamilyIndex      = indices.present;
    memcpy(present_q_ci, &queueCreateInfo, sizeof(VkDeviceQueueCreateInfo));

    VkDeviceQueueCreateInfo* async_compute_q_ci = typed_growable_array_get_element(&queueFamilyInfos, 2);
    queueCreateInfo.queueFamilyIndex            = indices.async_compute;
    memcpy(async_compute_q_ci, &queueCreateInfo, sizeof(VkDeviceQueueCreateInfo));

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
        .imageUsage            = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT,
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

gfx_cmd_buf vulkan_create_gfx_cmd_buf(gfx_cmd_pool* pool)
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
        frame_sync.image_ready.visibility = gpu;
        frame_sync.image_ready.value      = UINT32_MAX;
        frame_sync.image_ready.backend    = malloc(sizeof(VkSemaphore));
        vulkan_internal_create_sema(frame_sync.image_ready.backend);
        VK_TAG_OBJECT("image_ready_sema", VK_OBJECT_TYPE_SEMAPHORE, *(uint64_t*) frame_sync.image_ready.backend);
    }

    {
        uuid_generate(&frame_sync.rendering_done.uuid);
        frame_sync.rendering_done.visibility = gpu;
        frame_sync.rendering_done.value      = UINT32_MAX;
        frame_sync.rendering_done.backend    = malloc(sizeof(VkSemaphore));
        vulkan_internal_create_sema(frame_sync.rendering_done.backend);
        VK_TAG_OBJECT("rendering_done_sema", VK_OBJECT_TYPE_SEMAPHORE, *(uint64_t*) frame_sync.image_ready.backend);
    }

    if (!g_gfxConfig.use_timeline_semaphores) {
        uuid_generate(&frame_sync.in_flight.uuid);
        frame_sync.in_flight.visibility = cpu;
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

    if (!g_gfxConfig.use_timeline_semaphores && frame_sync->in_flight.visibility == cpu)
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
        backend->stage   = cs;

        backend->modules.CS = vulkan_internal_create_shader_handle(spv_file_path);

        backend->stage_ci = (VkPipelineShaderStageCreateInfo) {
            .sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage  = VK_SHADER_STAGE_COMPUTE_BIT,
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
        backend_vs->stage = vs;

        backend_vs->modules.VS = vulkan_internal_create_shader_handle(spv_file_path_vs);

        backend_vs->stage_ci = (VkPipelineShaderStageCreateInfo) {
            .sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage  = VK_SHADER_STAGE_VERTEX_BIT,
            .module = backend_vs->modules.VS};
    }

    shader_backend* backend_ps = malloc(sizeof(shader_backend));
    if (backend_ps) {
        shader.stages.PS  = backend_ps;
        backend_ps->stage = ps;

        backend_ps->modules.PS = vulkan_internal_create_shader_handle(spv_file_path_ps);

        backend_ps->stage_ci = (VkPipelineShaderStageCreateInfo) {
            .sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage  = VK_SHADER_STAGE_FRAGMENT_BIT,
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

static gfx_pipeline vulkan_internal_create_gfx_pipeline(gfx_pipeline_create_info info)
{
    gfx_pipeline pipeline = {0};
    uuid_generate(&pipeline.uuid);

    //----------------------------
    // Vertex Input Layout Stage
    //----------------------------
    // DISABLED FOR TESTING SCREEN QUAD
    // VkVertexInputBindingDescription vertex_binding_description = {
    //     .binding   = 0,
    //     .inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
    //     .stride    = 0};

    // VkVertexInputAttributeDescription input_attrib = {
    //     .binding  = 0,
    //     .location = 0,
    //     .format   = VK_FORMAT_R32G32B32A32_SFLOAT,
    //     .offset   = 0};

    // VkPipelineVertexInputStateCreateInfo vertex_input_sci = {
    //     .sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
    //     .pNext                           = NULL,
    //     .vertexBindingDescriptionCount   = 0,
    //     .pVertexBindingDescriptions      = NULL,
    //     .vertexAttributeDescriptionCount = 0,
    //     .pVertexAttributeDescriptions    = NULL};

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

    const uint32_t NUM_DYNAMIC_STATES                            = 2;
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
        .blendEnable         = VK_TRUE,
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
    // TODO: stencil testing is always disabled so change that

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
        .front                 = depthStencilSCI.back};

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
        .depthAttachmentFormat   = VK_FORMAT_UNDEFINED};

    // VkFormat formats[MAX_RT];
    // for (auto& attachment: pipelineInfo.colorAttachmentFormats)
    //     formats.push_back(VKUtilities::TextureFormatToVK(attachment));

    //----------------------------
    // Graphics Pipeline
    //----------------------------
    // VkGraphicsPipelineCreateInfo graphics_pipeline_ci = {
    //     .sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
    //     .pNext               = pipeline_rendering_ci,
    //     .layout              =,                   // Assume m_PipelineLayout is predefined
    //     .renderPass          = VK_NULL_HANDLE,    // Render pass setup is omitted here
    //     .pVertexInputState   = NULL,
    //     .pInputAssemblyState = &input_assembly_sci,
    //     .pViewportState      = &viewport_sci,
    //     .pRasterizationState = &rasterization_sci,
    //     .pColorBlendState    = &color_blend_sci,
    //     .pDynamicState       = &dynamic_state_ci};

    // std::vector<VkPipelineShaderStageCreateInfo> shaderStages = static_cast<VKShader*>(shader)->getShaderStages();

    VkGraphicsPipelineCreateInfo graphics_pipeline_ci = {
        .sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .pNext               = &pipeline_rendering_ci,
        .layout              = VK_NULL_HANDLE,
        .basePipelineHandle  = VK_NULL_HANDLE,
        .basePipelineIndex   = -1,
        .pVertexInputState   = NULL,
        .pInputAssemblyState = &input_assembly_sci,
        .pRasterizationState = &rasterization_sci,
        .pColorBlendState    = &color_blend_sci,
        .pTessellationState  = NULL,
        .pMultisampleState   = &multiSampleSCI,
        .pDynamicState       = &dynamic_state_ci,
        .pViewportState      = &viewport_sci,
        .pDepthStencilState  = &depthStencilSCI,
        .pStages             = NULL,
        .stageCount          = 0,
        .renderPass          = VK_NULL_HANDLE};

    pipeline_backend* backend = malloc(sizeof(pipeline_backend));
    VK_CHECK_RESULT(vkCreateGraphicsPipelines(VKDEVICE, VK_NULL_HANDLE, 1, &graphics_pipeline_ci, NULL, (VkPipeline*)backend->pipeline), "[Vulkan] could not create grapihcs pipeline");
    // VK_TAG_OBJECT(bufferName, VK_OBJECT_TYPE_PIPELINE, (uint64_t) m_Pipeline);

    pipeline.backend = backend;

    return pipeline;
}

gfx_pipeline vulkan_device_create_pipeline(gfx_pipeline_create_info info)
{
    if (info.type == graphics) return vulkan_internal_create_gfx_pipeline(info);
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

//--------------------------------------------------------

// TODO: Move come of these functions to front end use rhi_ wrappers instead

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

rhi_error_codes vulkan_begin_render_pass(gfx_cmd_buf* cmd_buf, gfx_render_pass render_pass, uint32_t backbuffer_index)
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

rhi_error_codes vulkan_end_render_pass(gfx_cmd_buf* cmd_buf)
{
    vulkan_internal_cmd_end_rendering(*(VkCommandBuffer*) cmd_buf->backend);
    return Success;
}

rhi_error_codes vulkan_gfx_cmd_enque_submit(gfx_cmd_queue* cmd_queue, gfx_cmd_buf* cmd_buff)
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

rhi_error_codes vulkan_gfx_cmd_submit_queue(gfx_cmd_queue* cmd_queue, gfx_frame_sync* frame_sync)
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

rhi_error_codes vulkan_present(gfx_swapchain* swapchain, gfx_frame_sync* frame_sync)
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

rhi_error_codes vulkan_begin_gfx_cmd_recording(gfx_cmd_buf* cmd_buf)
{
    VkCommandBufferBeginInfo begin = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .pNext = NULL,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT};

    vkResetCommandBuffer(*(VkCommandBuffer*) cmd_buf->backend, /*VkCommandBufferResetFlagBits*/ 0);
    VK_CHECK_RESULT(vkBeginCommandBuffer(*(VkCommandBuffer*) cmd_buf->backend, &begin), "[Vulkan] Failed to start recoding command buffer");
    return Success;
}

rhi_error_codes vulkan_end_gfx_cmd_recording(gfx_cmd_buf* cmd_buf)
{
    VK_CHECK_RESULT(vkEndCommandBuffer(*(VkCommandBuffer*) cmd_buf->backend), "[Vulkan] Failed to end recoding command buffer");
    return Success;
}

//--------------------------------------------------------

rhi_error_codes vulkan_draw(void)
{
    return FailedUnknown;
}
