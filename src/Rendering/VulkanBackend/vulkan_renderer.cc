#include "vulkan_renderer.hpp"

#include <SDL3/SDL_events.h>
#include <SDL3/SDL_vulkan.h>
#include <ecs_controller.h>
#include <stb_image.h>
#include <tiny_obj_loader.h>

#include <cstddef>
#include <iostream>
#include <read_file.hpp>
#include <sstream>
#include <stdexcept>
#include <span> 
#include <Utility/camera.hpp>
#include <Physics/physics_system.hpp> 
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_enums.hpp>
#include <vulkan/vulkan_structs.hpp>

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

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
    message << "\tmessageIDName   = <" << pCallbackData->pMessageIdName << ">\n";
    message << "\tmessageIdNumber = "
            << static_cast<uint32_t>(pCallbackData->messageIdNumber) << "\n";
    message << "\tmessage         = <" << pCallbackData->pMessage << ">\n";
    auto queueLabels = std::span(pCallbackData->pQueueLabels, pCallbackData->queueLabelCount);
    for (const auto& lbl : queueLabels) {
        message << "\tQueue Label: <" << lbl.pLabelName << ">\n";
    }
    auto cmdBufLabels = std::span(pCallbackData->pCmdBufLabels, pCallbackData->cmdBufLabelCount);
    for (const auto& lbl : cmdBufLabels) {
        message << "\tCmdBuf Label: <" << lbl.pLabelName << ">\n";
    }
    auto objects = std::span(pCallbackData->pObjects, pCallbackData->objectCount);
    uint32_t objIndex = 0;
    for (const auto& obj : objects) {
        message << "\tObject " << objIndex++ << " type="
                << vk::to_string(obj.objectType)
                << " handle=" << obj.objectHandle << "\n";
        if (obj.pObjectName) {
            message << "\t  name=<" << obj.pObjectName << ">\n";
        }
    }
    std::cerr << message.str() << '\n';
    return VK_FALSE;
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
} // anonymous namespace

namespace garnish {
bool VulkanRenderDevice::init(InitInfo& info) {
    window = static_cast<SDL_Window*>(info.nativeWindow);
    init_vulkan();
    return true;
}

bool VulkanRenderDevice::init_vulkan() {
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
    create_graphics_pipeline();
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
    load_texture("Textures/viking_room.png");

    create_command_buffers();
    create_sync_objects();
    setup_mesh("Models/viking_room.obj");
    return true;
}

void VulkanRenderDevice::cleanup() {
    gvDevice.waitIdle();
    cleanup_swap_chain();

    for (auto& texture : gvTextures) {
        gvDevice.destroyImageView(texture.textureImageView, nullptr);
        gvDevice.destroyImage(texture.textureImage, nullptr);
        gvDevice.freeMemory(texture.textureMemory, nullptr);
    }

    gvDevice.destroySampler(gvTextureSampler, nullptr);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        gvDevice.unmapMemory(uniformBuffersMemory[i]);
        gvDevice.destroyBuffer(uniformBuffers[i], nullptr);
        gvDevice.freeMemory(uniformBuffersMemory[i], nullptr);
    }

    destroy_model_buffers();

    gvDevice.destroyDescriptorPool(gvDescriptorPool, nullptr);
    gvDevice.destroyDescriptorSetLayout(gvDescriptorSetLayout, nullptr);

    gvDevice.destroyBuffer(indexBuffer, nullptr);
    gvDevice.freeMemory(indexBufferMemory, nullptr);

    gvDevice.destroyBuffer(vertexBuffer, nullptr);
    gvDevice.freeMemory(vertexBufferMemory, nullptr);

    gvDevice.destroyPipeline(gvPipeline, nullptr);
    gvDevice.destroyPipelineLayout(gvPipelineLayout, nullptr);

    gvDevice.destroyRenderPass(gvRenderPass, nullptr);

    for (auto& sem : imageAvailableSemaphores) { if (sem) gvDevice.destroySemaphore(sem); }
    for (auto& fence : inFlightFences) { if (fence) gvDevice.destroyFence(fence, nullptr); }

    gvDevice.destroyCommandPool(gvCommandPool, nullptr); 

    gvDevice.destroy();

    if (enableValidationLayers) {
        gvInstance.destroyDebugUtilsMessengerEXT(gvDebugMessenger);
    }

    gvInstance.destroySurfaceKHR(gvSurface);

    gvInstance.destroy();
}

void VulkanRenderDevice::update(ECSController& world) {
    draw_frame(world);
}

