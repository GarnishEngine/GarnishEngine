#include "vulkan_renderer.hpp"

#include <SDL3/SDL_events.h>
#include <SDL3/SDL_vulkan.h>
#include <ecs_controller.h>
#include <stb_image.h>
#include <tiny_obj_loader.h>

#include <Physics/physics_system.hpp>
#include <Utility/camera.hpp>
#include <Utility/log.hpp>
#include <cstddef>
#include <iostream>
#include <read_file.hpp>
#include <set>
#include <span>
#include <sstream>
#include <stdexcept>
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_enums.hpp>
#include <vulkan/vulkan_structs.hpp>

#include "geometry.hpp"

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE;  // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

namespace {
VKAPI_ATTR vk::Bool32 VKAPI_CALL debugMessageFunc(
    vk::DebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    vk::DebugUtilsMessageTypeFlagsEXT messageTypes,
    const vk::DebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* /*pUserData*/
) {
    std::ostringstream message;
    message << vk::to_string(messageSeverity) << ": "
            << vk::to_string(messageTypes) << ":\n";
    message << "\tmessageIDName   = <" << pCallbackData->pMessageIdName
            << ">\n";
    message << "\tmessageIdNumber = "
            << static_cast<uint32_t>(pCallbackData->messageIdNumber) << "\n";
    message << "\tmessage         = <" << pCallbackData->pMessage << ">\n";
    auto queueLabels =
        std::span(pCallbackData->pQueueLabels, pCallbackData->queueLabelCount);
    for (const auto& lbl : queueLabels) {
        message << "\tQueue Label: <" << lbl.pLabelName << ">\n";
    }
    auto cmdBufLabels = std::span(
        pCallbackData->pCmdBufLabels,
        pCallbackData->cmdBufLabelCount
    );
    for (const auto& lbl : cmdBufLabels) {
        message << "\tCmdBuf Label: <" << lbl.pLabelName << ">\n";
    }
    auto objects =
        std::span(pCallbackData->pObjects, pCallbackData->objectCount);
    uint32_t objIndex = 0;
    for (const auto& obj : objects) {
        message << "\tObject " << objIndex++
                << " type=" << vk::to_string(obj.objectType)
                << " handle=" << obj.objectHandle << "\n";
        if (obj.pObjectName) {
            message << "\t  name=<" << obj.pObjectName << ">\n";
        }
    }
    std::cerr << message.str() << '\n';
    return vk::False;
}

inline vk::SurfaceFormatKHR choose_surface_format(
    const std::vector<vk::SurfaceFormatKHR>& formats
) {
    for (const auto& f : formats) {
        if (f.format == vk::Format::eB8G8R8A8Srgb &&
            f.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear) {
            return f;
        }
    }
    return formats.front();
}
}  // anonymous namespace

