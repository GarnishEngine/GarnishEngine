#include "vulkan_renderer.hpp"

#include <iostream>
#include <stdexcept>

namespace garnish {
bool VulkanRenderDevice::init_vulkan() {
    return true;
}

bool VulkanRenderDevice::create_instance() {
    vk::ApplicationInfo applicationInfo(
        "Garnish",
        1,
        "GarnishEngine",
        1,
        vk::makeApiVersion(0, 1, 1, 0),
        nullptr
    );

    vk::InstanceCreateInfo instanceCreateInfo({}, &applicationInfo);
    uint32_t extensionCount = 0;
    const char* const* extensions =
        SDL_Vulkan_GetInstanceExtensions(&extensionCount);

    vk::InstanceCreateInfo(
        {},
        &applicationInfo,
        0,
        {},
        extensionCount,
        extensions,
        nullptr
    );

    gInstance = vk::createInstance(instanceCreateInfo, nullptr, {});
}

bool VulkanRenderDevice::pick_physical_device() {
    std::vector physicalDevices = gInstance.enumeratePhysicalDevices({});
    if (physicalDevices.size() == 0) {
        throw std::runtime_error("no physical devices for vulkan");
    }
    int i = 0;
    for (vk::PhysicalDevice& physicalDevice : physicalDevices) {
        std::cerr << "device: " << i++ << '\n';
        std::vector<vk::LayerProperties> properties =
            physicalDevice.enumerateDeviceLayerProperties();
        for (VkLayerProperties property : properties) {
            std::cerr << property.layerName << ": " << property.description
                      << '\n';
        }
    }
}

}  // namespace garnish