uint32_t VulkanRenderDevice::setup_mesh(const std::string& mesh_path) {
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn;
    std::string err;
    if (!tinyobj::LoadObj(
            &attrib,
            &shapes,
            &materials,
            &warn,
            &err,
            mesh_path.c_str()
        )) {
        throw std::runtime_error(warn + err);
    }
    std::vector<uint32_t> indices;
    std::vector<GVVertex3d> vertices;
    std::unordered_map<GVVertex3d, uint32_t> uniqueVertices{};

    for (const auto& shape : shapes) {
        for (const auto& index : shape.mesh.indices) {
            GVVertex3d vertex{};
            vertex.pos = {
                attrib.vertices[(3 * index.vertex_index) + 0],
                attrib.vertices[(3 * index.vertex_index) + 1],
                attrib.vertices[(3 * index.vertex_index) + 2]
            };

            vertex.texCoord = {
                attrib.texcoords[(2 * index.texcoord_index) + 0],
                1.0F - attrib.texcoords[(2 * index.texcoord_index) + 1]
            };

            vertex.color = {1.0F, 1.0F, 1.0F};

            if (!uniqueVertices.contains(vertex)) {
                uniqueVertices[vertex] = static_cast<uint32_t>(vertices.size());
                vertices.push_back(vertex);
            }

            indices.push_back(uniqueVertices[vertex]);
        }
    }

    gvMeshes.push_back(
        GVMesh{
            .firstVertex =
                static_cast<uint32_t>(totalVertexBytes / sizeof(GVVertex3d)),
            .vertexCount = static_cast<uint32_t>(vertices.size()),
            .firstIndex =
                static_cast<uint32_t>(totalIndexBytes / sizeof(uint32_t)),
            .indexCount = static_cast<uint32_t>(indices.size())
        }
    );

    vk::DeviceSize vertexBufferSize = sizeof(GVVertex3d) * vertices.size();
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
        gvDevice.mapMemory(vertexStagingBufferMemory, 0, vertexBufferSize),
        vertices.data(),
        (size_t)vertexBufferSize
    );
    gvDevice.unmapMemory(vertexStagingBufferMemory);

    create_buffer(
        indexBufferSize,
        vk::BufferUsageFlagBits::eTransferSrc,
        vk::MemoryPropertyFlagBits::eHostVisible |
            vk::MemoryPropertyFlagBits::eHostCoherent,
        indexStagingBuffer,
        indexStagingBufferMemory
    );
    memcpy(
        gvDevice.mapMemory(indexStagingBufferMemory, 0, indexBufferSize),
        indices.data(),
        (size_t)indexBufferSize
    );
    gvDevice.unmapMemory(indexStagingBufferMemory);

    copy_buffer(
        vertexStagingBuffer,
        vertexBuffer,
        vertexBufferSize,
        totalVertexBytes
    );

    copy_buffer(
        indexStagingBuffer,
        indexBuffer,
        indexBufferSize,
        totalIndexBytes
    );

    gvDevice.destroyBuffer(vertexStagingBuffer);
    gvDevice.freeMemory(vertexStagingBufferMemory);

    gvDevice.destroyBuffer(indexStagingBuffer);
    gvDevice.freeMemory(indexStagingBufferMemory);

    totalVertexBytes += sizeof(GVVertex3d) * vertices.size();
    totalIndexBytes += sizeof(uint32_t) * indices.size();
    return gvMeshes.size() - 1;
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

    if (enableValidationLayers) {
        extensions.push_back(vk::EXTDebugUtilsExtensionName);
    }

    uint32_t layerCount = enableValidationLayers
                              ? static_cast<uint32_t>(validationLayers.size())
                              : 0;
    const char* const* layers =
        enableValidationLayers ? validationLayers.data() : nullptr;

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
        std::cerr  // TODO should throw an error
            << "ERROR: missing one or more needed validation layers\n";
        return false;
    }

    gvInstance = vk::createInstance(instanceCreateInfo, nullptr);
    VULKAN_HPP_DEFAULT_DISPATCHER.init(gvInstance);
    return true;
}