namespace garnish::vulkan {
bool VulkanRenderDevice::init(const InitInfo& info) {
    window = static_cast<SDL_Window*>(info.nativeWindow);
    init_vulkan(info);
    return true;
}

bool VulkanRenderDevice::init_vulkan(const InitInfo& info) {
    create_instance();
    setup_debug_messenger();
    create_surface();
    pick_physical_device();
    create_logical_device();

    create_swap_chain();
    create_image_views();

    create_texture_sampler();
    create_render_pass();
    create_descriptor_set_layout();
    create_graphics_pipeline(info.assetPath);
    create_command_pool();
    create_color_resources();
    create_depth_resources();
    create_framebuffers();

    create_vertex_buffer();
    create_index_buffer();
    create_uniform_buffers();
    create_model_buffers(kInitialModelCapacity);

    create_descriptor_pool();
    create_descriptor_sets();
    load_texture(info.assetPath + "Textures/viking_room.png");

    create_command_buffers();
    create_sync_objects();
    setup_mesh(info.assetPath + "Models/viking_room.obj");
    return true;
}

void VulkanRenderDevice::cleanup() {
    gvDevice_.waitIdle();
    cleanup_swap_chain();

    for (auto& texture : gvTextures_) {
        gvDevice_.destroyImageView(texture.textureImageView, nullptr);
        gvDevice_.destroyImage(texture.textureImage, nullptr);
        gvDevice_.freeMemory(texture.textureMemory, nullptr);
    }

    gvDevice_.destroySampler(gvTextureSampler_, nullptr);

    for (size_t i = 0; i < kMAX_FRAMES_IN_FLIGHT; i++) {
        gvDevice_.unmapMemory(uniformBuffersMemory_[i]);
        gvDevice_.destroyBuffer(uniformBuffers_[i], nullptr);
        gvDevice_.freeMemory(uniformBuffersMemory_[i], nullptr);
    }

    destroy_model_buffers();

    gvDevice_.destroyDescriptorPool(gvDescriptorPool_, nullptr);
    gvDevice_.destroyDescriptorSetLayout(gvDescriptorSetLayout_, nullptr);

    gvDevice_.destroyBuffer(indexBuffer_, nullptr);
    gvDevice_.freeMemory(indexBufferMemory_, nullptr);

    gvDevice_.destroyBuffer(vertexBuffer_, nullptr);
    gvDevice_.freeMemory(vertexBufferMemory_, nullptr);

    gvDevice_.destroyPipeline(gvPipeline_, nullptr);
    gvDevice_.destroyPipelineLayout(gvPipelineLayout_, nullptr);

    gvDevice_.destroyRenderPass(gvRenderPass_, nullptr);

    for (auto& sem : imageAvailableSemaphores_) {
        if (sem) gvDevice_.destroySemaphore(sem);
    }
    for (auto& fence : inFlightFences_) {
        if (fence) gvDevice_.destroyFence(fence, nullptr);
    }

    gvDevice_.destroyCommandPool(gvCommandPool_, nullptr);

    gvDevice_.destroy();

    if (kEnableValidationLayers) {
        gvInstance_.destroyDebugUtilsMessengerEXT(gvDebugMessenger_);
    }

    gvInstance_.destroySurfaceKHR(gvSurface_);

    gvInstance_.destroy();
}

void VulkanRenderDevice::update(ECSController& world) {
    draw_frame(world);
}

uint32_t VulkanRenderDevice::setup_mesh(const Geometry& geometry) {
    std::vector<uint32_t> indices;
    std::vector<Vertex> vertices;
    std::unordered_map<Vertex, uint32_t> uniqueVertices{};

    indices.resize(geometry.indices.size());
    for (int i = 0; i < indices.size(); ++i) {
        indices[i] = geometry.indices[i];
    }

    vertices.resize(geometry.vertices.size());
    for (int i = 0; i < vertices.size(); ++i) {
        vertices[i].position = geometry.vertices[i].position;
        vertices[i].normal = geometry.vertices[i].normal;
        vertices[i].uv = geometry.vertices[i].uv;
    }

    gvMeshes_.push_back(
        GVMesh{
            .firstVertex =
                static_cast<uint32_t>(totalVertexBytes_ / sizeof(Vertex)),
            .vertexCount = static_cast<uint32_t>(vertices.size()),
            .firstIndex =
                static_cast<uint32_t>(totalIndexBytes_ / sizeof(uint32_t)),
            .indexCount = static_cast<uint32_t>(indices.size())
        }
    );

    vk::DeviceSize vertexBufferSize = sizeof(Vertex) * vertices.size();
    vk::DeviceSize indexBufferSize = sizeof(uint32_t) * indices.size();

    vk::Buffer vertexStagingBuffer;
    vk::DeviceMemory vertexStagingBufferMemory;

    vk::Buffer indexStagingBuffer;
    vk::DeviceMemory indexStagingBufferMemory;

    create_buffer(
        vertexBufferSize,
        vk::BufferUsageFlagBits::eTransferSrc,
        vk::MemoryPropertyFlagBits::eHostVisible |
            vk::MemoryPropertyFlagBits::eHostCoherent,
        vertexStagingBuffer,
        vertexStagingBufferMemory
    );
    memcpy(
        gvDevice_.mapMemory(vertexStagingBufferMemory, 0, vertexBufferSize),
        vertices.data(),
        (size_t)vertexBufferSize
    );
    gvDevice_.unmapMemory(vertexStagingBufferMemory);

    create_buffer(
        indexBufferSize,
        vk::BufferUsageFlagBits::eTransferSrc,
        vk::MemoryPropertyFlagBits::eHostVisible |
            vk::MemoryPropertyFlagBits::eHostCoherent,
        indexStagingBuffer,
        indexStagingBufferMemory
    );
    memcpy(
        gvDevice_.mapMemory(indexStagingBufferMemory, 0, indexBufferSize),
        indices.data(),
        (size_t)indexBufferSize
    );
    gvDevice_.unmapMemory(indexStagingBufferMemory);

    copy_buffer(
        vertexStagingBuffer,
        vertexBuffer_,
        vertexBufferSize,
        totalVertexBytes_
    );

    copy_buffer(
        indexStagingBuffer,
        indexBuffer_,
        indexBufferSize,
        totalIndexBytes_
    );

    gvDevice_.destroyBuffer(vertexStagingBuffer);
    gvDevice_.freeMemory(vertexStagingBufferMemory);

    gvDevice_.destroyBuffer(indexStagingBuffer);
    gvDevice_.freeMemory(indexStagingBufferMemory);

    totalVertexBytes_ += sizeof(Vertex) * vertices.size();
    totalIndexBytes_ += sizeof(uint32_t) * indices.size();
    return gvMeshes_.size() - 1;
}

bool VulkanRenderDevice::create_instance() {
    vk::detail::DynamicLoader dl;
    auto vkGetInstanceProcAddr =
        dl.getProcAddress<PFN_vkGetInstanceProcAddr>("vkGetInstanceProcAddr");
    VULKAN_HPP_DEFAULT_DISPATCHER.init(vkGetInstanceProcAddr);

    vk::ApplicationInfo applicationInfo{
        "Garnish",
        vk::makeApiVersion(0, 1, 1, 0),
        "GarnishEngine",
        vk::makeApiVersion(0, 1, 1, 0),
        VK_API_VERSION_1_2,
        nullptr
    };

    uint32_t sdlExtensionCount = 0;
    const char* const* sdlExtensions =
        SDL_Vulkan_GetInstanceExtensions(&sdlExtensionCount);

    std::span<const char* const> sdlExtSpan{sdlExtensions, sdlExtensionCount};
    std::vector<const char*> extensions(sdlExtSpan.begin(), sdlExtSpan.end());
    extensions.push_back(vk::KHRGetPhysicalDeviceProperties2ExtensionName);

    auto instExts = vk::enumerateInstanceExtensionProperties();
    for (auto& ext : instExts) {
        if (strcmp(
                ext.extensionName,
                vk::KHRPortabilityEnumerationExtensionName
            ) == 0) {
            extensions.push_back(vk::KHRPortabilityEnumerationExtensionName);
        }
    }

    if (kEnableValidationLayers) {
        extensions.push_back(vk::EXTDebugUtilsExtensionName);
    }

    uint32_t layerCount = kEnableValidationLayers
                              ? static_cast<uint32_t>(validationLayers.size())
                              : 0;
    const char* const* layers =
        kEnableValidationLayers ? validationLayers.data() : nullptr;

    vk::InstanceCreateInfo instanceCreateInfo(
        vk::InstanceCreateFlagBits::eEnumeratePortabilityKHR,
        &applicationInfo,
        layerCount,
        layers,
        extensions.size(),
        extensions.data()
    );
    bool haveValidationLayer = false;
    for (auto const& lp : vk::enumerateInstanceLayerProperties()) {
        if (strcmp(lp.layerName, "VK_LAYER_KHRONOS_validation") == 0) {
            haveValidationLayer = true;
            break;
        }
    }

    if (!haveValidationLayer) {
        throw std::runtime_error(
            "missing one or more needed validation layers"
        );
    }

    gvInstance_ = vk::createInstance(instanceCreateInfo, nullptr);
    VULKAN_HPP_DEFAULT_DISPATCHER.init(gvInstance_);
    return true;
}

bool VulkanRenderDevice::setup_debug_messenger() {
    if (!kEnableValidationLayers) return false;
    auto pfnVkCreateDebugUtilsMessengerEXT = reinterpret_cast<  // NOLINT
        PFN_vkCreateDebugUtilsMessengerEXT>(
        gvInstance_.getProcAddr("vkCreateDebugUtilsMessengerEXT")
    );
    vk::DebugUtilsMessengerCreateInfoEXT createInfo{
        {},
        vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose |
            vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
            vk::DebugUtilsMessageSeverityFlagBitsEXT::eError,
        vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
            vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation |
            vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance,
        &debugMessageFunc
    };
    gvDebugMessenger_ = gvInstance_.createDebugUtilsMessengerEXT(createInfo);
    return true;
}

bool VulkanRenderDevice::pick_physical_device() {
    std::vector physicalDevices = gvInstance_.enumeratePhysicalDevices();
    if (physicalDevices.size() == 0) {
        throw std::runtime_error("no physical devices for vulkan");
    }

    gvPhysicalDevice_ = nullptr;
    for (auto& physicalDevice : physicalDevices) {
        if (is_device_suitable(physicalDevice)) {
            gvPhysicalDevice_ = physicalDevice;
            msaaSamples_ = max_usable_sample_count();
            break;
        }
    }

    if (!gvPhysicalDevice_) {
        throw std::runtime_error("no suitable physical device");
    }

    vk::PhysicalDeviceProperties2 props2{};
    vk::PhysicalDeviceDescriptorIndexingProperties indexingPropsQuery{};
    props2.pNext = &indexingPropsQuery;
    gvPhysicalDevice_.getProperties2(&props2);
    textureLimit_ = props2.properties.limits.maxPerStageDescriptorSampledImages;
    indexingProperties_ = indexingPropsQuery;

    vk::PhysicalDeviceVulkan12Features vulkan12FeaturesQuery{};
    vk::PhysicalDeviceFeatures2 features2{};
    features2.pNext = &vulkan12FeaturesQuery;
    gvPhysicalDevice_.getFeatures2(&features2);
    vulkan12Features_ = vulkan12FeaturesQuery;

    vk::PhysicalDeviceFeatures2 features2b;
    vk::PhysicalDeviceDescriptorIndexingFeatures indexingFeaturesQuery;
    features2b.pNext = &indexingFeaturesQuery;
    gvPhysicalDevice_.getFeatures2(&features2b);
    supportedIndexingFeatures_ = indexingFeaturesQuery;

    log_debug(
        "Vulkan 1.2 descriptorIndexing: " +
        std::to_string(vulkan12Features_.descriptorIndexing) + "\n" +
        "Descriptor indexing support:\n" + "  runtimeDescriptorArray: " +
        std::to_string(supportedIndexingFeatures_.runtimeDescriptorArray) +
        "\n" + "  descriptorBindingPartiallyBound: " +
        std::to_string(
            supportedIndexingFeatures_.descriptorBindingPartiallyBound
        ) +
        "\n" + "  descriptorBindingVariableDescriptorCount: " +
        std::to_string(
            supportedIndexingFeatures_.descriptorBindingVariableDescriptorCount
        ) +
        "\n" + "  shaderSampledImageArrayNonUniformIndexing: " +
        std::to_string(
            supportedIndexingFeatures_.shaderSampledImageArrayNonUniformIndexing
        ) +
        "\n" + "Descriptor indexing properties (limits):\n" +
        "  maxPerStageDescriptorUpdateAfterBindSamplers: " +
        std::to_string(
            indexingProperties_.maxPerStageDescriptorUpdateAfterBindSamplers
        ) +
        "\n" + "  maxPerStageDescriptorUpdateAfterBindUniformBuffers: " +
        std::to_string(
            indexingProperties_
                .maxPerStageDescriptorUpdateAfterBindUniformBuffers
        ) +
        "\n" + "  maxPerStageDescriptorUpdateAfterBindStorageBuffers: " +
        std::to_string(
            indexingProperties_
                .maxPerStageDescriptorUpdateAfterBindStorageBuffers
        ) +
        "\n" + "  maxPerStageDescriptorUpdateAfterBindSampledImages: " +
        std::to_string(indexingProperties_
                           .maxPerStageDescriptorUpdateAfterBindSampledImages)
    );

    std::vector<vk::ExtensionProperties> availableExtensions =
        gvPhysicalDevice_.enumerateDeviceExtensionProperties();

    for (const auto& ext : availableExtensions) {
        if (strcmp(ext.extensionName, vk::KHRMaintenance3ExtensionName) == 0) {
            vk::PhysicalDeviceMaintenance3Properties maintenance3Props;
            auto props2WithMaintenance = vk::PhysicalDeviceProperties2{};
            props2WithMaintenance.pNext = &maintenance3Props;
            gvPhysicalDevice_.getProperties2(&props2WithMaintenance);

            log_timed(
                "maintenance3Props.maxMemoryAllocationSize: " +
                std::to_string(maintenance3Props.maxMemoryAllocationSize)
            );
            deviceExtensions_.push_back(vk::KHRMaintenance3ExtensionName);
            break;
        }
    }

    for (const auto& extension : availableExtensions) {
        if (strcmp(extension.extensionName, "VK_KHR_portability_subset") == 0) {
            deviceExtensions_.push_back("VK_KHR_portability_subset");
            break;
        }
    }

    return true;
}

bool VulkanRenderDevice::create_logical_device() {
    QueueFamilyIndices indices = find_queue_families(gvPhysicalDevice_);

    std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;
    std::set<uint32_t> uniqueQueueFamilies = {
        indices.graphicsFamily.value(),
        indices.presentFamily.value()
    };

    float queuePriority = 1.0F;
    for (uint32_t queueFamily : uniqueQueueFamilies) {
        vk::DeviceQueueCreateInfo queueCreateInfo{
            {},
            queueFamily,
            1,
            &queuePriority
        };
        queueCreateInfos.push_back(queueCreateInfo);
    }

    vk::PhysicalDeviceFeatures deviceFeatures{};
    deviceFeatures.setSamplerAnisotropy(vk::True);
    deviceFeatures.setSampleRateShading(vk::True);

    vk::PhysicalDeviceVulkan12Features vulkan12FeaturesEnable{};
    vulkan12FeaturesEnable.descriptorIndexing =
        vulkan12Features_.descriptorIndexing;

    vulkan12FeaturesEnable.runtimeDescriptorArray =
        supportedIndexingFeatures_.runtimeDescriptorArray;
    vulkan12FeaturesEnable.descriptorBindingPartiallyBound =
        supportedIndexingFeatures_.descriptorBindingPartiallyBound;
    vulkan12FeaturesEnable.descriptorBindingVariableDescriptorCount =
        supportedIndexingFeatures_.descriptorBindingVariableDescriptorCount;
    vulkan12FeaturesEnable.shaderSampledImageArrayNonUniformIndexing =
        supportedIndexingFeatures_.shaderSampledImageArrayNonUniformIndexing;

    vk::DeviceCreateInfo createInfo{
        {},
        static_cast<uint32_t>(queueCreateInfos.size()),
        queueCreateInfos.data(),
        static_cast<uint32_t>(validationLayers.size()),
        validationLayers.data(),
        static_cast<uint32_t>(deviceExtensions_.size()),
        deviceExtensions_.data(),
        &deviceFeatures,
        &vulkan12FeaturesEnable
    };

    gvDevice_ = gvPhysicalDevice_.createDevice(createInfo);
    gvGraphicsQueue_ = gvDevice_.getQueue(indices.graphicsFamily.value(), 0);
    gvPresentQueue_ = gvDevice_.getQueue(indices.presentFamily.value(), 0);

    VULKAN_HPP_DEFAULT_DISPATCHER.init(gvDevice_);
    return true;
}

bool VulkanRenderDevice::create_surface() {
    VkSurfaceKHR khr = nullptr;
    if (!SDL_Vulkan_CreateSurface(window, gvInstance_, nullptr, &khr)) {
        throw std::runtime_error("failed to create window surface!");
    }
    gvSurface_ = khr;
    return true;
}

bool VulkanRenderDevice::create_swap_chain() {
    SwapChainSupportDetails swapChainSupport{
        .capabilities = gvPhysicalDevice_.getSurfaceCapabilitiesKHR(gvSurface_),
        .formats = gvPhysicalDevice_.getSurfaceFormatsKHR(gvSurface_),
        .presentModes = gvPhysicalDevice_.getSurfacePresentModesKHR(gvSurface_)
    };
    uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
    if (swapChainSupport.capabilities.maxImageCount > 0 &&
        imageCount > swapChainSupport.capabilities.maxImageCount) {
        imageCount = swapChainSupport.capabilities.maxImageCount;
    }

    vk::SurfaceFormatKHR surfaceFormat =
        choose_surface_format(swapChainSupport.formats);
    vk::PresentModeKHR presentModes{VK_PRESENT_MODE_FIFO_KHR};
    vk::Extent2D extent = create_extent();

    QueueFamilyIndices indices = find_queue_families(gvPhysicalDevice_);
    std::array queueFamilyIndices{
        indices.graphicsFamily.value(),
        indices.presentFamily.value()
    };

    vk::SharingMode imageSharingMode{};
    uint32_t queueFamilyIndexCount = 0;
    if (indices.graphicsFamily != indices.presentFamily) {
        imageSharingMode = vk::SharingMode::eConcurrent;
        queueFamilyIndexCount = 2;
    } else {
        imageSharingMode = vk::SharingMode::eExclusive;
    }

    vk::SwapchainCreateInfoKHR createInfo{
        {},
        gvSurface_,
        imageCount,
        surfaceFormat.format,
        surfaceFormat.colorSpace,
        extent,
        1,
        vk::ImageUsageFlagBits::eColorAttachment,
        imageSharingMode,
        queueFamilyIndexCount,
        queueFamilyIndices.data(),
        swapChainSupport.capabilities.currentTransform,
        vk::CompositeAlphaFlagBitsKHR::eOpaque,
        presentModes,
        vk::True,
        VK_NULL_HANDLE
    };

    gvSwapchainKHR_ = gvDevice_.createSwapchainKHR(createInfo);

    gvSwapChainImageFormat_ = surfaceFormat.format;
    gvSwapChainExtent_ = extent;
    swapChainImages_ = gvDevice_.getSwapchainImagesKHR(gvSwapchainKHR_);
    return true;
}

bool VulkanRenderDevice::recreate_swap_chain() {
    gvDevice_.waitIdle();

    int width = 0;
    int height = 0;
    while (width == 0 || height == 0) {
        SDL_Event event;
        SDL_GetWindowSize(window, &width, &height);
    }

    const auto imageCount = static_cast<uint32_t>(swapChainImages_.size());
    imageInFlight_.assign(imageCount, VK_NULL_HANDLE);

    gvDevice_.waitIdle();

    cleanup_swap_chain();

    create_swap_chain();
    create_image_views();
    create_color_resources();
    create_depth_resources();
    create_framebuffers();

    for (auto& sem : renderFinishedSemaphores_) {
        sem = gvDevice_.createSemaphore({});
    }

    imageInFlight_.clear();
    imageInFlight_.resize(imageCount, VK_NULL_HANDLE);

    return true;
}

bool VulkanRenderDevice::cleanup_swap_chain() {
    gvDevice_.destroyImageView(colorImageView_);
    gvDevice_.destroyImage(colorImage_);
    gvDevice_.freeMemory(colorImageMemory_);

    gvDevice_.destroyImageView(depthImageView_);
    gvDevice_.destroyImage(depthImage_);
    gvDevice_.freeMemory(depthImageMemory_);

    for (auto framebuffer : swapChainFramebuffers_) {
        gvDevice_.destroyFramebuffer(framebuffer);
    }

    for (auto imageView : swapChainImageViews_) {
        gvDevice_.destroyImageView(imageView);
    }
    for (auto sem : renderFinishedSemaphores_) {
        gvDevice_.destroySemaphore(sem);
    }

    gvDevice_.destroySwapchainKHR(gvSwapchainKHR_);
    return true;
}

bool VulkanRenderDevice::create_image_views() {
    swapChainImageViews_.resize(swapChainImages_.size());

    for (size_t i = 0; i < swapChainImages_.size(); i++) {
        swapChainImageViews_[i] = create_image_view(
            swapChainImages_[i],
            gvSwapChainImageFormat_,
            vk::ImageAspectFlagBits::eColor,
            1
        );
    }
    return true;
}

vk::ImageView VulkanRenderDevice::create_image_view(
    const vk::Image image,
    const vk::Format format,
    const vk::ImageAspectFlags aspectFlags,
    const uint32_t mipLevels
) {
    vk::ImageViewCreateInfo viewInfo{
        {},
        image,
        vk::ImageViewType::e2D,
        format,
        {},
        vk::ImageSubresourceRange{aspectFlags, 0, mipLevels, 0, 1}
    };

    return gvDevice_.createImageView(viewInfo);
}

bool VulkanRenderDevice::create_descriptor_set_layout() {
    // binding 0: camera UBO, binding 1: model storage buffer, binding 2:
    // sampler, binding 3: sampled image array
    vk::DescriptorSetLayoutBinding camBinding{
        0,
        vk::DescriptorType::eUniformBuffer,
        1,
        vk::ShaderStageFlagBits::eVertex
    };
    vk::DescriptorSetLayoutBinding modelBinding{
        1,
        vk::DescriptorType::eStorageBuffer,
        1,
        vk::ShaderStageFlagBits::eVertex
    };
    vk::DescriptorSetLayoutBinding sampBinding{
        2,
        vk::DescriptorType::eSampler,
        1,
        vk::ShaderStageFlagBits::eFragment,
        &gvTextureSampler_
    };
    vk::DescriptorSetLayoutBinding imgBinding{
        3,
        vk::DescriptorType::eSampledImage,
        textureLimit_,
        vk::ShaderStageFlagBits::eFragment
    };
    std::array bindings{camBinding, modelBinding, sampBinding, imgBinding};

    vk::DescriptorBindingFlags textureBindingFlags{};
    if (supportedIndexingFeatures_.descriptorBindingPartiallyBound) {
        textureBindingFlags |= vk::DescriptorBindingFlagBits::ePartiallyBound;
    }
    if (supportedIndexingFeatures_.descriptorBindingVariableDescriptorCount) {
        textureBindingFlags |=
            vk::DescriptorBindingFlagBits::eVariableDescriptorCount;
    }

    std::array bindingFlags{
        vk::DescriptorBindingFlags{},
        vk::DescriptorBindingFlags{},
        vk::DescriptorBindingFlags{},
        textureBindingFlags
    };
    vk::DescriptorSetLayoutBindingFlagsCreateInfo flagsInfo{
        bindings.size(),
        bindingFlags.data()
    };
    vk::DescriptorSetLayoutCreateInfo layoutInfo{
        // vk::DescriptorSetLayoutCreateFlagBits::eUpdateAfterBindPool,
        {},
        bindings.size(),
        bindings.data(),
        &flagsInfo
    };
    gvDescriptorSetLayout_ = gvDevice_.createDescriptorSetLayout(layoutInfo);
    return true;
}

bool VulkanRenderDevice::create_render_pass() {
    vk::AttachmentReference colorAttachRef{
        0,
        vk::ImageLayout::eColorAttachmentOptimal
    };
    vk::AttachmentReference colorAttachResolveRef{
        2,
        vk::ImageLayout::eColorAttachmentOptimal
    };
    vk::AttachmentReference depthAttachRef{
        1,
        vk::ImageLayout::eDepthStencilAttachmentOptimal
    };

    vk::SubpassDescription subpass{
        {},
        vk::PipelineBindPoint::eGraphics,
        {},
        {},
        1,
        &colorAttachRef,
        &colorAttachResolveRef,
        &depthAttachRef
    };

    vk::SubpassDependency dependency{
        vk::SubpassExternal,
        0,
        vk::PipelineStageFlagBits::eColorAttachmentOutput |
            vk::PipelineStageFlagBits::eLateFragmentTests,
        vk::PipelineStageFlagBits::eColorAttachmentOutput |
            vk::PipelineStageFlagBits::eEarlyFragmentTests,
        vk::AccessFlagBits::eColorAttachmentWrite |
            vk::AccessFlagBits::eDepthStencilAttachmentWrite,
        vk::AccessFlagBits::eColorAttachmentWrite |
            vk::AccessFlagBits::eDepthStencilAttachmentWrite
    };

    std::array attachments = {
        vk::AttachmentDescription{
            {},
            gvSwapChainImageFormat_,
            msaaSamples_,
            vk::AttachmentLoadOp::eClear,
            vk::AttachmentStoreOp::eStore,
            vk::AttachmentLoadOp::eDontCare,
            vk::AttachmentStoreOp::eDontCare,
            vk::ImageLayout::eUndefined,
            vk::ImageLayout::eColorAttachmentOptimal
        },
        vk::AttachmentDescription{
            {},
            find_depth_format(),
            msaaSamples_,
            vk::AttachmentLoadOp::eClear,
            vk::AttachmentStoreOp::eDontCare,
            vk::AttachmentLoadOp::eDontCare,
            vk::AttachmentStoreOp::eDontCare,
            vk::ImageLayout::eUndefined,
            vk::ImageLayout::eDepthStencilAttachmentOptimal
        },
        vk::AttachmentDescription{
            {},
            gvSwapChainImageFormat_,
            vk::SampleCountFlagBits::e1,
            vk::AttachmentLoadOp::eDontCare,
            vk::AttachmentStoreOp::eStore,
            vk::AttachmentLoadOp::eDontCare,
            vk::AttachmentStoreOp::eDontCare,
            vk::ImageLayout::eUndefined,
            vk::ImageLayout::ePresentSrcKHR
        },

    };
    vk::RenderPassCreateInfo renderPassInfo{
        {},
        attachments.size(),
        attachments.data(),
        1,
        &subpass,
        1,
        &dependency
    };
    gvRenderPass_ = gvDevice_.createRenderPass(renderPassInfo);

    return true;
}

bool VulkanRenderDevice::create_graphics_pipeline(std::string assetPath) {
    auto vertShaderCode = read_file(assetPath + kVertexShaderPath);
    auto fragShaderCode = read_file(assetPath + kFragmentShaderPath);
    vk::ShaderModule vertShaderModule = create_shader_module(vertShaderCode);
    vk::ShaderModule fragShaderModule = create_shader_module(fragShaderCode);

    vk::PipelineShaderStageCreateInfo vertShaderStageInfo{
        {},
        vk::ShaderStageFlagBits::eVertex,
        vertShaderModule,
        "main"
    };
    vk::PipelineShaderStageCreateInfo fragShaderStageInfo{
        {},
        vk::ShaderStageFlagBits::eFragment,
        fragShaderModule,
        "main"
    };
    std::array shaderStages = {vertShaderStageInfo, fragShaderStageInfo};

    auto bindingDescription = getBindingDescription();
    auto attributeDescriptions = getAttributeDescriptions();

    vk::PipelineVertexInputStateCreateInfo vertexInputInfo{
        {},
        1,
        &bindingDescription,
        attributeDescriptions.size(),
        attributeDescriptions.data()
    };

    vk::PipelineInputAssemblyStateCreateInfo inputAssembly{
        {},
        vk::PrimitiveTopology::eTriangleList,
        vk::False
    };

    vk::PipelineViewportStateCreateInfo viewportState{{}, 1, {}, 1};

    vk::PipelineRasterizationStateCreateInfo rasterizer{
        {},
        vk::False,
        vk::False,
        vk::PolygonMode::eFill,
        vk::CullModeFlagBits::eBack,
        vk::FrontFace::eCounterClockwise,
        vk::False,
        0.0F,
        0.0F,
        0.0F,
        1.0F
    };

    vk::PipelineMultisampleStateCreateInfo multisampling{
        {},
        msaaSamples_,
        vk::True,
        kSampleRateShadingMinFraction,
        nullptr,
        vk::False,
        vk::False
    };

    vk::PipelineColorBlendAttachmentState colorBlendAttachment{
        vk::False,
        vk::BlendFactor::eOne,
        vk::BlendFactor::eZero,
        vk::BlendOp::eAdd,
        vk::BlendFactor::eOne,
        vk::BlendFactor::eZero,
        vk::BlendOp::eAdd,
        vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
            vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA
    };

    vk::PipelineColorBlendStateCreateInfo colorBlending{
        {},
        vk::False,
        vk::LogicOp::eCopy,
        1,
        &colorBlendAttachment
    };

    std::vector<vk::DynamicState> dynamicStates = {
        vk::DynamicState::eViewport,
        vk::DynamicState::eScissor
    };

    vk::PipelineDynamicStateCreateInfo dynamicState{
        {},
        static_cast<uint32_t>(dynamicStates.size()),
        dynamicStates.data()
    };

    // Push constants: texture index + model index
    struct PC {
        uint32_t texIndex;
        uint32_t modelIndex;
    };
    vk::PushConstantRange pcRange{
        vk::ShaderStageFlagBits::eFragment | vk::ShaderStageFlagBits::eVertex,
        0,
        sizeof(PC)
    };

    vk::PipelineLayoutCreateInfo
        pipelineLayoutInfo{{}, 1, &gvDescriptorSetLayout_, 1, &pcRange};

    gvPipelineLayout_ = gvDevice_.createPipelineLayout(pipelineLayoutInfo);

    vk::PipelineDepthStencilStateCreateInfo depthStencil{
        {},
        vk::True,
        vk::True,
        vk::CompareOp::eLess,
        vk::False,
        vk::False
    };

    vk::GraphicsPipelineCreateInfo pipelineInfo{
        {},
        2,
        shaderStages.data(),
        &vertexInputInfo,
        &inputAssembly,
        {},
        &viewportState,
        &rasterizer,
        &multisampling,
        &depthStencil,
        &colorBlending,
        &dynamicState,
        gvPipelineLayout_,
        gvRenderPass_,
        0,
        VK_NULL_HANDLE,
        -1
    };

    gvPipeline_ =
        gvDevice_.createGraphicsPipeline(VK_NULL_HANDLE, pipelineInfo).value;
    gvDevice_.destroyShaderModule(fragShaderModule);
    gvDevice_.destroyShaderModule(vertShaderModule);
    return true;
}

bool VulkanRenderDevice::create_color_resources() {
    vk::Format colorFormat = gvSwapChainImageFormat_;

    create_image(
        gvSwapChainExtent_.width,
        gvSwapChainExtent_.height,
        1,
        msaaSamples_,
        colorFormat,
        vk::ImageTiling::eOptimal,
        vk::ImageUsageFlagBits::eTransientAttachment |
            vk::ImageUsageFlagBits::eColorAttachment,
        vk::MemoryPropertyFlagBits::eDeviceLocal,
        colorImage_,
        colorImageMemory_
    );
    colorImageView_ = create_image_view(
        colorImage_,
        colorFormat,
        vk::ImageAspectFlagBits::eColor,
        1
    );
    return true;
}

bool VulkanRenderDevice::create_depth_resources() {
    vk::Format depthFormat = find_depth_format();

    create_image(
        gvSwapChainExtent_.width,
        gvSwapChainExtent_.height,
        1,
        msaaSamples_,
        depthFormat,
        vk::ImageTiling::eOptimal,
        vk::ImageUsageFlagBits::eDepthStencilAttachment,
        vk::MemoryPropertyFlagBits::eDeviceLocal,
        depthImage_,
        depthImageMemory_
    );
    depthImageView_ = create_image_view(
        depthImage_,
        depthFormat,
        vk::ImageAspectFlagBits::eDepth,
        1
    );
    return true;
}

bool VulkanRenderDevice::create_framebuffers() {
    swapChainFramebuffers_.resize(swapChainImageViews_.size());

    for (size_t i = 0; i < swapChainImageViews_.size(); i++) {
        std::array attachments = {
            colorImageView_,
            depthImageView_,
            swapChainImageViews_[i]
        };

        vk::FramebufferCreateInfo framebufferInfo{
            {},
            gvRenderPass_,
            static_cast<uint32_t>(attachments.size()),
            attachments.data(),
            gvSwapChainExtent_.width,
            gvSwapChainExtent_.height,
            1
        };
        swapChainFramebuffers_[i] =
            gvDevice_.createFramebuffer(framebufferInfo);
    }
    return true;
}

bool VulkanRenderDevice::create_command_pool() {
    QueueFamilyIndices queueFamilyIndices =
        find_queue_families(gvPhysicalDevice_);

    vk::CommandPoolCreateInfo poolInfo{
        vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
        queueFamilyIndices.graphicsFamily.value()
    };

    gvCommandPool_ = gvDevice_.createCommandPool(poolInfo);

    return true;
}

bool VulkanRenderDevice::create_texture_sampler() {
    vk::PhysicalDeviceProperties properties = gvPhysicalDevice_.getProperties();

    gvTextureSampler_ = gvDevice_.createSampler(
        {{},
         vk::Filter::eLinear,
         vk::Filter::eLinear,
         vk::SamplerMipmapMode::eLinear,
         vk::SamplerAddressMode::eRepeat,
         vk::SamplerAddressMode::eRepeat,
         vk::SamplerAddressMode::eRepeat,
         0.0F,
         vk::True,
         properties.limits.maxSamplerAnisotropy,
         vk::False,
         vk::CompareOp::eAlways,
         0.0F,
         VK_LOD_CLAMP_NONE,
         vk::BorderColor::eIntOpaqueBlack}
    );
    return true;
}

uint32_t VulkanRenderDevice::load_texture(const std::string& path) {
    auto stringHash = std::hash<std::string>{}(path);
    if (loadedTextures_.contains(stringHash)) {
        return loadedTextures_[stringHash];
    }
    loadedTextures_[stringHash] = gvTextures_.size();

    int texWidth = 0;
    int texHeight = 0;
    int texChannels = 0;
    stbi_uc* pixels = stbi_load(
        path.c_str(),
        &texWidth,
        &texHeight,
        &texChannels,
        STBI_rgb_alpha
    );
    if (!pixels) {
        throw std::runtime_error("failed to load texture image!");
    }
    auto imageSize = static_cast<vk::DeviceSize>(
        static_cast<uint64_t>(texWidth) * static_cast<uint64_t>(texHeight) * 4U
    );

    uint32_t mipLevels =
        static_cast<uint32_t>(
            std::floor(std::log2(std::max(texWidth, texHeight)))
        ) +
        1;

    vk::Image textureImage;
    vk::DeviceMemory textureImageMemory;
    vk::Buffer stagingBuffer;
    vk::DeviceMemory stagingBufferMemory;
    create_buffer(
        imageSize,
        vk::BufferUsageFlagBits::eTransferSrc,
        vk::MemoryPropertyFlagBits::eHostVisible |
            vk::MemoryPropertyFlagBits::eHostCoherent,
        stagingBuffer,
        stagingBufferMemory
    );
    void* data = gvDevice_.mapMemory(stagingBufferMemory, 0, imageSize);
    memcpy(data, pixels, (size_t)imageSize);
    gvDevice_.unmapMemory(stagingBufferMemory);

    stbi_image_free(pixels);

    create_image(
        texWidth,
        texHeight,
        mipLevels,
        vk::SampleCountFlagBits::e1,
        vk::Format::eR8G8B8A8Srgb,
        vk::ImageTiling::eOptimal,
        vk::ImageUsageFlagBits::eTransferSrc |
            vk::ImageUsageFlagBits::eTransferDst |
            vk::ImageUsageFlagBits::eSampled,
        vk::MemoryPropertyFlagBits::eDeviceLocal,
        textureImage,
        textureImageMemory
    );

    transition_image_layout(
        textureImage,
        vk::Format::eR8G8B8A8Srgb,
        vk::ImageLayout::eUndefined,
        vk::ImageLayout::eTransferDstOptimal,
        mipLevels
    );

    copy_buffer_to_image(
        stagingBuffer,
        textureImage,
        static_cast<uint32_t>(texWidth),
        static_cast<uint32_t>(texHeight)
    );

    generate_mipmaps(
        textureImage,
        vk::Format::eR8G8B8A8Srgb,
        {texWidth, texHeight},
        mipLevels
    );

    gvDevice_.destroyBuffer(stagingBuffer);
    gvDevice_.freeMemory(stagingBufferMemory);

    vk::ImageView textureImageView = create_image_view(
        textureImage,
        vk::Format::eR8G8B8A8Srgb,
        vk::ImageAspectFlagBits::eColor,
        mipLevels
    );

    gvTextures_.push_back(
        GVTexture{
            .mipLevels = mipLevels,
            .textureImage = textureImage,
            .textureMemory = textureImageMemory,
            .textureImageView = textureImageView,
        }
    );

    update_descriptor_sets();
    return gvTextures_.size() - 1;
}

void VulkanRenderDevice::create_image(
    const uint32_t width,
    const uint32_t height,
    const uint32_t mipLevels,
    const vk::SampleCountFlagBits numSamples,
    const vk::Format format,
    const vk::ImageTiling tiling,
    const vk::ImageUsageFlags usage,
    const vk::MemoryPropertyFlags properties,
    vk::Image& image,
    vk::DeviceMemory& imageMemory
) {
    vk::ImageCreateInfo imageInfo{
        {},
        vk::ImageType::e2D,
        format,
        vk::Extent3D{width, height, 1},
        mipLevels,
        1,
        numSamples,
        tiling,
        usage,
        vk::SharingMode::eExclusive,
    };
    image = gvDevice_.createImage(imageInfo);

    vk::MemoryRequirements memRequirements =
        gvDevice_.getImageMemoryRequirements(image);

    vk::MemoryAllocateInfo allocInfo{
        memRequirements.size,
        find_memory_type(memRequirements.memoryTypeBits, properties)
    };

    imageMemory = gvDevice_.allocateMemory(allocInfo);

    gvDevice_.bindImageMemory(image, imageMemory, 0);
}

void VulkanRenderDevice::create_vertex_buffer() {
    create_buffer(
        kbufferDefaultSize,
        vk::BufferUsageFlagBits::eTransferDst |
            vk::BufferUsageFlagBits::eVertexBuffer,
        vk::MemoryPropertyFlagBits::eDeviceLocal,
        vertexBuffer_,
        vertexBufferMemory_
    );
}

void VulkanRenderDevice::create_index_buffer() {
    create_buffer(
        kbufferDefaultSize,  // TODO replace with sane size
        vk::BufferUsageFlagBits::eTransferDst |
            vk::BufferUsageFlagBits::eIndexBuffer,
        vk::MemoryPropertyFlagBits::eDeviceLocal,
        indexBuffer_,
        indexBufferMemory_
    );
}

void VulkanRenderDevice::create_uniform_buffers() {
    vk::DeviceSize bufferSize = sizeof(CameraUBO);

    uniformBuffers_.resize(kMAX_FRAMES_IN_FLIGHT);
    uniformBuffersMemory_.resize(kMAX_FRAMES_IN_FLIGHT);
    uniformBuffersMapped_.resize(kMAX_FRAMES_IN_FLIGHT);

    for (size_t i = 0; i < kMAX_FRAMES_IN_FLIGHT; i++) {
        create_buffer(
            bufferSize,
            vk::BufferUsageFlagBits::eUniformBuffer,
            vk::MemoryPropertyFlagBits::eHostVisible |
                vk::MemoryPropertyFlagBits::eHostCoherent,
            uniformBuffers_[i],
            uniformBuffersMemory_[i]
        );

        uniformBuffersMapped_[i] =
            gvDevice_.mapMemory(uniformBuffersMemory_[i], 0, bufferSize);
    }
}

void VulkanRenderDevice::create_model_buffers(const uint32_t minCapacity) {
    modelBuffers_.resize(kMAX_FRAMES_IN_FLIGHT);
    modelBuffersMemory_.resize(kMAX_FRAMES_IN_FLIGHT);
    modelBuffersMapped_.resize(kMAX_FRAMES_IN_FLIGHT);
    modelBufferCapacity = std::max(minCapacity, kInitialModelCapacity);
    vk::DeviceSize bufferSize =
        static_cast<vk::DeviceSize>(modelBufferCapacity) * sizeof(glm::mat4);
    for (size_t i = 0; i < kMAX_FRAMES_IN_FLIGHT; ++i) {
        create_buffer(
            bufferSize,
            vk::BufferUsageFlagBits::eStorageBuffer,
            vk::MemoryPropertyFlagBits::eHostVisible |
                vk::MemoryPropertyFlagBits::eHostCoherent,
            modelBuffers_[i],
            modelBuffersMemory_[i]
        );
        modelBuffersMapped_[i] =
            gvDevice_.mapMemory(modelBuffersMemory_[i], 0, bufferSize);
    }
}

void VulkanRenderDevice::destroy_model_buffers() {
    for (size_t i = 0; i < modelBuffers_.size(); ++i) {
        if (modelBuffers_[i]) {
            gvDevice_.unmapMemory(modelBuffersMemory_[i]);
            gvDevice_.destroyBuffer(modelBuffers_[i]);
            gvDevice_.freeMemory(modelBuffersMemory_[i]);
        }
    }
    modelBuffers_.clear();
    modelBuffersMemory_.clear();
    modelBuffersMapped_.clear();
    modelBufferCapacity = 0;
}

void VulkanRenderDevice::ensure_model_capacity(
    const uint32_t requiredModelCount
) {
    if (requiredModelCount <= modelBufferCapacity) return;
    uint32_t newCap = modelBufferCapacity;
    while (newCap < requiredModelCount) newCap *= 2;
    destroy_model_buffers();
    create_model_buffers(newCap);
    update_descriptor_sets();
}

void VulkanRenderDevice::update_camera_buffer(const uint32_t currentImage) {
    memcpy(uniformBuffersMapped_[currentImage], &cameraUbo_, sizeof(CameraUBO));
}

void VulkanRenderDevice::update_model_buffer(
    const uint32_t currentImage,
    const std::vector<glm::mat4>& models
) {
    auto bytes = models.size() * sizeof(glm::mat4);
    memcpy(modelBuffersMapped_[currentImage], models.data(), bytes);
}

bool VulkanRenderDevice::create_descriptor_pool() {
    std::array poolSizes{
        vk::DescriptorPoolSize{
            vk::DescriptorType::eUniformBuffer,
            kMAX_FRAMES_IN_FLIGHT
        },
        vk::DescriptorPoolSize{
            vk::DescriptorType::eStorageBuffer,
            kMAX_FRAMES_IN_FLIGHT
        },
        vk::DescriptorPoolSize{
            vk::DescriptorType::eSampler,
            kMAX_FRAMES_IN_FLIGHT
        },
        vk::DescriptorPoolSize{
            vk::DescriptorType::eSampledImage,
            textureLimit_ * kMAX_FRAMES_IN_FLIGHT
        }
    };

    vk::DescriptorPoolCreateInfo poolInfo{
        // vk::DescriptorPoolCreateFlagBits::eUpdateAfterBind,
        {},
        kMAX_FRAMES_IN_FLIGHT,
        poolSizes.size(),
        poolSizes.data()
    };

    gvDescriptorPool_ = gvDevice_.createDescriptorPool(poolInfo);
    return true;
}

bool VulkanRenderDevice::create_descriptor_sets() {
    std::vector<vk::DescriptorSetLayout> layouts(
        kMAX_FRAMES_IN_FLIGHT,
        gvDescriptorSetLayout_
    );

    std::vector<uint32_t> variableCounts(kMAX_FRAMES_IN_FLIGHT, textureLimit_);

    vk::DescriptorSetVariableDescriptorCountAllocateInfo varInfo{
        kMAX_FRAMES_IN_FLIGHT,
        variableCounts.data()
    };

    vk::DescriptorSetAllocateInfo allocInfo{
        gvDescriptorPool_,
        static_cast<uint32_t>(layouts.size()),
        layouts.data(),
        &varInfo

    };

    descriptorSets_.resize(kMAX_FRAMES_IN_FLIGHT);
    descriptorSets_ = gvDevice_.allocateDescriptorSets(allocInfo);

    update_descriptor_sets();
    return true;
}

void VulkanRenderDevice::update_descriptor_sets() {
    if (descriptorSets_.empty()) return;
    std::vector<vk::DescriptorImageInfo> imageInfos(gvTextures_.size());
    for (uint32_t i = 0; i < gvTextures_.size(); ++i) {
        imageInfos[i] = vk::DescriptorImageInfo{
            VK_NULL_HANDLE,
            gvTextures_[i].textureImageView,
            vk::ImageLayout::eShaderReadOnlyOptimal
        };
    }
    std::vector<vk::DescriptorBufferInfo> camInfos(kMAX_FRAMES_IN_FLIGHT);
    std::vector<vk::DescriptorBufferInfo> modelInfos(kMAX_FRAMES_IN_FLIGHT);
    for (size_t i = 0; i < kMAX_FRAMES_IN_FLIGHT; ++i) {
        camInfos[i] =
            vk::DescriptorBufferInfo{uniformBuffers_[i], 0, sizeof(CameraUBO)};
        if (!modelBuffers_.empty()) {
            modelInfos[i] = vk::DescriptorBufferInfo{
                modelBuffers_[i],
                0,
                static_cast<vk::DeviceSize>(modelBufferCapacity) *
                    sizeof(glm::mat4)
            };
        }
    }
    std::vector<vk::WriteDescriptorSet> writes;
    for (size_t i = 0; i < kMAX_FRAMES_IN_FLIGHT; ++i) {
        writes.emplace_back(
            descriptorSets_[i],
            0,
            0,
            1,
            vk::DescriptorType::eUniformBuffer,
            nullptr,
            &camInfos[i]
        );
        if (!modelBuffers_.empty()) {
            writes.emplace_back(
                descriptorSets_[i],
                1,
                0,
                1,
                vk::DescriptorType::eStorageBuffer,
                nullptr,
                &modelInfos[i]
            );
        }
        if (!gvTextures_.empty()) {
            writes.emplace_back(
                descriptorSets_[i],
                3,
                0,
                static_cast<uint32_t>(gvTextures_.size()),
                vk::DescriptorType::eSampledImage,
                imageInfos.data()
            );
        }
    }
    if (!writes.empty()) {
        gvDevice_.updateDescriptorSets(
            static_cast<uint32_t>(writes.size()),
            writes.data(),
            0,
            nullptr
        );
    }
}

void VulkanRenderDevice::create_buffer(
    const vk::DeviceSize size,
    const vk::BufferUsageFlags usage,
    const vk::MemoryPropertyFlags properties,
    vk::Buffer& buffer,
    vk::DeviceMemory& bufferMemory
) {
    vk::BufferCreateInfo bufferInfo{
        {},
        size,
        usage,
        vk::SharingMode::eExclusive
    };
    buffer = gvDevice_.createBuffer(bufferInfo);

    vk::MemoryRequirements memRequirements =
        gvDevice_.getBufferMemoryRequirements(buffer);

    vk::MemoryAllocateInfo allocInfo{
        memRequirements.size,
        find_memory_type(memRequirements.memoryTypeBits, properties)
    };

    bufferMemory = gvDevice_.allocateMemory(allocInfo);
    gvDevice_.bindBufferMemory(buffer, bufferMemory, 0);
}

void VulkanRenderDevice::copy_buffer(
    const vk::Buffer srcBuffer,
    const vk::Buffer dstBuffer,
    const vk::DeviceSize size,
    const vk::DeviceSize dstOffset
) {
    vk::CommandBuffer commandBuffer = begin_single_time_commands();
    vk::BufferCopy copyRegion{0, dstOffset, size};
    commandBuffer.copyBuffer(srcBuffer, dstBuffer, 1, &copyRegion);
    end_single_time_commands(commandBuffer);
}

uint32_t VulkanRenderDevice::find_memory_type(
    const uint32_t typeFilter,
    const vk::MemoryPropertyFlags properties
) {
    vk::PhysicalDeviceMemoryProperties memProperties =
        gvPhysicalDevice_.getMemoryProperties();

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if (((typeFilter & (1 << i)) != 0U) &&
            (memProperties.memoryTypes.at(i).propertyFlags & properties) ==
                properties) {
            return i;
        }
    }

