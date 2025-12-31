#include "vulkan_renderer.hpp"

#include <SDL3/SDL_events.h>
#include <SDL3/SDL_vulkan.h>
#include <stb_image.h>
#include <tiny_obj_loader.h>

#include <algorithm>
#include <cstddef>
#include <format>
#include <iostream>
#include <ranges>
#include <set>
#include <span>
#include <sstream>
#include <stdexcept>
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_enums.hpp>
#include <vulkan/vulkan_structs.hpp>

#include "Utility/camera.hpp"
#include "Utility/log.hpp"
#include "Utility/read_file.hpp"
#include "ecs_controller.h"
#include "geometry.hpp"
#include "vulkan/vulkan_raii.hpp"

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE;  // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

namespace {
VKAPI_ATTR vk::Bool32 VKAPI_CALL debugMessageFunc(
    vk::DebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    vk::DebugUtilsMessageTypeFlagsEXT messageTypes,
    const vk::DebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* /*pUserData*/
) {
    std::ostringstream message;
    message << vk::to_string(messageSeverity) << ": " << vk::to_string(messageTypes) << ":\n";
    message << "\tmessageIDName   = <" << pCallbackData->pMessageIdName << ">\n";
    message << "\tmessageIdNumber = " << static_cast<uint32_t>(pCallbackData->messageIdNumber)
            << "\n";
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
        message << "\tObject " << objIndex++ << " type=" << vk::to_string(obj.objectType)
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
    auto it = std::ranges::find_if(formats, [](const auto& f) {
        return f.format == vk::Format::eB8G8R8A8Srgb &&
               f.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear;
    });
    return it != formats.end() ? *it : formats.front();
}
}  // anonymous namespace