bool VulkanRenderDevice::setup_debug_messenger() {
    if (!enableValidationLayers) return false;
    auto pfnVkCreateDebugUtilsMessengerEXT =
        reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>( // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
            gvInstance.getProcAddr("vkCreateDebugUtilsMessengerEXT")
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
    gvDebugMessenger = gvInstance.createDebugUtilsMessengerEXT(createInfo);
    return true;
}

bool VulkanRenderDevice::pick_physical_device() {
    std::vector physicalDevices = gvInstance.enumeratePhysicalDevices();
    if (physicalDevices.size() == 0) {
        throw std::runtime_error("no physical devices for vulkan");
    }

    gvPhysicalDevice = nullptr;
    for (auto& physicalDevice : physicalDevices) {
        if (is_device_suitable(physicalDevice)) {
            gvPhysicalDevice = physicalDevice;
            msaaSamples = max_usable_sample_count();
            break;
        }
    }

    if (!gvPhysicalDevice) {
        throw std::runtime_error("no suitable physical device");
    }

    const auto props2 = gvPhysicalDevice.getProperties2();
    textureLimit = props2.properties.limits.maxPerStageDescriptorSampledImages;

    auto availableExtensions =
        gvPhysicalDevice.enumerateDeviceExtensionProperties();

    for (const auto& extension : availableExtensions) {
        if (strcmp(extension.extensionName, "VK_KHR_portability_subset") == 0) {
            deviceExtensions.push_back("VK_KHR_portability_subset");
            break;
        }
    }

    return true;
}

bool VulkanRenderDevice::create_logical_device() {
    QueueFamilyIndices indices = find_queue_families(gvPhysicalDevice);

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

    vk::PhysicalDeviceDescriptorIndexingFeatures indexingFeatures{};
    indexingFeatures.runtimeDescriptorArray = VK_TRUE;
    indexingFeatures.descriptorBindingPartiallyBound = VK_TRUE;
    indexingFeatures.descriptorBindingVariableDescriptorCount = VK_TRUE;
    indexingFeatures.shaderSampledImageArrayNonUniformIndexing = VK_TRUE;
    indexingFeatures.descriptorBindingSampledImageUpdateAfterBind = VK_TRUE;

    vk::DeviceCreateInfo createInfo{
        {},
        static_cast<uint32_t>(queueCreateInfos.size()),
        queueCreateInfos.data(),
        static_cast<uint32_t>(validationLayers.size()),
        validationLayers.data(),
        static_cast<uint32_t>(deviceExtensions.size()),
        deviceExtensions.data(),
        &deviceFeatures,
        &indexingFeatures
    };

    gvDevice = gvPhysicalDevice.createDevice(createInfo);
    gvGraphicsQueue = gvDevice.getQueue(indices.graphicsFamily.value(), 0);
    gvPresentQueue = gvDevice.getQueue(indices.presentFamily.value(), 0);

    VULKAN_HPP_DEFAULT_DISPATCHER.init(gvDevice);
    return true;
}

bool VulkanRenderDevice::create_surface() {
    VkSurfaceKHR khr = nullptr;
    if (!SDL_Vulkan_CreateSurface(window, gvInstance, nullptr, &khr)) {
        throw std::runtime_error("failed to create window surface!");
    }
    gvSurface = khr;
    return true;
}

bool VulkanRenderDevice::create_swap_chain() {
    SwapChainSupportDetails swapChainSupport{
        .capabilities = gvPhysicalDevice.getSurfaceCapabilitiesKHR(gvSurface),
        .formats = gvPhysicalDevice.getSurfaceFormatsKHR(gvSurface),
        .presentModes = gvPhysicalDevice.getSurfacePresentModesKHR(gvSurface)
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

    QueueFamilyIndices indices = find_queue_families(gvPhysicalDevice);
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
        gvSurface,
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

    gvSwapchainKHR = gvDevice.createSwapchainKHR(createInfo);

    gvSwapChainImageFormat = surfaceFormat.format;
    gvSwapChainExtent = extent;
    swapChainImages = gvDevice.getSwapchainImagesKHR(gvSwapchainKHR);
    return true;
}

bool VulkanRenderDevice::recreate_swap_chain() {
    gvDevice.waitIdle();

    int width = 0;
    int height = 0;
    while (width == 0 || height == 0) {
        SDL_Event event;
        SDL_GetWindowSize(window, &width, &height);
    }

    const auto imageCount = static_cast<uint32_t>(swapChainImages.size());
    imageInFlight.assign(imageCount, VK_NULL_HANDLE);

    gvDevice.waitIdle();

    cleanup_swap_chain();

    create_swap_chain();
    create_image_views();
    create_color_resources();
    create_depth_resources();
    create_framebuffers();

    for (auto& sem : renderFinishedSemaphores) {
        sem = gvDevice.createSemaphore({});
    }

    imageInFlight.clear();
    imageInFlight.resize(imageCount, VK_NULL_HANDLE);

    return true;
}

bool VulkanRenderDevice::cleanup_swap_chain() {
    gvDevice.destroyImageView(colorImageView);
    gvDevice.destroyImage(colorImage);
    gvDevice.freeMemory(colorImageMemory);

    gvDevice.destroyImageView(depthImageView);
    gvDevice.destroyImage(depthImage);
    gvDevice.freeMemory(depthImageMemory);

    for (auto framebuffer : swapChainFramebuffers) {
        gvDevice.destroyFramebuffer(framebuffer);
    }

    for (auto imageView : swapChainImageViews) {
        gvDevice.destroyImageView(imageView);
    }
    for (auto sem : renderFinishedSemaphores) {
        gvDevice.destroySemaphore(sem);
    }

    gvDevice.destroySwapchainKHR(gvSwapchainKHR);
    return true;
}

bool VulkanRenderDevice::create_image_views() {
    swapChainImageViews.resize(swapChainImages.size());

    for (size_t i = 0; i < swapChainImages.size(); i++) {
        swapChainImageViews[i] = create_image_view(
            swapChainImages[i],
            gvSwapChainImageFormat,
            vk::ImageAspectFlagBits::eColor,
            1
        );
    }
    return true;
}

vk::ImageView VulkanRenderDevice::create_image_view(
    vk::Image image,
    vk::Format format,
    vk::ImageAspectFlags aspectFlags,
    uint32_t mipLevels
) {
    vk::ImageViewCreateInfo viewInfo{
        {},
        image,
        vk::ImageViewType::e2D,
        format,
        {},
        vk::ImageSubresourceRange{aspectFlags, 0, mipLevels, 0, 1}
    };

    return gvDevice.createImageView(viewInfo);
}

bool VulkanRenderDevice::create_descriptor_set_layout() {
    // binding 0: camera UBO, binding 1: model storage buffer, binding 2: sampler, binding 3: sampled image array
    vk::DescriptorSetLayoutBinding camBinding{0, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eVertex};
    vk::DescriptorSetLayoutBinding modelBinding{1, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eVertex};
    vk::DescriptorSetLayoutBinding sampBinding{2, vk::DescriptorType::eSampler, 1, vk::ShaderStageFlagBits::eFragment, &gvTextureSampler};
    vk::DescriptorSetLayoutBinding imgBinding{3, vk::DescriptorType::eSampledImage, textureLimit, vk::ShaderStageFlagBits::eFragment};
    std::array bindings{camBinding, modelBinding, sampBinding, imgBinding};
    std::array bindingFlags{
        vk::DescriptorBindingFlags{},
        vk::DescriptorBindingFlags{},
        vk::DescriptorBindingFlags{},
        vk::DescriptorBindingFlags{vk::DescriptorBindingFlagBits::ePartiallyBound | vk::DescriptorBindingFlagBits::eVariableDescriptorCount | vk::DescriptorBindingFlagBits::eUpdateAfterBind}
    };
    vk::DescriptorSetLayoutBindingFlagsCreateInfo flagsInfo{bindings.size(), bindingFlags.data()};
    vk::DescriptorSetLayoutCreateInfo layoutInfo{vk::DescriptorSetLayoutCreateFlagBits::eUpdateAfterBindPool, bindings.size(), bindings.data(), &flagsInfo};
    gvDescriptorSetLayout = gvDevice.createDescriptorSetLayout(layoutInfo);
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
            gvSwapChainImageFormat,
            msaaSamples,
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
            msaaSamples,
            vk::AttachmentLoadOp::eClear,
            vk::AttachmentStoreOp::eDontCare,
            vk::AttachmentLoadOp::eDontCare,
            vk::AttachmentStoreOp::eDontCare,
            vk::ImageLayout::eUndefined,
            vk::ImageLayout::eDepthStencilAttachmentOptimal
        },
        vk::AttachmentDescription{
            {},
            gvSwapChainImageFormat,
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
    gvRenderPass = gvDevice.createRenderPass(renderPassInfo);

    return true;
}

bool VulkanRenderDevice::create_graphics_pipeline() {
    auto vertShaderCode = read_file("shaders/vert.spv");
    auto fragShaderCode = read_file("shaders/frag.spv");
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

    auto bindingDescription = GVVertex3d::getBindingDescription();
    auto attributeDescriptions = GVVertex3d::getAttributeDescriptions();

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
        VK_FALSE
    };

    vk::PipelineViewportStateCreateInfo viewportState{{}, 1, {}, 1};

    vk::PipelineRasterizationStateCreateInfo rasterizer{
        {},
        VK_FALSE,
        VK_FALSE,
        vk::PolygonMode::eFill,
        vk::CullModeFlagBits::eBack,
        vk::FrontFace::eCounterClockwise,
        VK_FALSE,
        0.0F,
        0.0F,
        0.0F,
        1.0F
    };

    vk::PipelineMultisampleStateCreateInfo multisampling{
        {},
        msaaSamples,
        VK_TRUE,
        kSampleRateShadingMinFraction,
        nullptr,
        VK_FALSE,
        VK_FALSE
    };

    vk::PipelineColorBlendAttachmentState colorBlendAttachment{
        VK_FALSE,
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
        VK_FALSE,
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
    struct PC { uint32_t texIndex; uint32_t modelIndex; };
    vk::PushConstantRange pcRange{vk::ShaderStageFlagBits::eFragment | vk::ShaderStageFlagBits::eVertex, 0, sizeof(PC)};

    vk::PipelineLayoutCreateInfo pipelineLayoutInfo{
        {},
        1,
        &gvDescriptorSetLayout,
        1,
        &pcRange
    };

    gvPipelineLayout = gvDevice.createPipelineLayout(pipelineLayoutInfo);

    vk::PipelineDepthStencilStateCreateInfo depthStencil{
        {},
        VK_TRUE,
        VK_TRUE,
        vk::CompareOp::eLess,
        VK_FALSE,
        VK_FALSE
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
        gvPipelineLayout,
        gvRenderPass,
        0,
        VK_NULL_HANDLE,
        -1
    };

    gvPipeline =
        gvDevice.createGraphicsPipeline(VK_NULL_HANDLE, pipelineInfo).value;
    gvDevice.destroyShaderModule(fragShaderModule);
    gvDevice.destroyShaderModule(vertShaderModule);
    return true;
}

bool VulkanRenderDevice::create_color_resources() {
    vk::Format colorFormat = gvSwapChainImageFormat;

    create_image(
        gvSwapChainExtent.width,
        gvSwapChainExtent.height,
        1,
        msaaSamples,
        colorFormat,
        vk::ImageTiling::eOptimal,
        vk::ImageUsageFlagBits::eTransientAttachment |
            vk::ImageUsageFlagBits::eColorAttachment,
        vk::MemoryPropertyFlagBits::eDeviceLocal,
        colorImage,
        colorImageMemory
    );
    colorImageView = create_image_view(
        colorImage,
        colorFormat,
        vk::ImageAspectFlagBits::eColor,
        1
    );
    return true;
}

bool VulkanRenderDevice::create_depth_resources() {
    vk::Format depthFormat = find_depth_format();

    create_image(
        gvSwapChainExtent.width,
        gvSwapChainExtent.height,
        1,
        msaaSamples,
        depthFormat,
        vk::ImageTiling::eOptimal,
        vk::ImageUsageFlagBits::eDepthStencilAttachment,
        vk::MemoryPropertyFlagBits::eDeviceLocal,
        depthImage,
        depthImageMemory
    );
    depthImageView = create_image_view(
        depthImage,
        depthFormat,
        vk::ImageAspectFlagBits::eDepth,
        1
    );
    return true;
}

bool VulkanRenderDevice::create_framebuffers() {
    swapChainFramebuffers.resize(swapChainImageViews.size());

    for (size_t i = 0; i < swapChainImageViews.size(); i++) {
        std::array attachments = {
            colorImageView,
            depthImageView,
            swapChainImageViews[i]
        };

        vk::FramebufferCreateInfo framebufferInfo{
            {},
            gvRenderPass,
            static_cast<uint32_t>(attachments.size()),
            attachments.data(),
            gvSwapChainExtent.width,
            gvSwapChainExtent.height,
            1
        };
        swapChainFramebuffers[i] = gvDevice.createFramebuffer(framebufferInfo);
    }
    return true;
}

bool VulkanRenderDevice::create_command_pool() {
    QueueFamilyIndices queueFamilyIndices =
        find_queue_families(gvPhysicalDevice);

    vk::CommandPoolCreateInfo poolInfo{
        vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
        queueFamilyIndices.graphicsFamily.value()
    };

    gvCommandPool = gvDevice.createCommandPool(poolInfo);

    return true;
}

bool VulkanRenderDevice::create_texture_sampler() {
    vk::PhysicalDeviceProperties properties = gvPhysicalDevice.getProperties();

    gvTextureSampler = gvDevice.createSampler(
        {{},
         vk::Filter::eLinear,
         vk::Filter::eLinear,
         vk::SamplerMipmapMode::eLinear,
         vk::SamplerAddressMode::eRepeat,
         vk::SamplerAddressMode::eRepeat,
         vk::SamplerAddressMode::eRepeat,
         0.0F,
         VK_TRUE,
         properties.limits.maxSamplerAnisotropy,
         VK_FALSE,
         vk::CompareOp::eAlways,
         0.0F,
         VK_LOD_CLAMP_NONE,
         vk::BorderColor::eIntOpaqueBlack}
    );
    return true;
}

uint32_t VulkanRenderDevice::load_texture(const std::string& path) {
    auto stringHash = std::hash<std::string>{}(path);
    if (loadedTextures.contains(stringHash)) {
        return loadedTextures[stringHash];
    }
    loadedTextures[stringHash] = gvTextures.size();

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
    void* data = gvDevice.mapMemory(stagingBufferMemory, 0, imageSize);
    memcpy(data, pixels, (size_t)imageSize);
    gvDevice.unmapMemory(stagingBufferMemory);

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

    gvDevice.destroyBuffer(stagingBuffer);
    gvDevice.freeMemory(stagingBufferMemory);

    vk::ImageView textureImageView = create_image_view(
        textureImage,
        vk::Format::eR8G8B8A8Srgb,
        vk::ImageAspectFlagBits::eColor,
        mipLevels
    );

    gvTextures.push_back(
        GVTexture{
            .mipLevels = mipLevels,
            .textureImage = textureImage,
            .textureMemory = textureImageMemory,
            .textureImageView = textureImageView,
        }
    );

    update_descriptor_sets();
    return gvTextures.size() - 1;
}

void VulkanRenderDevice::create_image(
    uint32_t width,
    uint32_t height,
    uint32_t mipLevels,
    vk::SampleCountFlagBits numSamples,
    vk::Format format,
    vk::ImageTiling tiling,
    vk::ImageUsageFlags usage,
    vk::MemoryPropertyFlags properties,
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
    image = gvDevice.createImage(imageInfo);

    vk::MemoryRequirements memRequirements =
        gvDevice.getImageMemoryRequirements(image);

    vk::MemoryAllocateInfo allocInfo{
        memRequirements.size,
        find_memory_type(memRequirements.memoryTypeBits, properties)
    };

    imageMemory = gvDevice.allocateMemory(allocInfo);

    gvDevice.bindImageMemory(image, imageMemory, 0);
}

void VulkanRenderDevice::create_vertex_buffer() {
    create_buffer(
        bufferDefaultSize,
        vk::BufferUsageFlagBits::eTransferDst |
            vk::BufferUsageFlagBits::eVertexBuffer,
        vk::MemoryPropertyFlagBits::eDeviceLocal,
        vertexBuffer,
        vertexBufferMemory
    );
}

void VulkanRenderDevice::create_index_buffer() {
    create_buffer(
        bufferDefaultSize,  // TODO replace with sane size
        vk::BufferUsageFlagBits::eTransferDst |
            vk::BufferUsageFlagBits::eIndexBuffer,
        vk::MemoryPropertyFlagBits::eDeviceLocal,
        indexBuffer,
        indexBufferMemory
    );
}

void VulkanRenderDevice::create_uniform_buffers() {
    vk::DeviceSize bufferSize = sizeof(CameraUBO);

    uniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
    uniformBuffersMemory.resize(MAX_FRAMES_IN_FLIGHT);
    uniformBuffersMapped.resize(MAX_FRAMES_IN_FLIGHT);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        create_buffer(
            bufferSize,
            vk::BufferUsageFlagBits::eUniformBuffer,
            vk::MemoryPropertyFlagBits::eHostVisible |
                vk::MemoryPropertyFlagBits::eHostCoherent,
            uniformBuffers[i],
            uniformBuffersMemory[i]
        );

        uniformBuffersMapped[i] =
            gvDevice.mapMemory(uniformBuffersMemory[i], 0, bufferSize);
    }
}

void VulkanRenderDevice::create_model_buffers(uint32_t minCapacity) {
    modelBuffers.resize(MAX_FRAMES_IN_FLIGHT);
    modelBuffersMemory.resize(MAX_FRAMES_IN_FLIGHT);
    modelBuffersMapped.resize(MAX_FRAMES_IN_FLIGHT);
    modelBufferCapacity = std::max(minCapacity, kInitialModelCapacity);
    vk::DeviceSize bufferSize = static_cast<vk::DeviceSize>(modelBufferCapacity) * sizeof(glm::mat4);
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        create_buffer(bufferSize, vk::BufferUsageFlagBits::eStorageBuffer, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, modelBuffers[i], modelBuffersMemory[i]);
        modelBuffersMapped[i] = gvDevice.mapMemory(modelBuffersMemory[i], 0, bufferSize);
    }
}