    throw std::runtime_error("failed to find suitable memory type!");
}

vk::CommandBuffer VulkanRenderDevice::begin_single_time_commands() {
    vk::CommandBufferAllocateInfo allocInfo{
        gvCommandPool_,
        vk::CommandBufferLevel::ePrimary,
        1
    };

    vk::CommandBuffer commandBuffer =
        gvDevice_.allocateCommandBuffers(allocInfo)[0];

    commandBuffer.begin({vk::CommandBufferUsageFlagBits::eOneTimeSubmit});

    return commandBuffer;
}

void VulkanRenderDevice::end_single_time_commands(
    vk::CommandBuffer& commandBuffer
) {
    commandBuffer.end();

    vk::SubmitInfo submitInfo{};
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;
    std::vector<vk::SubmitInfo> submitInfos{submitInfo};

    gvGraphicsQueue_.submit(submitInfos, VK_NULL_HANDLE);

    gvGraphicsQueue_.waitIdle();
    gvDevice_.freeCommandBuffers(gvCommandPool_, commandBuffer);
}

void VulkanRenderDevice::transition_image_layout(
    const vk::Image image,
    const vk::Format /*format*/,
    const vk::ImageLayout oldLayout,
    const vk::ImageLayout newLayout,
    const uint32_t mipLevels
) {
    vk::CommandBuffer commandBuffer = begin_single_time_commands();

    vk::PipelineStageFlags sourceStage{};
    vk::PipelineStageFlags destinationStage{};

    vk::AccessFlags src{};
    vk::AccessFlags dst{};
    if (oldLayout == vk::ImageLayout::eUndefined &&
        newLayout == vk::ImageLayout::eTransferDstOptimal) {
        src = vk::AccessFlagBits::eNone;
        dst = vk::AccessFlagBits::eTransferWrite;
        sourceStage = vk::PipelineStageFlagBits::eTopOfPipe;
        destinationStage = vk::PipelineStageFlagBits::eTransfer;
    } else if (oldLayout == vk::ImageLayout::eTransferDstOptimal &&
               newLayout == vk::ImageLayout::eShaderReadOnlyOptimal) {
        src = vk::AccessFlagBits::eTransferWrite;
        dst = vk::AccessFlagBits::eShaderRead;
        sourceStage = vk::PipelineStageFlagBits::eTransfer;
        destinationStage = vk::PipelineStageFlagBits::eFragmentShader;
    } else if (oldLayout == vk::ImageLayout::eUndefined &&
               newLayout == vk::ImageLayout::eDepthStencilAttachmentOptimal) {
        src = vk::AccessFlagBits::eNone;
        dst = vk::AccessFlagBits::eDepthStencilAttachmentRead |
              vk::AccessFlagBits::eDepthStencilAttachmentWrite;
        sourceStage = vk::PipelineStageFlagBits::eTopOfPipe;
        destinationStage = destinationStage =
            vk::PipelineStageFlagBits::eEarlyFragmentTests;
    } else {
        throw std::invalid_argument("unsupported layout transition!");
    }

    vk::ImageMemoryBarrier barrier{
        src,
        dst,
        oldLayout,
        newLayout,
        vk::QueueFamilyIgnored,
        vk::QueueFamilyIgnored,
        image,
        {vk::ImageAspectFlagBits::eColor, 0, mipLevels, 0, 1}
    };

    // if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
    //     barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

    //     if (hasStencilComponent(format)) {
    //         barrier.subresourceRange.aspectMask |=
    //         VK_IMAGE_ASPECT_STENCIL_BIT;
    //     }
    // } else {
    //     barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    // }

    commandBuffer.pipelineBarrier(
        sourceStage,
        destinationStage,
        {},  // THIS MIGHT NOT
             // WORK!! TODO TODO
        0,
        nullptr,
        0,
        nullptr,
        1,
        &barrier
    );

    end_single_time_commands(commandBuffer);
}