namespace garnish::vulkan {

// Constructor
VulkanRenderDevice::VulkanRenderDevice(const RenderDevice::InitInfo& info)
    : RenderDevice(static_cast<SDL_Window*>(info.nativeWindow)),
      gvInstance_(create_instance()),
      gvSurface_(create_surface()),
      gvDebugMessenger_(setup_debug_messenger()),
      gvPhysicalDevice_(pick_physical_device()),
      gvDevice_(create_logical_device()),
      modelBufferAlloc_(create_model_buffers(kInitialModelCapacity)),
      gvGraphicsQueue_(gvDevice_.getQueue(0, 0)),
      gvPresentQueue_(gvDevice_.getQueue(0, 0)),
      gvSwapChainExtent_(create_extent()),
      gvSwapChainImageFormat_(create_swap_chain_image_format()),
      gvSwapchainKHR_(create_swap_chain()),
      swapChainImageViews_(create_image_views()),
      gvTextureSampler_(create_texture_sampler()),
      gvRenderPass_(create_render_pass()),
      gvDescriptorSetLayout_(create_descriptor_set_layout()),
      gvPipelineLayout_(create_pipeline_layout()),
      gvPipeline_(create_graphics_pipeline(info.assetPath)),
      gvCommandPool_(create_command_pool()),
      colorResources_(create_color_resources()),
      depthResources_(create_depth_resources()),
      swapChainFramebuffers_(create_framebuffers()),
      vertexBuffer_(create_vertex_buffer()),
      indexBuffer_(create_index_buffer()),
      uniformBufferAlloc_(create_uniform_buffers()),
      gvDescriptorPool_(create_descriptor_pool()),
      descriptorSets_(create_descriptor_sets()),
      gvCommandBuffers_(create_command_buffers()),
      imageAvailableSemaphores_(create_sync_objects()) {
    load_texture(info.assetPath + "Textures/viking_room.png");
    setup_mesh(info.assetPath + "Models/viking_room.obj");
}

// Public overrides - Lifecycle
void VulkanRenderDevice::cleanup() {
    if (!(*gvDevice_)) return;
    gvDevice_.waitIdle();

    cleanup_swap_chain();

    for (size_t i = 0; i < uniformBufferAlloc_.mapped.size(); ++i) {
        if (uniformBufferAlloc_.mapped[i]) {
            uniformBufferAlloc_.memory[i].unmapMemory();
            uniformBufferAlloc_.mapped[i] = nullptr;
        }
    }

    for (size_t i = 0; i < modelBufferAlloc_.mapped.size(); ++i) {
        if (modelBufferAlloc_.mapped[i]) {
            modelBufferAlloc_.memory[i].unmapMemory();
            modelBufferAlloc_.mapped[i] = nullptr;
        }
    }

    gvTextures_.clear();
    uniformBufferAlloc_.buffers.clear();
    uniformBufferAlloc_.memory.clear();
    modelBufferAlloc_.buffers.clear();
    modelBufferAlloc_.memory.clear();
    descriptorSets_.clear();
    imageAvailableSemaphores_.clear();
    renderFinishedSemaphores_.clear();
    inFlightFences_.clear();
    imageInFlight_.clear();
    swapChainFramebuffers_.clear();
    swapChainImageViews_.clear();
    gvCommandBuffers_.clear();
}

// Public overrides - Core rendering
bool VulkanRenderDevice::draw_frame(ECSController& world) {
    (void)gvDevice_.waitForFences(*inFlightFences_[currentFrame_], vk::True, UINT64_MAX);

    auto [aquireResult, imageIndex] = gvSwapchainKHR_.acquireNextImage(
        UINT64_MAX,
        *imageAvailableSemaphores_[currentFrame_],
        VK_NULL_HANDLE
    );

    if (aquireResult == vk::Result::eErrorOutOfDateKHR) {
        recreate_swap_chain();
        return false;
    }
    if (aquireResult != vk::Result::eSuccess && aquireResult != vk::Result::eSuboptimalKHR) {
        throw std::runtime_error("failed to acquire swap chain image!");
    }

    if (imageInFlight_[imageIndex] != vk::Fence{}) {
        (void)gvDevice_.waitForFences(imageInFlight_[imageIndex], vk::True, UINT64_MAX);
    }
    imageInFlight_[imageIndex] = *inFlightFences_[currentFrame_];

    // Only reset the fence if we are submitting work
    gvDevice_.resetFences(*inFlightFences_[currentFrame_]);
    gvCommandBuffers_[currentFrame_].reset();

    record_command_buffer(gvCommandBuffers_[currentFrame_], imageIndex, world);
    update_camera_buffer(currentFrame_);
    const std::array<vk::Semaphore, 1> waitSemaphores{*imageAvailableSemaphores_[currentFrame_]};
    const std::array<vk::PipelineStageFlags, 1> waitStages{
        vk::PipelineStageFlagBits::eColorAttachmentOutput
    };
    const std::array<vk::Semaphore, 1> signalSemaphores{*renderFinishedSemaphores_[imageIndex]};

    vk::CommandBuffer rawCmd = *gvCommandBuffers_[currentFrame_];
    auto submitInfo = vk::SubmitInfo{}
                          .setWaitSemaphores(waitSemaphores)
                          .setPWaitDstStageMask(waitStages.data())
                          .setCommandBuffers(rawCmd)
                          .setSignalSemaphores(signalSemaphores);

    gvGraphicsQueue_.submit(submitInfo, *inFlightFences_[currentFrame_]);
    const std::array<vk::SwapchainKHR, 1> swapChains{*gvSwapchainKHR_};

    auto presentInfo = vk::PresentInfoKHR{}
                           .setWaitSemaphoreCount(static_cast<uint32_t>(signalSemaphores.size()))
                           .setPWaitSemaphores(signalSemaphores.data())
                           .setSwapchainCount(static_cast<uint32_t>(swapChains.size()))
                           .setPSwapchains(swapChains.data())
                           .setPImageIndices(&imageIndex);

    auto presentResult = gvPresentQueue_.presentKHR(presentInfo);

    if (presentResult == vk::Result::eErrorOutOfDateKHR ||
        presentResult == vk::Result::eSuboptimalKHR) {
        framebufferResized_ = false;
        recreate_swap_chain();
    } else if (presentResult != vk::Result::eSuccess) {
        throw std::runtime_error("update_descriptor_setsfailed to present swap chain image!");
    }

    currentFrame_ = (currentFrame_ + 1) % kMAX_FRAMES_IN_FLIGHT;
    return true;
}

void VulkanRenderDevice::update(ECSController& world) {
    draw_frame(world);
}

// Public overrides - Resource loading
uint32_t VulkanRenderDevice::setup_mesh(const Geometry& geometry) {
    // TODO: decide if geometry should consumed or copied, if consumed
    // should std::move the vectors;
    std::vector<uint32_t> indices = geometry.indices;
    std::vector<Vertex> vertices = geometry.vertices;
    std::unordered_map<Vertex, uint32_t> uniqueVertices{};

    gvMeshes_.push_back(
        GVMesh{
            .firstVertex = static_cast<uint32_t>(totalVertexBytes_ / sizeof(Vertex)),
            .vertexCount = static_cast<uint32_t>(vertices.size()),
            .firstIndex = static_cast<uint32_t>(totalIndexBytes_ / sizeof(uint32_t)),
            .indexCount = static_cast<uint32_t>(indices.size())
        }
    );

    auto [vertexStagingBuffer, vertexStagingMem] = create_staging_buffer(std::span{vertices});
    auto [indexStagingBuffer, indexStagingMem] = create_staging_buffer(std::span{indices});

    copy_buffer(
        {.src = *vertexStagingBuffer,
         .dst = *vertexBuffer_.buffer,
         .size = sizeof(Vertex) * vertices.size(),
         .dstOffset = totalVertexBytes_}
    );
    copy_buffer(
        {.src = *indexStagingBuffer,
         .dst = *indexBuffer_.buffer,
         .size = sizeof(uint32_t) * indices.size(),
         .dstOffset = totalIndexBytes_}
    );

    totalVertexBytes_ += sizeof(Vertex) * vertices.size();
    totalIndexBytes_ += sizeof(uint32_t) * indices.size();
    return gvMeshes_.size() - 1;
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
    stbi_uc* pixels = stbi_load(path.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
    if (!pixels) {
        throw std::runtime_error("failed to load texture image!");
    }
    auto imageSize = static_cast<vk::DeviceSize>(
        static_cast<uint64_t>(texWidth) * static_cast<uint64_t>(texHeight) * 4U
    );

    uint32_t mipLevels = static_cast<uint32_t>(
                             std::floor(std::log2(std::max(texWidth, texHeight)))
                         ) +
                         1;

    auto [stagingBuffer, stagingMem] = create_staging_buffer(
        std::span{pixels, static_cast<size_t>(imageSize)}
    );
    stbi_image_free(pixels);

    auto [textureImage, textureImageMemory] = create_image(
        {.width = static_cast<uint32_t>(texWidth),
         .height = static_cast<uint32_t>(texHeight),
         .mipLevels = mipLevels,
         .samples = vk::SampleCountFlagBits::e1,
         .format = vk::Format::eR8G8B8A8Srgb,
         .tiling = vk::ImageTiling::eOptimal,
         .usage = vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eTransferDst |
                  vk::ImageUsageFlagBits::eSampled,
         .memoryProperties = vk::MemoryPropertyFlagBits::eDeviceLocal}
    );

    transition_image_layout(
        {.image = *textureImage,
         .oldLayout = vk::ImageLayout::eUndefined,
         .newLayout = vk::ImageLayout::eTransferDstOptimal,
         .mipLevels = mipLevels}
    );

    copy_buffer_to_image(
        {.buffer = *stagingBuffer,
         .image = *textureImage,
         .width = static_cast<uint32_t>(texWidth),
         .height = static_cast<uint32_t>(texHeight)}
    );

    generate_mipmaps(
        {.image = *textureImage,
         .format = vk::Format::eR8G8B8A8Srgb,
         .size = {.width = texWidth, .height = texHeight},
         .mipLevels = mipLevels}
    );

    vkr::ImageView textureImageView = create_image_view(
        {.image = *textureImage,
         .format = vk::Format::eR8G8B8A8Srgb,
         .aspectFlags = vk::ImageAspectFlagBits::eColor,
         .mipLevels = mipLevels}
    );

    gvTextures_.push_back(
        GVTexture{
            .mipLevels = mipLevels,
            .textureImage = std::move(textureImage),
            .textureMemory = std::move(textureImageMemory),
            .textureImageView = std::move(textureImageView),
        }
    );

    update_descriptor_sets();
    return gvTextures_.size() - 1;
}

// Instance and surface initialization
vkr::Instance VulkanRenderDevice::create_instance() {
    vk::detail::DynamicLoader dl;
    auto vkGetInstanceProcAddr = dl.getProcAddress<PFN_vkGetInstanceProcAddr>(
        "vkGetInstanceProcAddr"
    );
    VULKAN_HPP_DEFAULT_DISPATCHER.init(vkGetInstanceProcAddr);

    uint32_t sdlExtensionCount = 0;
    const char* const* sdlExtensions = SDL_Vulkan_GetInstanceExtensions(&sdlExtensionCount);

    std::span<const char* const> sdlExtSpan{sdlExtensions, sdlExtensionCount};
    std::vector<const char*> extensions(sdlExtSpan.begin(), sdlExtSpan.end());
    extensions.push_back(vk::KHRGetPhysicalDeviceProperties2ExtensionName);

    auto instExts = vk::enumerateInstanceExtensionProperties();
    for (auto& ext : instExts) {
        if (strcmp(ext.extensionName, vk::KHRPortabilityEnumerationExtensionName) == 0) {
            extensions.push_back(vk::KHRPortabilityEnumerationExtensionName);
        }
    }

    if (kEnableValidationLayers) {
        extensions.push_back(vk::EXTDebugUtilsExtensionName);
    }

    auto applicationInfo = vk::ApplicationInfo{}
                               .setPApplicationName("Garnish")
                               .setApplicationVersion(vk::makeApiVersion(0, 1, 1, 0))
                               .setPEngineName("GarnishEngine")
                               .setEngineVersion(vk::makeApiVersion(0, 1, 1, 0))
                               .setApiVersion(VK_API_VERSION_1_2);

    auto instanceCreateInfo = vk::InstanceCreateInfo{}
                                  .setFlags(vk::InstanceCreateFlagBits::eEnumeratePortabilityKHR)
                                  .setPApplicationInfo(&applicationInfo)
                                  .setPEnabledLayerNames(validationLayers)
                                  .setPEnabledExtensionNames(extensions);

    bool haveValidationLayer = std::ranges::any_of(
        vk::enumerateInstanceLayerProperties(),
        [](auto const& lp) { return strcmp(lp.layerName, "VK_LAYER_KHRONOS_validation") == 0; }
    );
    if (!haveValidationLayer) {
        throw std::runtime_error("missing one or more needed validation layers");
    }

    vkr::Instance instance = vkr::Instance(gvContext_, instanceCreateInfo);
    VULKAN_HPP_DEFAULT_DISPATCHER.init(*instance);
    return std::move(instance);
}

vkr::SurfaceKHR VulkanRenderDevice::create_surface() {
    VkSurfaceKHR khr = nullptr;
    if (!SDL_Vulkan_CreateSurface(window, *gvInstance_, nullptr, &khr)) {
        throw std::runtime_error("failed to create window surface!");
    }
    return vkr::SurfaceKHR(gvInstance_, khr);
}

vkr::DebugUtilsMessengerEXT VulkanRenderDevice::setup_debug_messenger() {
    if (!kEnableValidationLayers) return vkr::DebugUtilsMessengerEXT{nullptr};
    auto createInfo = vk::DebugUtilsMessengerCreateInfoEXT{}
                          .setMessageSeverity(
                              vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose |
                              vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
                              vk::DebugUtilsMessageSeverityFlagBitsEXT::eError
                          )
                          .setMessageType(
                              vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
                              vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation |
                              vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance
                          )
                          .setPfnUserCallback(&debugMessageFunc);
    return {gvInstance_, createInfo};
}

// Physical and logical device selection
vkr::PhysicalDevice VulkanRenderDevice::pick_physical_device() {
    std::vector physicalDevices = gvInstance_.enumeratePhysicalDevices();
    if (physicalDevices.empty()) {
        throw std::runtime_error("no physical devices for vulkan");
    }

    auto it = std::ranges::find_if(physicalDevices, [this](auto& device) {
        return is_device_suitable(device);
    });
    if (it == physicalDevices.end()) {
        throw std::runtime_error("no suitable physical device");
    }

    vkr::PhysicalDevice physicalDevice = *it;
    msaaSamples_ = max_usable_sample_count();

    auto [props2, indexingPropsQuery] = physicalDevice.getProperties2<
        vk::PhysicalDeviceProperties2,
        vk::PhysicalDeviceDescriptorIndexingProperties>();
    textureLimit_ = props2.properties.limits.maxPerStageDescriptorSampledImages;
    indexingProperties_ = indexingPropsQuery;

    auto [features2, vulkan12FeaturesQuery] = physicalDevice.getFeatures2<
        vk::PhysicalDeviceFeatures2,
        vk::PhysicalDeviceVulkan12Features>();
    vulkan12Features_ = vulkan12FeaturesQuery;

    auto [features2b, indexingFeaturesQuery] = physicalDevice.getFeatures2<
        vk::PhysicalDeviceFeatures2,
        vk::PhysicalDeviceDescriptorIndexingFeatures>();
    supportedIndexingFeatures_ = indexingFeaturesQuery;

    log_debug(
        std::format(
            "Vulkan 1.2 descriptorIndexing: {}\n"
            "Descriptor indexing support:\n"
            "  runtimeDescriptorArray: {}\n"
            "  descriptorBindingPartiallyBound: {}\n"
            "  descriptorBindingVariableDescriptorCount: {}\n"
            "  shaderSampledImageArrayNonUniformIndexing: {}\n"
            "Descriptor indexing properties (limits):\n"
            "  maxPerStageDescriptorUpdateAfterBindSamplers: {}\n"
            "  maxPerStageDescriptorUpdateAfterBindUniformBuffers: {}\n"
            "  maxPerStageDescriptorUpdateAfterBindStorageBuffers: {}\n"
            "  maxPerStageDescriptorUpdateAfterBindSampledImages: {}",
            vulkan12Features_.descriptorIndexing,
            supportedIndexingFeatures_.runtimeDescriptorArray,
            supportedIndexingFeatures_.descriptorBindingPartiallyBound,
            supportedIndexingFeatures_.descriptorBindingVariableDescriptorCount,
            supportedIndexingFeatures_.shaderSampledImageArrayNonUniformIndexing,
            indexingProperties_.maxPerStageDescriptorUpdateAfterBindSamplers,
            indexingProperties_.maxPerStageDescriptorUpdateAfterBindUniformBuffers,
            indexingProperties_.maxPerStageDescriptorUpdateAfterBindStorageBuffers,
            indexingProperties_.maxPerStageDescriptorUpdateAfterBindSampledImages
        )
    );

    std::vector<vk::ExtensionProperties>
        availableExtensions = physicalDevice.enumerateDeviceExtensionProperties();

    if (std::ranges::any_of(availableExtensions, [](const auto& ext) {
            return strcmp(ext.extensionName, vk::KHRMaintenance3ExtensionName) == 0;
        })) {
        auto [_, maintenance3Props] = physicalDevice.getProperties2<
            vk::PhysicalDeviceProperties2,
            vk::PhysicalDeviceMaintenance3Properties>();
        log_timed(
            std::format(
                "maintenance3Props.maxMemoryAllocationSize: {}",
                maintenance3Props.maxMemoryAllocationSize
            )
        );
        deviceExtensions_.push_back(vk::KHRMaintenance3ExtensionName);
    }

    if (std::ranges::any_of(availableExtensions, [](const auto& ext) {
            return strcmp(ext.extensionName, "VK_KHR_portability_subset") == 0;
        })) {
        deviceExtensions_.push_back("VK_KHR_portability_subset");
    }

    return physicalDevice;
}

vkr::Device VulkanRenderDevice::create_logical_device() {
    QueueFamilyIndices indices = find_queue_families(gvPhysicalDevice_);

    std::set<uint32_t> uniqueQueueFamilies = {
        indices.graphicsFamily.value(),
        indices.presentFamily.value()
    };

    float queuePriority = 1.0F;
    auto queueCreateInfos = uniqueQueueFamilies |
                            std::views::transform([&queuePriority](uint32_t queueFamily) {
                                return vk::DeviceQueueCreateInfo{}
                                    .setQueueFamilyIndex(queueFamily)
                                    .setQueueCount(1)
                                    .setPQueuePriorities(&queuePriority);
                            }) |
                            std::ranges::to<std::vector>();
    auto deviceFeatures = vk::PhysicalDeviceFeatures{}
                              .setSamplerAnisotropy(vk::True)
                              .setSampleRateShading(vk::True);

    auto vulkan12FeaturesEnable = vk::PhysicalDeviceVulkan12Features{}
                                      .setDescriptorIndexing(vulkan12Features_.descriptorIndexing)
                                      .setRuntimeDescriptorArray(
                                          supportedIndexingFeatures_.runtimeDescriptorArray
                                      )
                                      .setDescriptorBindingPartiallyBound(
                                          supportedIndexingFeatures_.descriptorBindingPartiallyBound
                                      )
                                      .setDescriptorBindingVariableDescriptorCount(
                                          supportedIndexingFeatures_
                                              .descriptorBindingVariableDescriptorCount
                                      )
                                      .setShaderSampledImageArrayNonUniformIndexing(
                                          supportedIndexingFeatures_
                                              .shaderSampledImageArrayNonUniformIndexing
                                      );

    auto createInfo = vk::DeviceCreateInfo{}
                          .setQueueCreateInfos(queueCreateInfos)
                          .setPEnabledExtensionNames(deviceExtensions_)
                          .setPEnabledFeatures(&deviceFeatures)
                          .setPNext(&vulkan12FeaturesEnable);

    vkr::Device device = vkr::Device(gvPhysicalDevice_, createInfo);
    // initalize these properly
    gvGraphicsQueue_ = device.getQueue(indices.graphicsFamily.value(), 0);
    gvPresentQueue_ = device.getQueue(indices.presentFamily.value(), 0);

    VULKAN_HPP_DEFAULT_DISPATCHER.init(*device);
    return device;
}

bool VulkanRenderDevice::is_device_suitable(const vkr::PhysicalDevice& device) {
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

bool VulkanRenderDevice::check_device_extension_support(const vkr::PhysicalDevice& device) {
    const auto availableExtensions = device.enumerateDeviceExtensionProperties();

    std::set<std::string> requiredExtensions(deviceExtensions_.begin(), deviceExtensions_.end());

    for (const auto& extension : availableExtensions) {
        requiredExtensions.erase(extension.extensionName);
    }

    return requiredExtensions.empty();
}

VulkanRenderDevice::QueueFamilyIndices VulkanRenderDevice::find_queue_families(
    const vkr::PhysicalDevice& device
) {
    QueueFamilyIndices indices;
    auto queueFamilies = device.getQueueFamilyProperties();

    for (int i = 0; const auto& queueFamily : queueFamilies) {
        if (queueFamily.queueFlags & vk::QueueFlagBits::eGraphics) {
            indices.graphicsFamily = i;
        }
        if (device.getSurfaceSupportKHR(i, gvSurface_)) {
            indices.presentFamily = i;
        }
        if (indices.isComplete()) {
            break;
        }
        ++i;
    }
    return indices;
}

vk::SampleCountFlagBits VulkanRenderDevice::max_usable_sample_count() const {
    const auto props = gvPhysicalDevice_.getProperties();
    const auto counts = props.limits.framebufferColorSampleCounts &
                        props.limits.framebufferDepthSampleCounts;

    // unused but could later check
    const vk::FormatProperties colorFormatProps = gvPhysicalDevice_.getFormatProperties(
        vk::Format::eB8G8R8A8Unorm
    );
    const vk::FormatProperties depthFormatProps = gvPhysicalDevice_.getFormatProperties(
        vk::Format::eD32Sfloat
    );

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

// Swap chain creation and management
vk::Extent2D VulkanRenderDevice::create_extent() {
    int width = 0;
    int height = 0;
    SDL_GetWindowSize(window, &width, &height);
    return vk::Extent2D{static_cast<uint32_t>(width), static_cast<uint32_t>(height)};
}

vk::Format VulkanRenderDevice::create_swap_chain_image_format() const {
    auto support = query_swap_chain_support(*gvPhysicalDevice_);
    return choose_surface_format(support.formats).format;
}

vkr::SwapchainKHR VulkanRenderDevice::create_swap_chain() {
    auto support = query_swap_chain_support(*gvPhysicalDevice_);
    auto surfaceFormat = choose_surface_format(support.formats);
    auto presentMode = choose_present_mode(support.presentModes);
    auto indices = find_queue_families(gvPhysicalDevice_);

    auto createInfo = build_swapchain_create_info(support, surfaceFormat, presentMode, indices);

    vkr::SwapchainKHR swapchain(gvDevice_, createInfo);
    swapChainImages_ = swapchain.getImages();
    return swapchain;
}

VulkanRenderDevice::SwapChainSupportDetails VulkanRenderDevice::query_swap_chain_support(
    const vk::PhysicalDevice& device
) const {
    return SwapChainSupportDetails{
        .capabilities = device.getSurfaceCapabilitiesKHR(*gvSurface_),
        .formats = device.getSurfaceFormatsKHR(*gvSurface_),
        .presentModes = device.getSurfacePresentModesKHR(*gvSurface_)
    };
}

vk::PresentModeKHR VulkanRenderDevice::choose_present_mode(
    const std::vector<vk::PresentModeKHR>& availableModes
) {
    return std::ranges::contains(availableModes, vk::PresentModeKHR::eMailbox)
               ? vk::PresentModeKHR::eMailbox
               : vk::PresentModeKHR::eFifo;
}

uint32_t VulkanRenderDevice::choose_image_count(const vk::SurfaceCapabilitiesKHR& capabilities) {
    uint32_t imageCount = capabilities.minImageCount + 1;
    if (capabilities.maxImageCount > 0 && imageCount > capabilities.maxImageCount) {
        imageCount = capabilities.maxImageCount;
    }
    return imageCount;
}

vk::SwapchainCreateInfoKHR VulkanRenderDevice::build_swapchain_create_info(
    const SwapChainSupportDetails& support,
    const vk::SurfaceFormatKHR& surfaceFormat,
    const vk::PresentModeKHR& presentMode,
    const QueueFamilyIndices& indices
) const {
    uint32_t imageCount = choose_image_count(support.capabilities);
    bool sameFamily = indices.graphicsFamily == indices.presentFamily;
    std::array queueFamilyIndices{indices.graphicsFamily.value(), indices.presentFamily.value()};

    return vk::SwapchainCreateInfoKHR{}
        .setSurface(*gvSurface_)
        .setMinImageCount(imageCount)
        .setImageFormat(surfaceFormat.format)
        .setImageColorSpace(surfaceFormat.colorSpace)
        .setImageExtent(gvSwapChainExtent_)
        .setImageArrayLayers(1)
        .setImageUsage(vk::ImageUsageFlagBits::eColorAttachment)
        .setImageSharingMode(
            sameFamily ? vk::SharingMode::eExclusive : vk::SharingMode::eConcurrent
        )
        .setQueueFamilyIndexCount(sameFamily ? 0 : 2)
        .setPQueueFamilyIndices(sameFamily ? nullptr : queueFamilyIndices.data())
        .setPreTransform(support.capabilities.currentTransform)
        .setCompositeAlpha(vk::CompositeAlphaFlagBitsKHR::eOpaque)
        .setPresentMode(presentMode)
        .setClipped(vk::True);
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
    imageInFlight_.clear();

    gvDevice_.waitIdle();

    cleanup_swap_chain();

    gvSwapChainExtent_ = create_extent();
    gvSwapchainKHR_ = create_swap_chain();
    swapChainImageViews_ = create_image_views();
    colorResources_ = create_color_resources();
    depthResources_ = create_depth_resources();
    swapChainFramebuffers_ = create_framebuffers();

    const auto newImageCount = static_cast<uint32_t>(swapChainImages_.size());
    renderFinishedSemaphores_ = std::views::iota(0U, newImageCount) |
                                std::views::transform([this](auto) {
                                    return gvDevice_.createSemaphore({});
                                }) |
                                std::ranges::to<std::vector>();

    imageInFlight_.assign(newImageCount, VK_NULL_HANDLE);

    return true;
}

bool VulkanRenderDevice::cleanup_swap_chain() {
    colorResources_.view.clear();
    colorResources_.image.clear();
    colorResources_.memory.clear();

    depthResources_.view.clear();
    depthResources_.image.clear();
    depthResources_.memory.clear();

    swapChainFramebuffers_.clear();
    swapChainImageViews_.clear();
    renderFinishedSemaphores_.clear();

    gvSwapchainKHR_.clear();
    return true;
}

// Image views
std::vector<vkr::ImageView> VulkanRenderDevice::create_image_views() {
    return swapChainImages_ | std::views::transform([this](const auto& image) {
               return create_image_view(
                   {.image = image,
                    .format = gvSwapChainImageFormat_,
                    .aspectFlags = vk::ImageAspectFlagBits::eColor,
                    .mipLevels = 1}
               );
           }) |
           std::ranges::to<std::vector>();
}

vkr::ImageView VulkanRenderDevice::create_image_view(const ImageViewCreateParams& params) {
    auto viewInfo = vk::ImageViewCreateInfo{}
                        .setImage(params.image)
                        .setViewType(vk::ImageViewType::e2D)
                        .setFormat(params.format)
                        .setSubresourceRange(
                            vk::ImageSubresourceRange{}
                                .setAspectMask(params.aspectFlags)
                                .setBaseMipLevel(0)
                                .setLevelCount(params.mipLevels)
                                .setBaseArrayLayer(0)
                                .setLayerCount(1)
                        );

    return gvDevice_.createImageView(viewInfo);
}

// Texture sampler
vkr::Sampler VulkanRenderDevice::create_texture_sampler() {
    vk::PhysicalDeviceProperties properties = gvPhysicalDevice_.getProperties();

    return gvDevice_.createSampler(
        vk::SamplerCreateInfo{}
            .setMagFilter(vk::Filter::eLinear)
            .setMinFilter(vk::Filter::eLinear)
            .setMipmapMode(vk::SamplerMipmapMode::eLinear)
            .setAddressModeU(vk::SamplerAddressMode::eRepeat)
            .setAddressModeV(vk::SamplerAddressMode::eRepeat)
            .setAddressModeW(vk::SamplerAddressMode::eRepeat)
            .setMipLodBias(0.0F)
            .setAnisotropyEnable(vk::True)
            .setMaxAnisotropy(properties.limits.maxSamplerAnisotropy)
            .setCompareEnable(vk::False)
            .setCompareOp(vk::CompareOp::eAlways)
            .setMinLod(0.0F)
            .setMaxLod(VK_LOD_CLAMP_NONE)
            .setBorderColor(vk::BorderColor::eIntOpaqueBlack)
    );
}

// Note: This function appears to be unused but keeping it as it's in the header
// vkr::ImageView VulkanRenderDevice::create_texture_image_view() {
//     return vkr::ImageView{nullptr};
// }

// Render pass
vkr::RenderPass VulkanRenderDevice::create_render_pass() {
    auto colorAttachRef = vk::AttachmentReference{}.setAttachment(0).setLayout(
        vk::ImageLayout::eColorAttachmentOptimal
    );

    auto colorAttachResolveRef = vk::AttachmentReference{}.setAttachment(2).setLayout(
        vk::ImageLayout::eColorAttachmentOptimal
    );

    auto depthAttachRef = vk::AttachmentReference{}.setAttachment(1).setLayout(
        vk::ImageLayout::eDepthStencilAttachmentOptimal
    );

    auto subpass = vk::SubpassDescription{}
                       .setPipelineBindPoint(vk::PipelineBindPoint::eGraphics)
                       .setColorAttachmentCount(1)
                       .setPColorAttachments(&colorAttachRef)
                       .setPResolveAttachments(&colorAttachResolveRef)
                       .setPDepthStencilAttachment(&depthAttachRef);

    auto dependency = vk::SubpassDependency{}
                          .setSrcSubpass(vk::SubpassExternal)
                          .setDstSubpass(0)
                          .setSrcStageMask(
                              vk::PipelineStageFlagBits::eColorAttachmentOutput |
                              vk::PipelineStageFlagBits::eLateFragmentTests
                          )
                          .setDstStageMask(
                              vk::PipelineStageFlagBits::eColorAttachmentOutput |
                              vk::PipelineStageFlagBits::eEarlyFragmentTests
                          )
                          .setSrcAccessMask(
                              vk::AccessFlagBits::eColorAttachmentWrite |
                              vk::AccessFlagBits::eDepthStencilAttachmentWrite
                          )
                          .setDstAccessMask(
                              vk::AccessFlagBits::eColorAttachmentWrite |
                              vk::AccessFlagBits::eDepthStencilAttachmentWrite
                          );

    std::array attachments = {
        vk::AttachmentDescription{}
            .setFormat(gvSwapChainImageFormat_)
            .setSamples(msaaSamples_)
            .setLoadOp(vk::AttachmentLoadOp::eClear)
            .setStoreOp(vk::AttachmentStoreOp::eStore)
            .setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
            .setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
            .setInitialLayout(vk::ImageLayout::eUndefined)
            .setFinalLayout(vk::ImageLayout::eColorAttachmentOptimal),
        vk::AttachmentDescription{}
            .setFormat(find_depth_format())
            .setSamples(msaaSamples_)
            .setLoadOp(vk::AttachmentLoadOp::eClear)
            .setStoreOp(vk::AttachmentStoreOp::eDontCare)
            .setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
            .setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
            .setInitialLayout(vk::ImageLayout::eUndefined)
            .setFinalLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal),
        vk::AttachmentDescription{}
            .setFormat(gvSwapChainImageFormat_)
            .setSamples(vk::SampleCountFlagBits::e1)
            .setLoadOp(vk::AttachmentLoadOp::eDontCare)
            .setStoreOp(vk::AttachmentStoreOp::eStore)
            .setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
            .setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
            .setInitialLayout(vk::ImageLayout::eUndefined)
            .setFinalLayout(vk::ImageLayout::ePresentSrcKHR),
    };

    auto renderPassInfo = vk::RenderPassCreateInfo{}
                              .setAttachments(attachments)
                              .setSubpassCount(1)
                              .setPSubpasses(&subpass)
                              .setDependencyCount(1)
                              .setPDependencies(&dependency);

    return gvDevice_.createRenderPass(renderPassInfo);
}

vk::Format VulkanRenderDevice::find_depth_format() {
    return find_supported_format(
        {vk::Format::eD32Sfloat, vk::Format::eD32SfloatS8Uint, vk::Format::eD24UnormS8Uint},
        vk::ImageTiling::eOptimal,
        vk::FormatFeatureFlagBits::eDepthStencilAttachment
    );
}

vk::Format VulkanRenderDevice::find_supported_format(
    const std::vector<vk::Format>& candidates,
    const vk::ImageTiling tiling,
    const vk::FormatFeatureFlags features
) {
    auto it = std::ranges::find_if(candidates, [&](const vk::Format& format) {
        vk::FormatProperties props = gvPhysicalDevice_.getFormatProperties(format);
        auto tilingFeatures = (tiling == vk::ImageTiling::eLinear) ? props.linearTilingFeatures
                                                                   : props.optimalTilingFeatures;
        return (tilingFeatures & features) == features;
    });

    if (it == candidates.end()) {
        throw std::runtime_error("failed to find supported format!");
    }
    return *it;
}

// Descriptor set layout and pipeline
vkr::DescriptorSetLayout VulkanRenderDevice::create_descriptor_set_layout() {
    auto camBinding = vk::DescriptorSetLayoutBinding{}
                          .setBinding(0)
                          .setDescriptorType(vk::DescriptorType::eUniformBuffer)
                          .setDescriptorCount(1)
                          .setStageFlags(vk::ShaderStageFlagBits::eVertex);

    auto modelBinding = vk::DescriptorSetLayoutBinding{}
                            .setBinding(1)
                            .setDescriptorType(vk::DescriptorType::eStorageBuffer)
                            .setDescriptorCount(1)
                            .setStageFlags(vk::ShaderStageFlagBits::eVertex);

    auto sampBinding = vk::DescriptorSetLayoutBinding{}
                           .setBinding(2)
                           .setDescriptorType(vk::DescriptorType::eSampler)
                           .setDescriptorCount(1)
                           .setStageFlags(vk::ShaderStageFlagBits::eFragment)
                           .setPImmutableSamplers(&(*gvTextureSampler_));

    auto imgBinding = vk::DescriptorSetLayoutBinding{}
                          .setBinding(3)
                          .setDescriptorType(vk::DescriptorType::eSampledImage)
                          .setDescriptorCount(textureLimit_)
                          .setStageFlags(vk::ShaderStageFlagBits::eFragment);
    std::array bindings{camBinding, modelBinding, sampBinding, imgBinding};

    vk::DescriptorBindingFlags textureBindingFlags{};
    if (supportedIndexingFeatures_.descriptorBindingPartiallyBound) {
        textureBindingFlags |= vk::DescriptorBindingFlagBits::ePartiallyBound;
    }
    if (supportedIndexingFeatures_.descriptorBindingVariableDescriptorCount) {
        textureBindingFlags |= vk::DescriptorBindingFlagBits::eVariableDescriptorCount;
    }

    std::array bindingFlags{
        vk::DescriptorBindingFlags{},
        vk::DescriptorBindingFlags{},
        vk::DescriptorBindingFlags{},
        textureBindingFlags
    };
    auto flagsInfo = vk::DescriptorSetLayoutBindingFlagsCreateInfo{}.setBindingFlags(bindingFlags);

    auto layoutInfo = vk::DescriptorSetLayoutCreateInfo{}.setBindings(bindings).setPNext(
        &flagsInfo
    );
    return vkr::DescriptorSetLayout(gvDevice_, layoutInfo);
}

vkr::PipelineLayout VulkanRenderDevice::create_pipeline_layout() {
    struct PC {
        uint32_t texIndex;
        uint32_t modelIndex;
    };

    auto pcRange = vk::PushConstantRange{}
                       .setStageFlags(
                           vk::ShaderStageFlagBits::eFragment | vk::ShaderStageFlagBits::eVertex
                       )
                       .setOffset(0)
                       .setSize(sizeof(PC));

    auto pipelineLayoutInfo = vk::PipelineLayoutCreateInfo{}
                                  .setSetLayoutCount(1)
                                  .setPSetLayouts(&(*gvDescriptorSetLayout_))
                                  .setPushConstantRangeCount(1)
                                  .setPPushConstantRanges(&pcRange);

    return gvDevice_.createPipelineLayout(pipelineLayoutInfo);
}

vkr::Pipeline VulkanRenderDevice::create_graphics_pipeline(const std::string& assetPath) {
    auto vertShaderCode = read_file(assetPath + std::string(kVertexShaderPath));
    auto fragShaderCode = read_file(assetPath + std::string(kFragmentShaderPath));
    vkr::ShaderModule vertShaderModule = create_shader_module(vertShaderCode);
    vkr::ShaderModule fragShaderModule = create_shader_module(fragShaderCode);

    auto vertShaderStageInfo = vk::PipelineShaderStageCreateInfo{}
                                   .setStage(vk::ShaderStageFlagBits::eVertex)
                                   .setModule(vertShaderModule)
                                   .setPName("main");

    auto fragShaderStageInfo = vk::PipelineShaderStageCreateInfo{}
                                   .setStage(vk::ShaderStageFlagBits::eFragment)
                                   .setModule(fragShaderModule)
                                   .setPName("main");
    std::array shaderStages = {vertShaderStageInfo, fragShaderStageInfo};

    auto bindingDescription = getBindingDescription();
    auto attributeDescriptions = getAttributeDescriptions();

    auto vertexInputInfo = vk::PipelineVertexInputStateCreateInfo{}
                               .setVertexBindingDescriptionCount(1)
                               .setPVertexBindingDescriptions(&bindingDescription)
                               .setVertexAttributeDescriptions(attributeDescriptions);

    auto inputAssembly = vk::PipelineInputAssemblyStateCreateInfo{}
                             .setTopology(vk::PrimitiveTopology::eTriangleList)
                             .setPrimitiveRestartEnable(vk::False);

    auto viewportState = vk::PipelineViewportStateCreateInfo{}.setViewportCount(1).setScissorCount(
        1
    );

    auto rasterizer = vk::PipelineRasterizationStateCreateInfo{}
                          .setDepthClampEnable(vk::False)
                          .setRasterizerDiscardEnable(vk::False)
                          .setPolygonMode(vk::PolygonMode::eFill)
                          .setCullMode(vk::CullModeFlagBits::eBack)
                          .setFrontFace(vk::FrontFace::eCounterClockwise)
                          .setDepthBiasEnable(vk::False)
                          .setLineWidth(1.0F);

    auto multisampling = vk::PipelineMultisampleStateCreateInfo{}
                             .setRasterizationSamples(msaaSamples_)
                             .setSampleShadingEnable(vk::True)
                             .setMinSampleShading(kSampleRateShadingMinFraction)
                             .setAlphaToCoverageEnable(vk::False)
                             .setAlphaToOneEnable(vk::False);

    auto colorBlendAttachment = vk::PipelineColorBlendAttachmentState{}
                                    .setBlendEnable(vk::False)
                                    .setSrcColorBlendFactor(vk::BlendFactor::eOne)
                                    .setDstColorBlendFactor(vk::BlendFactor::eZero)
                                    .setColorBlendOp(vk::BlendOp::eAdd)
                                    .setSrcAlphaBlendFactor(vk::BlendFactor::eOne)
                                    .setDstAlphaBlendFactor(vk::BlendFactor::eZero)
                                    .setAlphaBlendOp(vk::BlendOp::eAdd)
                                    .setColorWriteMask(
                                        vk::ColorComponentFlagBits::eR |
                                        vk::ColorComponentFlagBits::eG |
                                        vk::ColorComponentFlagBits::eB |
                                        vk::ColorComponentFlagBits::eA
                                    );

    auto colorBlending = vk::PipelineColorBlendStateCreateInfo{}
                             .setLogicOpEnable(vk::False)
                             .setLogicOp(vk::LogicOp::eCopy)
                             .setAttachmentCount(1)
                             .setPAttachments(&colorBlendAttachment);

    std::array dynamicStates = {vk::DynamicState::eViewport, vk::DynamicState::eScissor};

    auto dynamicState = vk::PipelineDynamicStateCreateInfo{}.setDynamicStates(dynamicStates);

    auto depthStencil = vk::PipelineDepthStencilStateCreateInfo{}
                            .setDepthTestEnable(vk::True)
                            .setDepthWriteEnable(vk::True)
                            .setDepthCompareOp(vk::CompareOp::eLess)
                            .setDepthBoundsTestEnable(vk::False)
                            .setStencilTestEnable(vk::False);

    auto pipelineInfo = vk::GraphicsPipelineCreateInfo{}
                            .setStages(shaderStages)
                            .setPVertexInputState(&vertexInputInfo)
                            .setPInputAssemblyState(&inputAssembly)
                            .setPViewportState(&viewportState)
                            .setPRasterizationState(&rasterizer)
                            .setPMultisampleState(&multisampling)
                            .setPDepthStencilState(&depthStencil)
                            .setPColorBlendState(&colorBlending)
                            .setPDynamicState(&dynamicState)
                            .setLayout(gvPipelineLayout_)
                            .setRenderPass(gvRenderPass_)
                            .setSubpass(0);

    return gvDevice_.createGraphicsPipeline(VK_NULL_HANDLE, pipelineInfo);
}

vkr::ShaderModule VulkanRenderDevice::create_shader_module(const std::vector<char>& code) {
    return gvDevice_.createShaderModule(
        vk::ShaderModuleCreateInfo{
            {},
            code.size(),
            reinterpret_cast<const uint32_t*>(code.data())  // NOLINT
        }
    );
}

// Command pool
vkr::CommandPool VulkanRenderDevice::create_command_pool() {
    QueueFamilyIndices queueFamilyIndices = find_queue_families(gvPhysicalDevice_);

    auto poolInfo = vk::CommandPoolCreateInfo{}
                        .setFlags(vk::CommandPoolCreateFlagBits::eResetCommandBuffer)
                        .setQueueFamilyIndex(queueFamilyIndices.graphicsFamily.value());

    return gvDevice_.createCommandPool(poolInfo);
}

vkr::CommandBuffer VulkanRenderDevice::begin_single_time_commands() {
    auto allocInfo = vk::CommandBufferAllocateInfo{}
                         .setCommandPool(gvCommandPool_)
                         .setLevel(vk::CommandBufferLevel::ePrimary)
                         .setCommandBufferCount(1);

    auto commandBuffers = gvDevice_.allocateCommandBuffers(allocInfo);
    vkr::CommandBuffer commandBuffer = std::move(commandBuffers[0]);

    commandBuffer.begin(
        vk::CommandBufferBeginInfo{}.setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit)
    );

    return commandBuffer;
}

void VulkanRenderDevice::end_single_time_commands(vkr::CommandBuffer& commandBuffer) {
    commandBuffer.end();

    vk::CommandBuffer rawCmd = *commandBuffer;
    auto submitInfo = vk::SubmitInfo{}.setCommandBuffers(rawCmd);

    gvGraphicsQueue_.submit(submitInfo, VK_NULL_HANDLE);
    gvGraphicsQueue_.waitIdle();
}

// Color and depth resources
VulkanRenderDevice::ColorResources VulkanRenderDevice::create_color_resources() {
    vk::Format colorFormat = gvSwapChainImageFormat_;

    auto [image, memory] = create_image(
        {.width = gvSwapChainExtent_.width,
         .height = gvSwapChainExtent_.height,
         .mipLevels = 1,
         .samples = msaaSamples_,
         .format = colorFormat,
         .tiling = vk::ImageTiling::eOptimal,
         .usage = vk::ImageUsageFlagBits::eTransientAttachment |
                  vk::ImageUsageFlagBits::eColorAttachment,
         .memoryProperties = vk::MemoryPropertyFlagBits::eDeviceLocal}
    );

    auto view = create_image_view(
        {.image = *image,
         .format = colorFormat,
         .aspectFlags = vk::ImageAspectFlagBits::eColor,
         .mipLevels = 1}
    );

    return {.image = std::move(image), .memory = std::move(memory), .view = std::move(view)};
}

VulkanRenderDevice::DepthResources VulkanRenderDevice::create_depth_resources() {
    vk::Format depthFormat = find_depth_format();

    auto [image, memory] = create_image(
        {.width = gvSwapChainExtent_.width,
         .height = gvSwapChainExtent_.height,
         .mipLevels = 1,
         .samples = msaaSamples_,
         .format = depthFormat,
         .tiling = vk::ImageTiling::eOptimal,
         .usage = vk::ImageUsageFlagBits::eDepthStencilAttachment,
         .memoryProperties = vk::MemoryPropertyFlagBits::eDeviceLocal}
    );

    auto view = create_image_view(
        {.image = *image,
         .format = depthFormat,
         .aspectFlags = vk::ImageAspectFlagBits::eDepth,
         .mipLevels = 1}
    );

    return {.image = std::move(image), .memory = std::move(memory), .view = std::move(view)};
}

// Framebuffers
std::vector<vkr::Framebuffer> VulkanRenderDevice::create_framebuffers() {
    return swapChainImageViews_ | std::views::transform([this](const auto& imageView) {
               std::array attachments = {*colorResources_.view, *depthResources_.view, *imageView};
               return gvDevice_.createFramebuffer(
                   vk::FramebufferCreateInfo{}
                       .setRenderPass(gvRenderPass_)
                       .setAttachments(attachments)
                       .setWidth(gvSwapChainExtent_.width)
                       .setHeight(gvSwapChainExtent_.height)
                       .setLayers(1)
               );
           }) |
           std::ranges::to<std::vector>();
}

// Vertex, index, and uniform buffers
VulkanRenderDevice::VertexBufferAllocation VulkanRenderDevice::create_vertex_buffer() {
    auto [buffer, memory] = create_buffer(
        {.size = kbufferDefaultSize,
         .usage = vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer,
         .memoryProperties = vk::MemoryPropertyFlagBits::eDeviceLocal}
    );
    return {.buffer = std::move(buffer), .memory = std::move(memory)};
}

VulkanRenderDevice::IndexBufferAllocation VulkanRenderDevice::create_index_buffer() {
    auto [buffer, memory] = create_buffer(
        {.size = kbufferDefaultSize,
         .usage = vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer,
         .memoryProperties = vk::MemoryPropertyFlagBits::eDeviceLocal}
    );
    return {.buffer = std::move(buffer), .memory = std::move(memory)};
}

VulkanRenderDevice::UniformBufferAllocation VulkanRenderDevice::create_uniform_buffers() {
    vk::DeviceSize bufferSize = sizeof(CameraUBO);

    UniformBufferAllocation alloc;
    for (size_t i = 0; i < kMAX_FRAMES_IN_FLIGHT; i++) {
        auto [buffer, memory] = create_buffer(
            {.size = bufferSize,
             .usage = vk::BufferUsageFlagBits::eUniformBuffer,
             .memoryProperties = vk::MemoryPropertyFlagBits::eHostVisible |
                                 vk::MemoryPropertyFlagBits::eHostCoherent}
        );
        alloc.buffers.push_back(std::move(buffer));
        alloc.memory.push_back(std::move(memory));
        alloc.mapped.push_back(alloc.memory.back().mapMemory(0, bufferSize));
    }
    return alloc;
}

VulkanRenderDevice::ModelBufferAllocation VulkanRenderDevice::create_model_buffers(
    const uint32_t minCapacity
) {
    uint32_t capacity = std::max(minCapacity, kInitialModelCapacity);
    vk::DeviceSize bufferSize = static_cast<vk::DeviceSize>(capacity) * sizeof(glm::mat4);

    ModelBufferAllocation alloc;
    alloc.capacity = capacity;
    for (size_t i = 0; i < kMAX_FRAMES_IN_FLIGHT; ++i) {
        auto [buffer, memory] = create_buffer(
            {.size = bufferSize,
             .usage = vk::BufferUsageFlagBits::eStorageBuffer,
             .memoryProperties = vk::MemoryPropertyFlagBits::eHostVisible |
                                 vk::MemoryPropertyFlagBits::eHostCoherent}
        );
        alloc.buffers.push_back(std::move(buffer));
        alloc.memory.push_back(std::move(memory));
        alloc.mapped.push_back(alloc.memory.back().mapMemory(0, bufferSize));
    }
    return alloc;
}

void VulkanRenderDevice::destroy_model_buffers() {
    for (size_t i = 0; i < modelBufferAlloc_.mapped.size(); ++i) {
        if (modelBufferAlloc_.mapped[i] && *modelBufferAlloc_.memory[i]) {
            modelBufferAlloc_.memory[i].unmapMemory();
        }
    }
    modelBufferAlloc_.buffers.clear();
    modelBufferAlloc_.memory.clear();
    modelBufferAlloc_.mapped.clear();
    modelBufferAlloc_.capacity = 0;
}

void VulkanRenderDevice::ensure_model_capacity(const uint32_t requiredModelCount) {
    if (requiredModelCount <= modelBufferAlloc_.capacity) return;
    uint32_t newCap = modelBufferAlloc_.capacity;
    while (newCap < requiredModelCount) newCap *= 2;
    destroy_model_buffers();
    modelBufferAlloc_ = create_model_buffers(newCap);
    update_descriptor_sets();
}

// Descriptor pool and sets
vkr::DescriptorPool VulkanRenderDevice::create_descriptor_pool() {
    std::array poolSizes{
        vk::DescriptorPoolSize{}
            .setType(vk::DescriptorType::eUniformBuffer)
            .setDescriptorCount(kMAX_FRAMES_IN_FLIGHT),
        vk::DescriptorPoolSize{}
            .setType(vk::DescriptorType::eStorageBuffer)
            .setDescriptorCount(kMAX_FRAMES_IN_FLIGHT),
        vk::DescriptorPoolSize{}
            .setType(vk::DescriptorType::eSampler)
            .setDescriptorCount(kMAX_FRAMES_IN_FLIGHT),
        vk::DescriptorPoolSize{}
            .setType(vk::DescriptorType::eSampledImage)
            .setDescriptorCount(textureLimit_ * kMAX_FRAMES_IN_FLIGHT)
    };

    auto poolInfo = vk::DescriptorPoolCreateInfo{}
                        .setFlags(vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet)
                        .setMaxSets(kMAX_FRAMES_IN_FLIGHT)
                        .setPoolSizes(poolSizes);

    return gvDevice_.createDescriptorPool(poolInfo);
}

std::vector<vkr::DescriptorSet> VulkanRenderDevice::create_descriptor_sets() {
    std::vector<vk::DescriptorSetLayout> layouts(kMAX_FRAMES_IN_FLIGHT, *gvDescriptorSetLayout_);

    std::vector<uint32_t> variableCounts(kMAX_FRAMES_IN_FLIGHT, textureLimit_);

    auto varInfo = vk::DescriptorSetVariableDescriptorCountAllocateInfo{}.setDescriptorCounts(
        variableCounts
    );

    auto allocInfo = vk::DescriptorSetAllocateInfo{}
                         .setDescriptorPool(gvDescriptorPool_)
                         .setSetLayouts(layouts)
                         .setPNext(&varInfo);

    auto sets = gvDevice_.allocateDescriptorSets(allocInfo);
    update_descriptor_sets();
    return sets;
}

void VulkanRenderDevice::update_descriptor_sets() {
    if (descriptorSets_.empty()) return;
    std::vector<vk::DescriptorImageInfo>
        imageInfos = gvTextures_ | std::views::transform([](const auto& texture) {
                         return vk::DescriptorImageInfo{}
                             .setImageView(texture.textureImageView)
                             .setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal);
                     }) |
                     std::ranges::to<std::vector>();
    std::vector<vk::DescriptorBufferInfo> camInfos(kMAX_FRAMES_IN_FLIGHT);
    std::vector<vk::DescriptorBufferInfo> modelInfos(kMAX_FRAMES_IN_FLIGHT);
    std::vector<vk::WriteDescriptorSet> writes;

    for (size_t i = 0; i < kMAX_FRAMES_IN_FLIGHT; ++i) {
        camInfos[i] = vk::DescriptorBufferInfo{}
                          .setBuffer(uniformBufferAlloc_.buffers[i])
                          .setOffset(0)
                          .setRange(sizeof(CameraUBO));
        writes.push_back(
            vk::WriteDescriptorSet{}
                .setDstSet(descriptorSets_[i])
                .setDstBinding(0)
                .setDstArrayElement(0)
                .setDescriptorCount(1)
                .setDescriptorType(vk::DescriptorType::eUniformBuffer)
                .setPBufferInfo(&camInfos[i])
        );

        if (!modelBufferAlloc_.buffers.empty()) {
            modelInfos[i] = vk::DescriptorBufferInfo{}
                                .setBuffer(modelBufferAlloc_.buffers[i])
                                .setOffset(0)
                                .setRange(
                                    static_cast<vk::DeviceSize>(modelBufferAlloc_.capacity) *
                                    sizeof(glm::mat4)
                                );
            writes.push_back(
                vk::WriteDescriptorSet{}
                    .setDstSet(descriptorSets_[i])
                    .setDstBinding(1)
                    .setDstArrayElement(0)
                    .setDescriptorCount(1)
                    .setDescriptorType(vk::DescriptorType::eStorageBuffer)
                    .setPBufferInfo(&modelInfos[i])
            );
        }

        if (!gvTextures_.empty()) {
            writes.push_back(
                vk::WriteDescriptorSet{}
                    .setDstSet(descriptorSets_[i])
                    .setDstBinding(3)
                    .setDstArrayElement(0)
                    .setDescriptorCount(static_cast<uint32_t>(gvTextures_.size()))
                    .setDescriptorType(vk::DescriptorType::eSampledImage)
                    .setPImageInfo(imageInfos.data())
            );
        }
    }
    if (!writes.empty()) {
        gvDevice_.updateDescriptorSets(writes, {});
    }
}

// Command buffers
std::vector<vkr::CommandBuffer> VulkanRenderDevice::create_command_buffers() {
    auto allocInfo = vk::CommandBufferAllocateInfo{}
                         .setCommandPool(gvCommandPool_)
                         .setLevel(vk::CommandBufferLevel::ePrimary)
                         .setCommandBufferCount(kMAX_FRAMES_IN_FLIGHT);

    return gvDevice_.allocateCommandBuffers(allocInfo);
}

void VulkanRenderDevice::record_command_buffer(
    vkr::CommandBuffer& commandBuffer,
    const uint32_t imageIndex,
    ECSController& world
) {
    commandBuffer.begin(
        vk::CommandBufferBeginInfo{}.setFlags(vk::CommandBufferUsageFlagBits::eSimultaneousUse)
    );

    std::array clearValues{
        vk::ClearValue{}.setColor(vk::ClearColorValue{std::array{0.0F, 0.0F, 0.0F, 1.0F}}),
        vk::ClearValue{}.setDepthStencil(vk::ClearDepthStencilValue{}.setDepth(1.0F).setStencil(0))
    };

    auto renderPassInfo = vk::RenderPassBeginInfo{}
                              .setRenderPass(gvRenderPass_)
                              .setFramebuffer(swapChainFramebuffers_[imageIndex])
                              .setRenderArea(
                                  vk::Rect2D{}.setOffset({0, 0}).setExtent(gvSwapChainExtent_)
                              )
                              .setClearValueCount(static_cast<uint32_t>(clearValues.size()))
                              .setPClearValues(clearValues.data());

    commandBuffer.beginRenderPass(renderPassInfo, vk::SubpassContents::eInline);

    commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, gvPipeline_);