void VulkanRenderDevice::destroy_model_buffers() {
    for (size_t i = 0; i < modelBuffers.size(); ++i) {
        if (modelBuffers[i]) {
            gvDevice.unmapMemory(modelBuffersMemory[i]);
            gvDevice.destroyBuffer(modelBuffers[i]);
            gvDevice.freeMemory(modelBuffersMemory[i]);
        }
    }
    modelBuffers.clear();
    modelBuffersMemory.clear();
    modelBuffersMapped.clear();
    modelBufferCapacity = 0;
}

void VulkanRenderDevice::ensure_model_capacity(uint32_t requiredModelCount) {
    if (requiredModelCount <= modelBufferCapacity) return;
    uint32_t newCap = modelBufferCapacity;
    while (newCap < requiredModelCount) newCap *= 2;
    destroy_model_buffers();
    create_model_buffers(newCap);
    update_descriptor_sets();
}

void VulkanRenderDevice::update_camera_buffer(uint32_t currentImage) {
    memcpy(uniformBuffersMapped[currentImage], &cameraUbo, sizeof(CameraUBO));
}

void VulkanRenderDevice::update_model_buffer(uint32_t currentImage, const std::vector<glm::mat4>& models) {
    auto bytes = models.size() * sizeof(glm::mat4);
    memcpy(modelBuffersMapped[currentImage], models.data(), bytes);
}