void VulkanRenderDevice::copy_buffer_to_image(
    const vk::Buffer buffer,
    const vk::Image image,
    const uint32_t width,
    const uint32_t height
) {
    vk::CommandBuffer commandBuffer = begin_single_time_commands();

    vk::BufferImageCopy region{
        0,
        0,
        0,
        {vk::ImageAspectFlagBits::eColor, 0, 0, 1},
        {0, 0, 0},
        {width, height, 1}
    };

    commandBuffer.copyBufferToImage(
        buffer,
        image,
        vk::ImageLayout::eTransferDstOptimal,
        1,
        &region
    );

    end_single_time_commands(commandBuffer);
}

bool VulkanRenderDevice::create_command_buffers() {
    gvCommandBuffers_.resize(kMAX_FRAMES_IN_FLIGHT);

    vk::CommandBufferAllocateInfo allocInfo{
        gvCommandPool_,
        vk::CommandBufferLevel::ePrimary,
        static_cast<uint32_t>(gvCommandBuffers_.size())
    };

    gvCommandBuffers_ = gvDevice_.allocateCommandBuffers(allocInfo);
    return true;
}

void VulkanRenderDevice::record_command_buffer(
    const vk::CommandBuffer commandBuffer,
    const uint32_t imageIndex,
    ECSController& world
) {
    commandBuffer.begin({vk::CommandBufferUsageFlagBits::eSimultaneousUse});

    std::array clearValues{
        vk::ClearValue{{0.0F, 0.0F, 0.0F, 1.0F}},
        vk::ClearValue{vk::ClearDepthStencilValue{1.0F, 0}}
    };

    vk::RenderPassBeginInfo renderPassInfo{
        gvRenderPass_,
        swapChainFramebuffers_[imageIndex],
        {{0, 0}, gvSwapChainExtent_},
        clearValues.size(),
        clearValues.data()
    };

    commandBuffer.beginRenderPass(renderPassInfo, vk::SubpassContents::eInline);

    commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, gvPipeline_);

    vk::Viewport viewport{
        0.0F,
        0.0F,
        static_cast<float>(gvSwapChainExtent_.width),
        static_cast<float>(gvSwapChainExtent_.height),
        0.0F,
        1.0F
    };

    commandBuffer.setViewport(0, 1, &viewport);

    vk::Rect2D scissor{{0, 0}, gvSwapChainExtent_};

    commandBuffer.setScissor(0, 1, &scissor);

    glm::mat4 view{1.0F};
    glm::mat4 proj{1.0F};
    auto cameras = world.get_entities<garnish::Camera>();
    if (!cameras.empty()) {
        auto& cam = world.get_component<garnish::Camera>(cameras[0]);
        view = cam.view_matrix();
        constexpr float kFovDeg = 45.0F;
        constexpr float kNear = 0.1F;
        constexpr float kFar = 100.0F;
        proj = glm::perspective(
            glm::radians(kFovDeg),
            static_cast<float>(gvSwapChainExtent_.width) /
                static_cast<float>(gvSwapChainExtent_.height),
            kNear,
            kFar
        );
        proj[1][1] *= -1.0F;
    }
    cameraUbo_.view = view;
    cameraUbo_.proj = proj;

    auto entities = world.get_entities<Renderable, Transform>();
    std::vector<glm::mat4> modelMatrices;

    modelMatrices.reserve(entities.size());

    for (auto e : entities) {
        const auto& tf = world.get_component<Transform>(e);
        glm::mat4 model{1.0F};
        model = glm::translate(model, tf.position);
        model *= glm::mat4_cast(tf.rotation);
        modelMatrices.push_back(model);
    }

    ensure_model_capacity(static_cast<uint32_t>(modelMatrices.size()));
    update_model_buffer(currentFrame_, modelMatrices);

    // Bind descriptor sets once
    commandBuffer.bindDescriptorSets(
        vk::PipelineBindPoint::eGraphics,
        gvPipelineLayout_,
        0,
        1,
        &descriptorSets_[currentFrame_],
        0,
        nullptr
    );

    uint32_t modelIdx = 0;
    for (auto e : entities) {
        const auto& r = world.get_component<Renderable>(e);
        const auto& msh = gvMeshes_[r.meshHandle];
        vk::DeviceSize vByteOffset =
            static_cast<vk::DeviceSize>(msh.firstVertex) * sizeof(Vertex);
        commandBuffer.bindVertexBuffers(0, 1, &vertexBuffer_, &vByteOffset);
        commandBuffer.bindIndexBuffer(
            indexBuffer_,
            static_cast<vk::DeviceSize>(msh.firstIndex) * sizeof(uint32_t),
            vk::IndexType::eUint32
        );
        struct PC {
            uint32_t texIndex;
            uint32_t modelIndex;
        } pc{.texIndex = r.texHandle, .modelIndex = modelIdx};
        commandBuffer.pushConstants(
            gvPipelineLayout_,
            vk::ShaderStageFlagBits::eFragment |
                vk::ShaderStageFlagBits::eVertex,
            0,
            sizeof(PC),
            &pc
        );
        commandBuffer.drawIndexed(msh.indexCount, 1, 0, 0, 0);
        ++modelIdx;
    }

    commandBuffer.endRenderPass();
    commandBuffer.end();
}

