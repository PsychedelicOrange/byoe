#include "backend_vulkan.h"

#include "../core/containers/typed_growable_array.h"
#include "../core/logging/log.h"

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

//--------------------------------------------------------

#define VK_CHECK_RESULT(x, msg) \
    if (x != VK_SUCCESS) { LOG_ERROR(msg); }

#define VK_LAYER_KHRONOS_VALIDATION_NAME "VK_LAYER_KHRONOS_validation"

//--------------------------------------------------------
// Internal Types
//--------------------------------------------------------
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
//---------------------------------

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

typedef struct vulkan_context
{
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

//--------------------------------------------------------

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
    //
    //    for (uint32_t i = 0; i < glfwExtensionsCount; i++) {
    //        void* ext_element = typed_growable_array_get_element(&instance_extensions, i);
    //        strcpy(ext_element, glfwExtensions[i]);
    //    }
    //
    //    // Add any custom extension from the list of supported extensions that you need and are not included by GLFW
    //#ifdef _WIN32
    //    typed_growable_array_append(&instance_extensions, VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
    //#elif defined __APPLE__
    //    typed_growable_array_append(&instance_extensions, VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
    //#endif
    //    typed_growable_array_append(&instance_extensions, VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
    //    typed_growable_array_append(&instance_extensions, VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    //    typed_growable_array_append(&instance_extensions, "VK_EXT_debug_report");
    //
    //    for (uint32_t i = 0; i < instance_extensions.size; i++) {
    //        LOG_WARN("extension %s", (const char*) typed_growable_array_get_element(&instance_extensions, i));
    //    }

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

    volkLoadInstance(instance);

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

    VkPhysicalDevice* gpus = calloc(sizeof(VkPhysicalDevice), total_gpus_count);
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
        VkExtensionProperties* supported_extensions = calloc(sizeof(VkExtensionProperties), extCount);
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

    VkQueueFamilyProperties* q_fam_props = calloc(sizeof(VkQueueFamilyProperties), queueFamilyCount);
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
    queue_backend backend;

    vkGetDeviceQueue(VKDEVICE, indices.gfx, 0, &backend.gfx);
    vkGetDeviceQueue(VKDEVICE, indices.present, 0, &backend.present);
    vkGetDeviceQueue(VKDEVICE, indices.async_compute, 0, &backend.async_compute);

    return backend;
}

bool vulkan_ctx_init(GLFWwindow* window, uint32_t width, uint32_t height)
{
    (void) width;
    (void) height;

    memset(&s_VkCtx, 0, sizeof(vulkan_context));

    if (volkInitialize() != VK_SUCCESS) {
        LOG_ERROR("Failed to initialize Volk");
        return false;
    }

    VKINSTANCE = vulkan_internal_create_instance();

    s_VkCtx.surface = vulkan_internal_create_surface(window, VKINSTANCE);

    // query physical GPUs and create a logical device
    VKGPU = vulkan_internal_select_best_gpu(VKINSTANCE);
    if (VKGPU == VK_NULL_HANDLE) {
        LOG_ERROR("No suitable GPU found!");
        return false;
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

    s_VkCtx.queues = vulkan_internal_create_queues(s_VkCtx.queue_idxs);

    return false;
}

void vulkan_ctx_destroy(void)
{
    free(s_VkCtx.supported_extensions);

    vkDestroySurfaceKHR(VKINSTANCE, s_VkCtx.surface, NULL);
    vkDestroyDevice(s_VkCtx.logical_device, NULL);
    vulkan_internal_destroy_debug_utils_messenger(VKINSTANCE, s_VkCtx.debug_messenger, NULL);
    vkDestroyInstance(VKINSTANCE, NULL);
}

//--------------------------------------------------------

rhi_error_codes vulkan_draw(void)
{
    return FailedUnknown;
}