bool VulkanRenderDevice::create_descriptor_pool() {
    std::array poolSizes{
        vk::DescriptorPoolSize{vk::DescriptorType::eUniformBuffer, MAX_FRAMES_IN_FLIGHT},
        vk::DescriptorPoolSize{vk::DescriptorType::eStorageBuffer, MAX_FRAMES_IN_FLIGHT},
        vk::DescriptorPoolSize{vk::DescriptorType::eSampler, MAX_FRAMES_IN_FLIGHT},
        vk::DescriptorPoolSize{vk::DescriptorType::eSampledImage, textureLimit * MAX_FRAMES_IN_FLIGHT}
    };

    vk::DescriptorPoolCreateInfo poolInfo{
        vk::DescriptorPoolCreateFlagBits::eUpdateAfterBind,
        MAX_FRAMES_IN_FLIGHT,
        poolSizes.size(),
        poolSizes.data()
    };

    gvDescriptorPool = gvDevice.createDescriptorPool(poolInfo);
    return true;
}

bool VulkanRenderDevice::create_descriptor_sets() {
    std::vector<vk::DescriptorSetLayout> layouts(
        MAX_FRAMES_IN_FLIGHT,
        gvDescriptorSetLayout
    );

    std::vector<uint32_t> variableCounts(MAX_FRAMES_IN_FLIGHT, textureLimit);

    vk::DescriptorSetVariableDescriptorCountAllocateInfo varInfo{
        MAX_FRAMES_IN_FLIGHT,
        variableCounts.data()
    };

    vk::DescriptorSetAllocateInfo allocInfo{
        gvDescriptorPool,
        static_cast<uint32_t>(layouts.size()),
        layouts.data(),
        &varInfo

    };

    descriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
    descriptorSets = gvDevice.allocateDescriptorSets(allocInfo);

    update_descriptor_sets();
    return true;
}