bool VulkanRenderDevice::create_sync_objects() {
    imageAvailableSemaphores_.resize(kMAX_FRAMES_IN_FLIGHT);
    inFlightFences_.resize(kMAX_FRAMES_IN_FLIGHT);

    vk::FenceCreateInfo fenceInfo{vk::FenceCreateFlagBits::eSignaled};

    for (size_t i = 0; i < kMAX_FRAMES_IN_FLIGHT; i++) {
        imageAvailableSemaphores_[i] = gvDevice_.createSemaphore({});
        inFlightFences_[i] = gvDevice_.createFence(fenceInfo);
    }

    const auto imageCount = static_cast<uint32_t>(swapChainImages_.size());
    renderFinishedSemaphores_.resize(imageCount);
    imageInFlight_.assign(imageCount, VK_NULL_HANDLE);

    for (size_t i = 0; i < imageCount; i++) {
        renderFinishedSemaphores_[i] = gvDevice_.createSemaphore({});
    }

    return true;
}

bool VulkanRenderDevice::draw_frame(ECSController& world) {
    (void)gvDevice_
        .waitForFences(inFlightFences_[currentFrame_], vk::True, UINT64_MAX);

    uint32_t imageIndex = 0;
    auto aquireResult = gvDevice_.acquireNextImageKHR(
        gvSwapchainKHR_,
        UINT64_MAX,
        imageAvailableSemaphores_[currentFrame_],
        VK_NULL_HANDLE,
        &imageIndex
    );

    if (aquireResult == vk::Result::eErrorOutOfDateKHR) {
        recreate_swap_chain();
        return false;
    }
    if (aquireResult != vk::Result::eSuccess &&
        aquireResult != vk::Result::eSuboptimalKHR) {
        throw std::runtime_error("failed to acquire swap chain image!");
    }

    if (imageInFlight_[imageIndex] != VK_NULL_HANDLE) {
        (void)gvDevice_.waitForFences(
            1,
            &imageInFlight_[imageIndex],
            vk::True,
            UINT64_MAX
        );
    }
    imageInFlight_[imageIndex] = inFlightFences_[currentFrame_];

    // Only reset the fence if we are submitting work
    (void)gvDevice_.resetFences(1, &inFlightFences_[currentFrame_]);
    gvCommandBuffers_[currentFrame_].reset();

    record_command_buffer(gvCommandBuffers_[currentFrame_], imageIndex, world);
    update_camera_buffer(currentFrame_);
    const std::array<vk::Semaphore, 1> waitSemaphores{
        imageAvailableSemaphores_[currentFrame_]
    };
    const std::array<vk::PipelineStageFlags, 1> waitStages{
        vk::PipelineStageFlagBits::eColorAttachmentOutput
    };
    const std::array<vk::Semaphore, 1> signalSemaphores{
        renderFinishedSemaphores_[imageIndex]
    };

    vk::SubmitInfo submitInfo{
        static_cast<uint32_t>(waitSemaphores.size()),
        waitSemaphores.data(),
        waitStages.data(),
        1,
        &gvCommandBuffers_[currentFrame_],
        static_cast<uint32_t>(signalSemaphores.size()),
        signalSemaphores.data()
    };

    (
        void
    )gvGraphicsQueue_.submit(1, &submitInfo, inFlightFences_[currentFrame_]);
    const std::array<vk::SwapchainKHR, 1> swapChains{gvSwapchainKHR_};

    vk::PresentInfoKHR presentInfo{
        static_cast<uint32_t>(signalSemaphores.size()),
        signalSemaphores.data(),
        static_cast<uint32_t>(swapChains.size()),
        swapChains.data(),
        &imageIndex
    };

    auto presentResult = gvPresentQueue_.presentKHR(&presentInfo);

    if (presentResult == vk::Result::eErrorOutOfDateKHR ||
        presentResult == vk::Result::eSuboptimalKHR) {
        framebufferResized_ = false;
        recreate_swap_chain();
    } else if (presentResult != vk::Result::eSuccess) {
        throw std::runtime_error(
            "update_descriptor_setsfailed to present swap chain image!"
        );
    }

    currentFrame_ = (currentFrame_ + 1) % kMAX_FRAMES_IN_FLIGHT;
    return true;
}