    auto viewport = vk::Viewport{}
                        .setX(0.0F)
                        .setY(0.0F)
                        .setWidth(static_cast<float>(gvSwapChainExtent_.width))
                        .setHeight(static_cast<float>(gvSwapChainExtent_.height))
                        .setMinDepth(0.0F)
                        .setMaxDepth(1.0F);

    commandBuffer.setViewport(0, viewport);

    vk::Rect2D scissor{vk::Offset2D{0, 0}, gvSwapChainExtent_};

    commandBuffer.setScissor(0, scissor);

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
    auto modelMatrices = entities | std::views::transform([&world](auto e) {
                             const auto& tf = world.get_component<Transform>(e);
                             return glm::translate(glm::mat4_cast(tf.rotation), tf.position);
                         }) |
                         std::ranges::to<std::vector>();

    ensure_model_capacity(static_cast<uint32_t>(modelMatrices.size()));
    update_model_buffer(currentFrame_, modelMatrices);

    // Bind descriptor sets once
    commandBuffer.bindDescriptorSets(
        vk::PipelineBindPoint::eGraphics,
        *gvPipelineLayout_,
        0,
        *descriptorSets_[currentFrame_],
        {}
    );

    uint32_t modelIdx = 0;
    for (auto e : entities) {
        const auto& r = world.get_component<Renderable>(e);
        const auto& msh = gvMeshes_[r.meshHandle];
        vk::DeviceSize vByteOffset = static_cast<vk::DeviceSize>(msh.firstVertex) * sizeof(Vertex);
        commandBuffer.bindVertexBuffers(0, *vertexBuffer_.buffer, vByteOffset);
        commandBuffer.bindIndexBuffer(
            *indexBuffer_.buffer,
            static_cast<vk::DeviceSize>(msh.firstIndex) * sizeof(uint32_t),
            vk::IndexType::eUint32
        );
        struct PC {
            uint32_t texIndex;
            uint32_t modelIndex;
        } pc{.texIndex = r.texHandle, .modelIndex = modelIdx};
        commandBuffer.pushConstants<PC>(
            *gvPipelineLayout_,
            vk::ShaderStageFlagBits::eFragment | vk::ShaderStageFlagBits::eVertex,
            0,
            pc
        );
        commandBuffer.drawIndexed(msh.indexCount, 1, 0, 0, 0);
        ++modelIdx;
    }