void VulkanRenderDevice::update_descriptor_sets() {
    if (descriptorSets.empty()) return;
    std::vector<vk::DescriptorImageInfo> imageInfos(gvTextures.size());
    for (uint32_t i = 0; i < gvTextures.size(); ++i) {
        imageInfos[i] = vk::DescriptorImageInfo{VK_NULL_HANDLE, gvTextures[i].textureImageView, vk::ImageLayout::eShaderReadOnlyOptimal};
    }
    std::vector<vk::DescriptorBufferInfo> camInfos(MAX_FRAMES_IN_FLIGHT);
    std::vector<vk::DescriptorBufferInfo> modelInfos(MAX_FRAMES_IN_FLIGHT);
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        camInfos[i] = vk::DescriptorBufferInfo{uniformBuffers[i], 0, sizeof(CameraUBO)};
        if (!modelBuffers.empty()) {
            modelInfos[i] = vk::DescriptorBufferInfo{modelBuffers[i], 0, static_cast<vk::DeviceSize>(modelBufferCapacity) * sizeof(glm::mat4)};
        }
    }
    std::vector<vk::WriteDescriptorSet> writes;
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        writes.emplace_back(descriptorSets[i], 0, 0, 1, vk::DescriptorType::eUniformBuffer, nullptr, &camInfos[i]);
        if (!modelBuffers.empty()) {
            writes.emplace_back(descriptorSets[i], 1, 0, 1, vk::DescriptorType::eStorageBuffer, nullptr, &modelInfos[i]);
        }
        if (!gvTextures.empty()) {
            writes.emplace_back(descriptorSets[i], 3, 0, static_cast<uint32_t>(gvTextures.size()), vk::DescriptorType::eSampledImage, imageInfos.data());
        }
    }
    if (!writes.empty()) {
        gvDevice.updateDescriptorSets(static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);
    }
}

void VulkanRenderDevice::create_buffer(
    vk::DeviceSize size,
    vk::BufferUsageFlags usage,
    vk::MemoryPropertyFlags properties,
    vk::Buffer& buffer,
    vk::DeviceMemory& bufferMemory
) {
    vk::BufferCreateInfo bufferInfo{
        {},
        size,
        usage,
        vk::SharingMode::eExclusive
    };
    buffer = gvDevice.createBuffer(bufferInfo);

    vk::MemoryRequirements memRequirements =
        gvDevice.getBufferMemoryRequirements(buffer);

    vk::MemoryAllocateInfo allocInfo{
        memRequirements.size,
        find_memory_type(memRequirements.memoryTypeBits, properties)
    };

    bufferMemory = gvDevice.allocateMemory(allocInfo);
    gvDevice.bindBufferMemory(buffer, bufferMemory, 0);
}

void VulkanRenderDevice::copy_buffer(
    vk::Buffer srcBuffer,
    vk::Buffer dstBuffer,
    vk::DeviceSize size,
    vk::DeviceSize dstOffset
) {
    vk::CommandBuffer commandBuffer = begin_single_time_commands();
    vk::BufferCopy copyRegion{0, dstOffset, size};
    commandBuffer.copyBuffer(srcBuffer, dstBuffer, 1, &copyRegion);
    end_single_time_commands(commandBuffer);
}