void VulkanRenderDevice::generate_mipmaps(
    const vk::Image image,
    const vk::Format imageFormat,
    const TextureSize size,
    const uint32_t mipLevels
) {
    vk::FormatProperties formatProperties =
        gvPhysicalDevice_.getFormatProperties(imageFormat);
    if (!(formatProperties.optimalTilingFeatures &
          vk::FormatFeatureFlagBits::eSampledImageFilterLinear)) {
        throw std::runtime_error(
            "texture image format does not support linear blitting!"
        );
    }
    vk::CommandBuffer commandBuffer = begin_single_time_commands();
    vk::ImageMemoryBarrier barrier{};
    barrier.image = image;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;
    barrier.subresourceRange.levelCount = 1;

    int32_t mipWidth = size.width;
    int32_t mipHeight = size.height;
    for (uint32_t i = 1; i < mipLevels; ++i) {
        barrier.subresourceRange.baseMipLevel = i - 1;
        barrier.oldLayout = vk::ImageLayout::eTransferDstOptimal;
        barrier.newLayout = vk::ImageLayout::eTransferSrcOptimal;
        barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
        barrier.dstAccessMask = vk::AccessFlagBits::eTransferRead;
        commandBuffer.pipelineBarrier(
            vk::PipelineStageFlagBits::eTransfer,
            vk::PipelineStageFlagBits::eTransfer,
            {},
            0,
            nullptr,
            0,
            nullptr,
            1,
            &barrier
        );
        vk::ImageBlit blit{
            {vk::ImageAspectFlagBits::eColor, i - 1, 0, 1},
            {vk::Offset3D{0, 0, 0}, vk::Offset3D{mipWidth, mipHeight, 1}},
            {vk::ImageAspectFlagBits::eColor, i, 0, 1},
            {vk::Offset3D{0, 0, 0},
             vk::Offset3D{
                 (mipWidth > 1 ? mipWidth / 2 : 1),
                 (mipHeight > 1 ? mipHeight / 2 : 1),
                 1
             }}
        };
        commandBuffer.blitImage(
            image,
            vk::ImageLayout::eTransferSrcOptimal,
            image,
            vk::ImageLayout::eTransferDstOptimal,
            1,
            &blit,
            vk::Filter::eLinear
        );
        barrier.oldLayout = vk::ImageLayout::eTransferSrcOptimal;
        if (mipWidth > 1) {
            mipWidth /= 2;
        }
        if (mipHeight > 1) {
            mipHeight /= 2;
        }
    }
    barrier.subresourceRange.baseMipLevel = mipLevels - 1;
    barrier.oldLayout = vk::ImageLayout::eTransferDstOptimal;
    barrier.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
    barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
    barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;
    commandBuffer.pipelineBarrier(
        vk::PipelineStageFlagBits::eTransfer,
        vk::PipelineStageFlagBits::eFragmentShader,
        {},
        0,
        nullptr,
        0,
        nullptr,
        1,
        &barrier
    );
    end_single_time_commands(commandBuffer);
}