    commandBuffer.endRenderPass();
    commandBuffer.end();
}

// Synchronization objects
std::vector<vkr::Semaphore> VulkanRenderDevice::create_sync_objects() {
    auto fenceInfo = vk::FenceCreateInfo{}.setFlags(vk::FenceCreateFlagBits::eSignaled);

    inFlightFences_ = std::views::iota(0U, kMAX_FRAMES_IN_FLIGHT) |
                      std::views::transform([this, &fenceInfo](auto) {
                          return gvDevice_.createFence(fenceInfo);
                      }) |
                      std::ranges::to<std::vector>();

    const auto imageCount = static_cast<uint32_t>(swapChainImages_.size());
    renderFinishedSemaphores_ = std::views::iota(0U, imageCount) |
                                std::views::transform([this](auto) {
                                    return gvDevice_.createSemaphore({});
                                }) |
                                std::ranges::to<std::vector>();

    imageInFlight_.assign(imageCount, VK_NULL_HANDLE);

    return std::views::iota(0U, kMAX_FRAMES_IN_FLIGHT) |
           std::views::transform([this](auto) { return gvDevice_.createSemaphore({}); }) |
           std::ranges::to<std::vector>();
}

// Low-level resource creation helpers
VulkanRenderDevice::ImageAllocation VulkanRenderDevice::create_image(const ImageCreateInfo& info) {
    auto imageInfo = vk::ImageCreateInfo{}
                         .setImageType(vk::ImageType::e2D)
                         .setFormat(info.format)
                         .setExtent(vk::Extent3D{info.width, info.height, 1})
                         .setMipLevels(info.mipLevels)
                         .setArrayLayers(1)
                         .setSamples(info.samples)
                         .setTiling(info.tiling)
                         .setUsage(info.usage)
                         .setSharingMode(vk::SharingMode::eExclusive);

    vkr::Image image = gvDevice_.createImage(imageInfo);

    vk::MemoryRequirements memRequirements = image.getMemoryRequirements();

    auto allocInfo = vk::MemoryAllocateInfo{}
                         .setAllocationSize(memRequirements.size)
                         .setMemoryTypeIndex(
                             find_memory_type(memRequirements.memoryTypeBits, info.memoryProperties)
                         );

    vkr::DeviceMemory memory = gvDevice_.allocateMemory(allocInfo);
    image.bindMemory(*memory, 0);

    return {.image = std::move(image), .memory = std::move(memory)};
}