uint32_t VulkanRenderDevice::find_memory_type(
    uint32_t typeFilter,
    vk::MemoryPropertyFlags properties
) {
    vk::PhysicalDeviceMemoryProperties memProperties =
        gvPhysicalDevice.getMemoryProperties();

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
        gvCommandPool,
        vk::CommandBufferLevel::ePrimary,
        1
    };

    vk::CommandBuffer commandBuffer =
        gvDevice.allocateCommandBuffers(allocInfo)[0];

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

    gvGraphicsQueue.submit(submitInfos, VK_NULL_HANDLE);

    gvGraphicsQueue.waitIdle();
    gvDevice.freeCommandBuffers(gvCommandPool, commandBuffer);
}

void VulkanRenderDevice::transition_image_layout(
    vk::Image image,
    vk::Format /*format*/,
    vk::ImageLayout oldLayout,
    vk::ImageLayout newLayout,
    uint32_t mipLevels
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
    vk::Buffer buffer,
    vk::Image image,
    uint32_t width,
    uint32_t height
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
    gvCommandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

    vk::CommandBufferAllocateInfo allocInfo{
        gvCommandPool,
        vk::CommandBufferLevel::ePrimary,
        static_cast<uint32_t>(gvCommandBuffers.size())
    };

    gvCommandBuffers = gvDevice.allocateCommandBuffers(allocInfo);
    return true;
}

void VulkanRenderDevice::record_command_buffer(
    vk::CommandBuffer commandBuffer,
    uint32_t imageIndex,
    ECSController& world
) {
    commandBuffer.begin({vk::CommandBufferUsageFlagBits::eSimultaneousUse});

    std::array clearValues{
        vk::ClearValue{{0.0F, 0.0F, 0.0F, 1.0F}},
        vk::ClearValue{vk::ClearDepthStencilValue{1.0F, 0}}
    };

    vk::RenderPassBeginInfo renderPassInfo{
        gvRenderPass,
        swapChainFramebuffers[imageIndex],
        {{0, 0}, gvSwapChainExtent},
        clearValues.size(),
        clearValues.data()
    };

    commandBuffer.beginRenderPass(renderPassInfo, vk::SubpassContents::eInline);

    commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, gvPipeline);

    vk::Viewport viewport{
        0.0F,
        0.0F,
        static_cast<float>(gvSwapChainExtent.width),
        static_cast<float>(gvSwapChainExtent.height),
        0.0F,
        1.0F
    };

    commandBuffer.setViewport(0, 1, &viewport);

    vk::Rect2D scissor{{0, 0}, gvSwapChainExtent};

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
        proj = glm::perspective(glm::radians(kFovDeg), static_cast<float>(gvSwapChainExtent.width) / static_cast<float>(gvSwapChainExtent.height), kNear, kFar);
        proj[1][1] *= -1.0F;
    }
    cameraUbo.view = view;
    cameraUbo.proj = proj;

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
    update_model_buffer(currentFrame, modelMatrices);

    // Bind descriptor sets once
    commandBuffer.bindDescriptorSets(
        vk::PipelineBindPoint::eGraphics,
        gvPipelineLayout,
        0,
        1,
        &descriptorSets[currentFrame],
        0,
        nullptr
    );

    uint32_t modelIdx = 0;
    for (auto e : entities) {
        const auto& r = world.get_component<Renderable>(e);
        const auto& msh = gvMeshes[r.meshHandle];
        vk::DeviceSize vByteOffset = static_cast<vk::DeviceSize>(msh.firstVertex) * sizeof(GVVertex3d);
        commandBuffer.bindVertexBuffers(0, 1, &vertexBuffer, &vByteOffset);
        commandBuffer.bindIndexBuffer(indexBuffer, static_cast<vk::DeviceSize>(msh.firstIndex) * sizeof(uint32_t), vk::IndexType::eUint32);
        struct PC { uint32_t texIndex; uint32_t modelIndex; } pc{.texIndex=r.texHandle, .modelIndex=modelIdx};
        commandBuffer.pushConstants(
            gvPipelineLayout,
            vk::ShaderStageFlagBits::eFragment | vk::ShaderStageFlagBits::eVertex,
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
    imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

    vk::FenceCreateInfo fenceInfo{vk::FenceCreateFlagBits::eSignaled};

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        imageAvailableSemaphores[i] = gvDevice.createSemaphore({});
        inFlightFences[i] = gvDevice.createFence(fenceInfo);
    }

    const auto imageCount = static_cast<uint32_t>(swapChainImages.size());
    renderFinishedSemaphores.resize(imageCount);
    imageInFlight.assign(imageCount, VK_NULL_HANDLE);

    for (size_t i = 0; i < imageCount; i++) {
        renderFinishedSemaphores[i] = gvDevice.createSemaphore({});
    }

    return true;
}