VulkanRenderDevice::QueueFamilyIndices VulkanRenderDevice::find_queue_families(
    const vk::PhysicalDevice& device
) {
    QueueFamilyIndices indices;
    auto queueFamilies = device.getQueueFamilyProperties();

    int i = 0;
    for (const auto& queueFamily : queueFamilies) {
        vk::Bool32 presentSupport = device.getSurfaceSupportKHR(i, gvSurface_);
        if (queueFamily.queueFlags & vk::QueueFlagBits::eGraphics) {
            indices.graphicsFamily = i;
        }
        if (presentSupport) {
            indices.presentFamily = i;
        }
        if (indices.isComplete()) {
            break;
        }

        i++;
    }
    return indices;
}

vk::Extent2D VulkanRenderDevice::create_extent() {
    int width = 0;
    int height = 0;
    SDL_GetWindowSize(window, &width, &height);
    return vk::Extent2D{
        static_cast<uint32_t>(width),
        static_cast<uint32_t>(height)
    };
}

vk::Format VulkanRenderDevice::find_supported_format(
    const std::vector<vk::Format>& candidates,
    const vk::ImageTiling tiling,
    const vk::FormatFeatureFlags features
) {
    for (const vk::Format& format : candidates) {
        vk::FormatProperties props =
            gvPhysicalDevice_.getFormatProperties(format);

        if (vk::ImageTiling::eLinear == tiling &&
            (props.linearTilingFeatures & features) == features) {
            return format;
        }

        if (tiling == vk::ImageTiling::eOptimal &&
            (props.optimalTilingFeatures & features) == features) {
            return format;
        }
    }

    throw std::runtime_error("failed to find supported format!");
}