VulkanRenderDevice::BufferAllocation VulkanRenderDevice::create_buffer(
    const BufferCreateInfo& info
) {
    auto bufferInfo = vk::BufferCreateInfo{}
                          .setSize(info.size)
                          .setUsage(info.usage)
                          .setSharingMode(vk::SharingMode::eExclusive);

    vkr::Buffer buffer = gvDevice_.createBuffer(bufferInfo);

    vk::MemoryRequirements memRequirements = buffer.getMemoryRequirements();

    auto allocInfo = vk::MemoryAllocateInfo{}
                         .setAllocationSize(memRequirements.size)
                         .setMemoryTypeIndex(
                             find_memory_type(memRequirements.memoryTypeBits, info.memoryProperties)
                         );

    vkr::DeviceMemory memory = gvDevice_.allocateMemory(allocInfo);
    buffer.bindMemory(*memory, 0);

    return {.buffer = std::move(buffer), .memory = std::move(memory)};
}

uint32_t VulkanRenderDevice::find_memory_type(
    const uint32_t typeFilter,
    const vk::MemoryPropertyFlags properties
) {
    vk::PhysicalDeviceMemoryProperties memProperties = gvPhysicalDevice_.getMemoryProperties();

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if (((typeFilter & (1 << i)) != 0U) &&
            (memProperties.memoryTypes.at(i).propertyFlags & properties) == properties) {
            return i;
        }
    }

    throw std::runtime_error("failed to find suitable memory type!");
}

