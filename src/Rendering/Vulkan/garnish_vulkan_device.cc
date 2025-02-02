#include "garnish_vulkan_device.hpp"

namespace garnish::vk { 
    void gvr_device::createInstance(VkInstanceCreateInfo createInfo, VkApplicationInfo appInfo, bool enableValidationLayers) {
        validationLayersEnabled = enableValidationLayers;
        if (enableValidationLayers) {
            throw std::runtime_error("validation layers requested, but not available!");
        }
    
        std::vector<const char*> extensions = getRequiredExtensions();        
        extensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
        extensions.emplace_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
        createInfo.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;

        createInfo.enabledExtensionCount = (uint32_t) extensions.size();
        createInfo.ppEnabledExtensionNames = extensions.data();

        VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
        if (enableValidationLayers) {
            createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
            createInfo.ppEnabledLayerNames = validationLayers.data();

            // populateDebugMessengerCreateInfo(debugCreateInfo);
            // createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*) &debugCreateInfo;
        } else {
            createInfo.enabledLayerCount = 0;

            createInfo.pNext = nullptr;
        }

        if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
            throw std::runtime_error("failed to create instance!");
        }
    }
    std::vector<const char*> gvr_device::getRequiredExtensions() {
        uint32_t sdlExtensionCount = 0;
        const char *const *sdlExtensions;
        sdlExtensions = SDL_Vulkan_GetInstanceExtensions(&sdlExtensionCount);

        std::vector<const char*> extensions(sdlExtensions, sdlExtensions + sdlExtensionCount);

        if (validationLayersEnabled) {
            extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        }

        return extensions;
    }
    
    // void gvr_device::populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& debugCreateInfo) {
    //     debugCreateInfo = {};
    //     debugCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    //     debugCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    //     debugCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |    VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    //     debugCreateInfo.pfnUserCallback = debugCallback;
    // }

    // VKAPI_ATTR VkBool32 VKAPI_CALL gvr_device::debugCallback (
    //     VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    //     VkDebugUtilsMessageTypeFlagsEXT messageType,
    //     const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    //     void* pUserData) {

    //     std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

    //     return VK_FALSE;
    // }

    // void gvr_device::createSurface() {

    // }

    void gvr_device::pickPhysicalDevice() {

    }
    void gvr_device::createLogicalDevice() {

    }

    void gvr_device::setupDebugMessenger() {

    }


}