bool VulkanRenderDevice::draw_frame(ECSController& world) {
    (void)gvDevice.waitForFences(inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);

    uint32_t imageIndex = 0;
    auto aquireResult = gvDevice.acquireNextImageKHR(
        gvSwapchainKHR,
        UINT64_MAX,
        imageAvailableSemaphores[currentFrame],
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

    if (imageInFlight[imageIndex] != VK_NULL_HANDLE) {
        (void)gvDevice.waitForFences(1, &imageInFlight[imageIndex], VK_TRUE, UINT64_MAX);
    }
    imageInFlight[imageIndex] = inFlightFences[currentFrame];

    // Only reset the fence if we are submitting work
    (void)gvDevice.resetFences(1, &inFlightFences[currentFrame]);
    gvCommandBuffers[currentFrame].reset();

    record_command_buffer(gvCommandBuffers[currentFrame], imageIndex, world);
    update_camera_buffer(currentFrame);
    const std::array<vk::Semaphore,1> waitSemaphores{imageAvailableSemaphores[currentFrame]};
    const std::array<vk::PipelineStageFlags,1> waitStages{vk::PipelineStageFlagBits::eColorAttachmentOutput};
    const std::array<vk::Semaphore,1> signalSemaphores{renderFinishedSemaphores[imageIndex]};

    vk::SubmitInfo submitInfo{
        static_cast<uint32_t>(waitSemaphores.size()),
        waitSemaphores.data(),
        waitStages.data(),
        1,
        &gvCommandBuffers[currentFrame],
        static_cast<uint32_t>(signalSemaphores.size()),
        signalSemaphores.data()
    };

    (void)gvGraphicsQueue.submit(1, &submitInfo, inFlightFences[currentFrame]);
    const std::array<vk::SwapchainKHR,1> swapChains{gvSwapchainKHR};

    vk::PresentInfoKHR presentInfo{
        static_cast<uint32_t>(signalSemaphores.size()),
        signalSemaphores.data(),
        static_cast<uint32_t>(swapChains.size()),
        swapChains.data(),
        &imageIndex
    };

    auto presentResult = gvPresentQueue.presentKHR(&presentInfo);

    if (presentResult == vk::Result::eErrorOutOfDateKHR ||
        presentResult == vk::Result::eSuboptimalKHR) {
        framebufferResized = false;
        recreate_swap_chain();
    } else if (presentResult != vk::Result::eSuccess) {
        throw std::runtime_error(
            "update_descriptor_setsfailed to present swap chain image!"
        );
    }

    currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
    return true;
}

void VulkanRenderDevice::generate_mipmaps(
    vk::Image image,
    vk::Format imageFormat,
    TextureSize size,
    uint32_t mipLevels
) {
    vk::FormatProperties formatProperties =
        gvPhysicalDevice.getFormatProperties(imageFormat);
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
             vk::Offset3D{(mipWidth > 1 ? mipWidth / 2 : 1),
                          (mipHeight > 1 ? mipHeight / 2 : 1),
                          1}}
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
        if (mipWidth > 1) { mipWidth /= 2; }
        if (mipHeight > 1) { mipHeight /= 2; }
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
    vk::PhysicalDevice& device
) {
    QueueFamilyIndices indices;
    auto queueFamilies = device.getQueueFamilyProperties();

    int i = 0;
    for (const auto& queueFamily : queueFamilies) {
        vk::Bool32 presentSupport = device.getSurfaceSupportKHR(i, gvSurface);
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
    vk::ImageTiling tiling,
    vk::FormatFeatureFlags features
) {
    for (const vk::Format& format : candidates) {
        vk::FormatProperties props =
            gvPhysicalDevice.getFormatProperties(format);

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
    return gvDevice.createShaderModule(
        vk::ShaderModuleCreateInfo{
            {},
            code.size(),
            reinterpret_cast<const uint32_t*>(code.data()) // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast,performance-no-int-to-ptr)
        }
    );
}

bool VulkanRenderDevice::check_device_extension_support(
    vk::PhysicalDevice& device
) {
    const auto availableExtensions =
        device.enumerateDeviceExtensionProperties();

    std::set<std::string> requiredExtensions(
        deviceExtensions.begin(),
        deviceExtensions.end()
    );
    // std::cerr << "ee\n";
    for (const auto& extension : availableExtensions) {
        // std::cerr << extension.extensionName << '\n';
        requiredExtensions.erase(extension.extensionName);
    }
    // std::cerr << "ee\n";

    return requiredExtensions.empty();
}

bool VulkanRenderDevice::is_device_suitable(vk::PhysicalDevice& device) {
    QueueFamilyIndices indices = find_queue_families(device);

    bool extensionsSupported = check_device_extension_support(device);

    bool swapChainAdequate = false;
    if (extensionsSupported) {
        SwapChainSupportDetails swapChainSupport{
            .capabilities = device.getSurfaceCapabilitiesKHR(gvSurface),
            .formats = device.getSurfaceFormatsKHR(gvSurface),
            .presentModes = device.getSurfacePresentModesKHR(gvSurface)
        };
        swapChainAdequate = !swapChainSupport.formats.empty() &&
                            !swapChainSupport.presentModes.empty();
    }

    vk::PhysicalDeviceFeatures supportedFeatures = device.getFeatures();

    return indices.isComplete() && extensionsSupported && swapChainAdequate &&
           (supportedFeatures.samplerAnisotropy != 0U);
}

vk::SampleCountFlagBits VulkanRenderDevice::max_usable_sample_count() const {
    const auto props = gvPhysicalDevice.getProperties();
    const auto counts = props.limits.framebufferColorSampleCounts &
                        props.limits.framebufferDepthSampleCounts;

    if (counts & vk::SampleCountFlagBits::e64) { return vk::SampleCountFlagBits::e64; }
    if (counts & vk::SampleCountFlagBits::e32) { return vk::SampleCountFlagBits::e32; }
    if (counts & vk::SampleCountFlagBits::e16) { return vk::SampleCountFlagBits::e16; }
    if (counts & vk::SampleCountFlagBits::e8) { return vk::SampleCountFlagBits::e8; }
    if (counts & vk::SampleCountFlagBits::e4) { return vk::SampleCountFlagBits::e4; }
    if (counts & vk::SampleCountFlagBits::e2) { return vk::SampleCountFlagBits::e2; }

    return vk::SampleCountFlagBits::e1;
}
}  // namespace garnish