template <typename T>
std::pair<vkr::Buffer, vkr::DeviceMemory> VulkanRenderDevice::create_staging_buffer(
    std::span<T> data
) {
    vk::DeviceSize size = sizeof(T) * data.size();

    auto [buffer, memory] = create_buffer(
        {.size = size,
         .usage = vk::BufferUsageFlagBits::eTransferSrc,
         .memoryProperties = vk::MemoryPropertyFlagBits::eHostVisible |
                             vk::MemoryPropertyFlagBits::eHostCoherent}
    );

    memcpy(memory.mapMemory(0, size), data.data(), size);
    memory.unmapMemory();

    return {std::move(buffer), std::move(memory)};
}

// Resource transfer and manipulation
void VulkanRenderDevice::transition_image_layout(const ImageLayoutTransitionInfo& info) {
    vkr::CommandBuffer commandBuffer = begin_single_time_commands();

    vk::PipelineStageFlags sourceStage{};
    vk::PipelineStageFlags destinationStage{};

    vk::AccessFlags src{};
    vk::AccessFlags dst{};
    if (info.oldLayout == vk::ImageLayout::eUndefined &&
        info.newLayout == vk::ImageLayout::eTransferDstOptimal) {
        src = vk::AccessFlagBits::eNone;
        dst = vk::AccessFlagBits::eTransferWrite;
        sourceStage = vk::PipelineStageFlagBits::eTopOfPipe;
        destinationStage = vk::PipelineStageFlagBits::eTransfer;
    } else if (info.oldLayout == vk::ImageLayout::eTransferDstOptimal &&
               info.newLayout == vk::ImageLayout::eShaderReadOnlyOptimal) {
        src = vk::AccessFlagBits::eTransferWrite;
        dst = vk::AccessFlagBits::eShaderRead;
        sourceStage = vk::PipelineStageFlagBits::eTransfer;
        destinationStage = vk::PipelineStageFlagBits::eFragmentShader;
    } else if (info.oldLayout == vk::ImageLayout::eUndefined &&
               info.newLayout == vk::ImageLayout::eDepthStencilAttachmentOptimal) {
        src = vk::AccessFlagBits::eNone;
        dst = vk::AccessFlagBits::eDepthStencilAttachmentRead |
              vk::AccessFlagBits::eDepthStencilAttachmentWrite;
        sourceStage = vk::PipelineStageFlagBits::eTopOfPipe;
        destinationStage = vk::PipelineStageFlagBits::eEarlyFragmentTests;
    } else {
        throw std::invalid_argument("unsupported layout transition!");
    }

    vk::ImageMemoryBarrier barrier = vk::ImageMemoryBarrier{}
                                         .setSrcAccessMask(src)
                                         .setDstAccessMask(dst)
                                         .setOldLayout(info.oldLayout)
                                         .setNewLayout(info.newLayout)
                                         .setSrcQueueFamilyIndex(vk::QueueFamilyIgnored)
                                         .setDstQueueFamilyIndex(vk::QueueFamilyIgnored)
                                         .setImage(info.image)
                                         .setSubresourceRange(
                                             vk::ImageSubresourceRange{}
                                                 .setAspectMask(vk::ImageAspectFlagBits::eColor)
                                                 .setBaseMipLevel(0)
                                                 .setLevelCount(info.mipLevels)
                                                 .setBaseArrayLayer(0)
                                                 .setLayerCount(1)
                                         );

    commandBuffer.pipelineBarrier(sourceStage, destinationStage, {}, {}, {}, barrier);

    end_single_time_commands(commandBuffer);
}

