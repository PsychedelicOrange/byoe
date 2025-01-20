#include "backend_vulkan.h"

#include "../core/logging/log.h"

#define VK_NO_PROTOTYPES
#include <vulkan/vulkan.h>

#include <volk.h>

#define VK_CHECK_RESULT(x, msg) \
    if (x != VK_SUCCESS) { LOG_ERROR(msg); }

//--------------------------------------------------------
// Internal Types
//--------------------------------------------------------
typedef struct vulkan_context
{
    VkInstance instance;
} vulkan_context;

static vulkan_context s_VkCtx;

//--------------------------------------------------------

// Debug callback
VkBool32 vulkan_backend_debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageTypes, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData)
{
    (void) messageSeverity;
    (void) messageTypes;
    (void) pCallbackData;
    (void) pUserData;

    return VK_TRUE;
}

//--------------------------------------------------------
// Context
//--------------------------------------------------------

static VkInstance vulkan_internal_create_ctx()
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

    VkInstanceCreateInfo info_ci = {
        .sType            = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pNext            = &debug_ci,    // struct injection
        .flags            = 0,
        .pApplicationInfo = &app_info,
    };

    VkInstance instance;
    VK_CHECK_RESULT(vkCreateInstance(&info_ci, NULL, &instance), "Failed to create vulkan instance");

    return instance;
}

int vulkan_internal_query_physical_GPUs(void)
{
    return 0;
}

VkPhysicalDevice vulkan_internal_select_best_gpu(void)
{
    VkPhysicalDevice dummy = {0};
    return dummy;
}

VkDevice vulkan_internal_create_logical_device(VkPhysicalDevice gpu)
{
    (void) gpu;

    VkDevice dummy = {0};
    return dummy;
}

bool vulkan_ctx_init(uint32_t width, uint32_t height)
{
    (void) width;
    (void) height;

    if (volkInitialize() != VK_SUCCESS) {
        LOG_ERROR("Failed to initialize Volk");
        return false;
    }

    s_VkCtx.instance = vulkan_internal_create_ctx();

    volkLoadInstance(s_VkCtx.instance);

    // query physical GPUs and create a logical device

    return true;
}

//--------------------------------------------------------

rhi_error_codes vulkan_draw()
{
    return FailedUnknown;
}