vk::Format VulkanRenderDevice::find_depth_format() {
    return find_supported_format(
        {vk::Format::eD32Sfloat,
         vk::Format::eD32SfloatS8Uint,
         vk::Format::eD24UnormS8Uint},
        vk::ImageTiling::eOptimal,
        vk::FormatFeatureFlagBits::eDepthStencilAttachment
    );
}

vk::ShaderModule VulkanRenderDevice::create_shader_module(
    const std::vector<char>& code
) {
    return gvDevice_.createShaderModule(
        vk::ShaderModuleCreateInfo{
            {},
            code.size(),
            reinterpret_cast<const uint32_t*>(  // NOLINT
                code.data()
            )
        }
    );
}

bool VulkanRenderDevice::check_device_extension_support(
    const vk::PhysicalDevice& device
) {
    const auto availableExtensions =
        device.enumerateDeviceExtensionProperties();

    std::set<std::string> requiredExtensions(
        deviceExtensions_.begin(),
        deviceExtensions_.end()
    );

    for (const auto& extension : availableExtensions) {
        requiredExtensions.erase(extension.extensionName);
    }

    return requiredExtensions.empty();
}

bool VulkanRenderDevice::is_device_suitable(const vk::PhysicalDevice& device) {
    QueueFamilyIndices indices = find_queue_families(device);

    bool extensionsSupported = check_device_extension_support(device);

    bool swapChainAdequate = false;
    if (extensionsSupported) {
        SwapChainSupportDetails swapChainSupport{
            .capabilities = device.getSurfaceCapabilitiesKHR(gvSurface_),
            .formats = device.getSurfaceFormatsKHR(gvSurface_),
            .presentModes = device.getSurfacePresentModesKHR(gvSurface_)
        };
        swapChainAdequate = !swapChainSupport.formats.empty() &&
                            !swapChainSupport.presentModes.empty();
    }

    vk::PhysicalDeviceFeatures supportedFeatures = device.getFeatures();

    return indices.isComplete() && extensionsSupported && swapChainAdequate &&
           (supportedFeatures.samplerAnisotropy != 0U);
}

vk::SampleCountFlagBits VulkanRenderDevice::max_usable_sample_count() const {
    const auto props = gvPhysicalDevice_.getProperties();
    const auto counts = props.limits.framebufferColorSampleCounts &
                        props.limits.framebufferDepthSampleCounts;

    // unused but could later check
    const vk::FormatProperties colorFormatProps =
        gvPhysicalDevice_.getFormatProperties(vk::Format::eB8G8R8A8Unorm);
    const vk::FormatProperties depthFormatProps =
        gvPhysicalDevice_.getFormatProperties(vk::Format::eD32Sfloat);

    if (counts & vk::SampleCountFlagBits::e64) {
        return vk::SampleCountFlagBits::e64;
    }
    if (counts & vk::SampleCountFlagBits::e32) {
        return vk::SampleCountFlagBits::e32;
    }
    if (counts & vk::SampleCountFlagBits::e16) {
        return vk::SampleCountFlagBits::e16;
    }
    if (counts & vk::SampleCountFlagBits::e8) {
        return vk::SampleCountFlagBits::e8;
    }
    if (counts & vk::SampleCountFlagBits::e4) {
        return vk::SampleCountFlagBits::e4;
    }
    if (counts & vk::SampleCountFlagBits::e2) {
        return vk::SampleCountFlagBits::e2;
    }

    return vk::SampleCountFlagBits::e1;
}
}  // namespace garnish::vulkan