void VulkanRenderDevice::copy_buffer(const CopyBufferInfo& info) {
    vkr::CommandBuffer commandBuffer = begin_single_time_commands();
    auto copyRegion = vk::BufferCopy{}
                          .setSrcOffset(0)
                          .setDstOffset(info.dstOffset)
                          .setSize(info.size);
    commandBuffer.copyBuffer(info.src, info.dst, copyRegion);
    end_single_time_commands(commandBuffer);
}

void VulkanRenderDevice::copy_buffer_to_image(const CopyBufferToImageInfo& info) {
    vkr::CommandBuffer commandBuffer = begin_single_time_commands();

    auto region = vk::BufferImageCopy{}
                      .setBufferOffset(0)
                      .setBufferRowLength(0)
                      .setBufferImageHeight(0)
                      .setImageSubresource(
                          vk::ImageSubresourceLayers{}
                              .setAspectMask(vk::ImageAspectFlagBits::eColor)
                              .setMipLevel(0)
                              .setBaseArrayLayer(0)
                              .setLayerCount(1)
                      )
                      .setImageOffset({0, 0, 0})
                      .setImageExtent({info.width, info.height, 1});

    commandBuffer
        .copyBufferToImage(info.buffer, info.image, vk::ImageLayout::eTransferDstOptimal, region);

    end_single_time_commands(commandBuffer);
}

void VulkanRenderDevice::generate_mipmaps(const MipmapGenerationInfo& info) {
    vk::FormatProperties formatProperties = gvPhysicalDevice_.getFormatProperties(info.format);
    if (!(formatProperties.optimalTilingFeatures &
          vk::FormatFeatureFlagBits::eSampledImageFilterLinear)) {
        throw std::runtime_error("texture image format does not support linear blitting!");
    }
    vkr::CommandBuffer commandBuffer = begin_single_time_commands();

    auto barrier = vk::ImageMemoryBarrier{}
                       .setImage(info.image)
                       .setSrcQueueFamilyIndex(vk::QueueFamilyIgnored)
                       .setDstQueueFamilyIndex(vk::QueueFamilyIgnored)
                       .setSubresourceRange(
                           vk::ImageSubresourceRange{}
                               .setAspectMask(vk::ImageAspectFlagBits::eColor)
                               .setBaseArrayLayer(0)
                               .setLayerCount(1)
                               .setLevelCount(1)
                       );

    int32_t mipWidth = info.size.width;
    int32_t mipHeight = info.size.height;
    for (uint32_t i = 1; i < info.mipLevels; ++i) {
        barrier.subresourceRange.baseMipLevel = i - 1;
        barrier.oldLayout = vk::ImageLayout::eTransferDstOptimal;
        barrier.newLayout = vk::ImageLayout::eTransferSrcOptimal;
        barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
        barrier.dstAccessMask = vk::AccessFlagBits::eTransferRead;
        commandBuffer.pipelineBarrier(
            vk::PipelineStageFlagBits::eTransfer,
            vk::PipelineStageFlagBits::eTransfer,
            {},
            {},
            {},
            barrier
        );
        auto blit = vk::ImageBlit{}
                        .setSrcSubresource(
                            vk::ImageSubresourceLayers{}
                                .setAspectMask(vk::ImageAspectFlagBits::eColor)
                                .setMipLevel(i - 1)
                                .setBaseArrayLayer(0)
                                .setLayerCount(1)
                        )
                        .setSrcOffsets(
                            {vk::Offset3D{0, 0, 0}, vk::Offset3D{mipWidth, mipHeight, 1}}
                        )
                        .setDstSubresource(
                            vk::ImageSubresourceLayers{}
                                .setAspectMask(vk::ImageAspectFlagBits::eColor)
                                .setMipLevel(i)
                                .setBaseArrayLayer(0)
                                .setLayerCount(1)
                        )
                        .setDstOffsets(
                            {vk::Offset3D{0, 0, 0},
                             vk::Offset3D{
                                 (mipWidth > 1 ? mipWidth / 2 : 1),
                                 (mipHeight > 1 ? mipHeight / 2 : 1),
                                 1
                             }}
                        );
        commandBuffer.blitImage(
            info.image,
            vk::ImageLayout::eTransferSrcOptimal,
            info.image,
            vk::ImageLayout::eTransferDstOptimal,
            blit,
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
    barrier.subresourceRange.baseMipLevel = info.mipLevels - 1;
    barrier.oldLayout = vk::ImageLayout::eTransferDstOptimal;
    barrier.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
    barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
    barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;
    commandBuffer.pipelineBarrier(
        vk::PipelineStageFlagBits::eTransfer,
        vk::PipelineStageFlagBits::eFragmentShader,
        {},
        nullptr,
        nullptr,
        barrier
    );
    end_single_time_commands(commandBuffer);
}

// Runtime update functions
void VulkanRenderDevice::update_camera_buffer(const uint32_t currentImage) {
    memcpy(uniformBufferAlloc_.mapped[currentImage], &cameraUbo_, sizeof(CameraUBO));
}

void VulkanRenderDevice::update_model_buffer(
    const uint32_t currentImage,
    const std::vector<glm::mat4>& models
) {
    auto bytes = models.size() * sizeof(glm::mat4);
    memcpy(modelBufferAlloc_.mapped[currentImage], models.data(), bytes);
}

// Historic (deprecated)
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

}  // namespace garnish::vulkan
