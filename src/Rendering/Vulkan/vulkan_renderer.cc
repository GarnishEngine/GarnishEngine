#include "vulkan_renderer.hpp"

#include <vulkan/vulkan_core.h>

#include <cstddef>
#include <cstdint>
#include <iostream>
#include <stdexcept>

#include "garnish_texture.hpp"

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

    gvInstance = vk::createInstance(instanceCreateInfo, nullptr, {});
}

bool VulkanRenderDevice::pick_physical_device() {
    std::vector physicalDevices = gvInstance.enumeratePhysicalDevices({});
    if (physicalDevices.size() == 0) {
        throw std::runtime_error("no physical devices for vulkan");
    }

    gvPhysicalDevice = physicalDevices[0];
    return true;
}

bool VulkanRenderDevice::create_logical_device() {
    QueueFamilyIndices indices = find_queue_families();

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
    vk::DeviceCreateInfo createInfo{
        {},
        static_cast<uint32_t>(queueCreateInfos.size()),
        queueCreateInfos.data(),
        static_cast<uint32_t>(validationLayers.size()),
        validationLayers.data(),
        static_cast<uint32_t>(deviceExtensions.size()),
        deviceExtensions.data(),
        &deviceFeatures
    };
    gvDevice = gvPhysicalDevice.createDevice(createInfo);
    gvGraphicsQueue = gvDevice.getQueue(indices.graphicsFamily.value(), 0);
    gvPresentQueue = gvDevice.getQueue(indices.presentFamily.value(), 0);
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

    vk::SurfaceFormatKHR surfaceFormat = swapChainSupport.formats[0];
    vk::PresentModeKHR presentModes{VK_PRESENT_MODE_MAILBOX_KHR};
    vk::Extent2D extent = create_extent();

    QueueFamilyIndices indices = find_queue_families();
    std::array<uint32_t, 2> queueFamilyIndices{
        indices.graphicsFamily.value(),
        indices.presentFamily.value()
    };

    vk::SharingMode imageSharingMode;
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

    swapChainImageFormat = surfaceFormat.format;
    swapChainExtent = extent;
    swapChainImages = gvDevice.getSwapchainImagesKHR(gvSwapchainKHR);
    return true;
}

bool VulkanRenderDevice::create_image_views() {
    swapChainImageViews.resize(swapChainImages.size());

    for (auto& image : swapChainImages) {
        create_image_view(
            image,
            swapChainImageFormat,
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
    vk::DescriptorSetLayoutBinding uboLayoutBinding{
        0,
        vk::DescriptorType::eUniformBuffer,
        1,
        vk::ShaderStageFlagBits::eVertex
    };

    vk::DescriptorSetLayoutBinding samplerLayoutBinding{
        1,
        vk::DescriptorType::eCombinedImageSampler,
        1,
        vk::ShaderStageFlagBits::eFragment
    };

    std::array<vk::DescriptorSetLayoutBinding, 2> bindings = {
        uboLayoutBinding,
        samplerLayoutBinding
    };
    vk::DescriptorSetLayoutCreateInfo layoutInfo{{}, 2, bindings.data()};

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

    std::array<vk::AttachmentDescription, 3> attachments = {
        vk::AttachmentDescription{
            {},
            swapChainImageFormat,
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
            swapChainImageFormat,
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
            vk::AttachmentStoreOp::eStore,
            vk::AttachmentLoadOp::eDontCare,
            vk::AttachmentStoreOp::eDontCare,
            vk::ImageLayout::eUndefined,
            vk::ImageLayout::eDepthAttachmentOptimal
        }
    };
    vk::RenderPassCreateInfo
        renderPassInfo{{}, 3, attachments.data(), 1, &subpass, 1, &dependency};
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
        fragShaderModule,
        "main"
    };
    vk::PipelineShaderStageCreateInfo fragShaderStageInfo{
        {},
        vk::ShaderStageFlagBits::eFragment,
        fragShaderModule,
        "main"
    };
    std::array<vk::PipelineShaderStageCreateInfo, 2> shaderStages = {
        vertShaderStageInfo,
        fragShaderStageInfo
    };

    auto bindingDescription = GVvertex3d::getBindingDescription();
    auto attributeDescriptions = GVvertex3d::getAttributeDescriptions();

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
        0.2F,
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

    vk::PipelineLayoutCreateInfo pipelineLayoutInfo{
        {},
        1,
        &gvDescriptorSetLayout,
        0
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
    vk::Format colorFormat = swapChainImageFormat;

    create_image(
        swapChainExtent.width,
        swapChainExtent.height,
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
}

bool VulkanRenderDevice::create_depth_resources() {
    vk::Format depthFormat = find_depth_format();

    create_image(
        swapChainExtent.width,
        swapChainExtent.height,
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
}

bool VulkanRenderDevice::create_framebuffers() {
    swapChainFramebuffers.resize(swapChainImageViews.size());

    for (size_t i = 0; i < swapChainImageViews.size(); i++) {
        std::array<vk::ImageView, 3> attachments = {
            colorImageView,
            depthImageView,
            swapChainImageViews[i]
        };

        vk::FramebufferCreateInfo framebufferInfo{
            {},
            gvRenderPass,
            static_cast<uint32_t>(attachments.size()),
            attachments.data(),
            swapChainExtent.width,
            swapChainExtent.height,
            1
        };
        swapChainFramebuffers[i] = gvDevice.createFramebuffer(framebufferInfo);
    }
    return true;
}

bool VulkanRenderDevice::create_command_pool() {
    QueueFamilyIndices queueFamilyIndices = find_queue_families();

    vk::CommandPoolCreateInfo poolInfo{
        vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
        queueFamilyIndices.graphicsFamily.value()
    };

    gvCommandPool = gvDevice.createCommandPool(poolInfo);

    return true;
}

bool VulkanRenderDevice::create_texture_image(const std::string& TEXTURE_PATH) {
    int texWidth, texHeight, texChannels;
    stbi_uc* pixels = stbi_load(
        TEXTURE_PATH.c_str(),
        &texWidth,
        &texHeight,
        &texChannels,
        STBI_rgb_alpha
    );
    vk::DeviceSize imageSize = texWidth * texHeight * 4;

    mipLevels = static_cast<uint32_t>(
                    std::floor(std::log2(std::max(texWidth, texHeight)))
                ) +
                1;

    if (!pixels) {
        throw std::runtime_error("failed to load texture image!");
    }

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
        gvTextureImage,
        textureImageMemory
    );

    transition_image_layout(
        gvTextureImage,
        vk::Format::eR8G8B8A8Srgb,
        vk::ImageLayout::eUndefined,
        vk::ImageLayout::eTransferDstOptimal,
        mipLevels
    );

    copy_buffer_to_image(
        stagingBuffer,
        gvTextureImage,
        static_cast<uint32_t>(texWidth),
        static_cast<uint32_t>(texHeight)
    );

    generate_mipmaps(
        gvTextureImage,
        vk::Format::eR8G8B8A8Srgb,
        texWidth,
        texHeight,
        mipLevels
    );

    gvDevice.destroyBuffer(stagingBuffer);
    gvDevice.freeMemory(stagingBufferMemory);
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

bool VulkanRenderDevice::create_texture_image_view() {
    gvTextureImageView = create_image_view(
        gvTextureImage,
        vk::Format::eR8G8B8A8Srgb,
        vk::ImageAspectFlagBits::eColor,
        mipLevels
    );
}

bool VulkanRenderDevice::create_texture_sampler() {
    vk::PhysicalDeviceProperties properties = gvPhysicalDevice.getProperties();

    gvTextureFormat = gvDevice.createSampler(
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
}

// void loadModel() {
//     tinyobj::attrib_t attrib;
//     std::vector<tinyobj::shape_t> shapes;
//     std::vector<tinyobj::material_t> materials;
//     std::string warn, err;

//     if (!tinyobj::LoadObj(
//             &attrib,
//             &shapes,
//             &materials,
//             &warn,
//             &err,
//             MODEL_PATH.c_str()
//         )) {
//         throw std::runtime_error(warn + err);
//     }

//     std::unordered_map<Vertex, uint32_t> uniqueVertices{};

//     for (const auto& shape : shapes) {
//         for (const auto& index : shape.mesh.indices) {
//             Vertex vertex{};
//             vertex.pos = {
//                 attrib.vertices[3 * index.vertex_index + 0],
//                 attrib.vertices[3 * index.vertex_index + 1],
//                 attrib.vertices[3 * index.vertex_index + 2]
//             };

//             vertex.texCoord = {
//                 attrib.texcoords[2 * index.texcoord_index + 0],
//                 1.0F - attrib.texcoords[2 * index.texcoord_index + 1]
//             };

//             vertex.color = {1.0F, 1.0F, 1.0F};

//             if (uniqueVertices.count(vertex) == 0) {
//                 uniqueVertices[vertex] =
//                 static_cast<uint32_t>(vertices.size());
//                 vertices.push_back(vertex);
//             }

//             indices.push_back(uniqueVertices[vertex]);
//         }
//     }
// }

void VulkanRenderDevice::create_vertex_buffer() {
    vk::DeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();

    vk::Buffer stagingBuffer;
    vk::DeviceMemory stagingBufferMemory;
    create_buffer(
        bufferSize,
        vk::BufferUsageFlagBits::eTransferSrc,
        vk::MemoryPropertyFlagBits::eHostVisible |
            vk::MemoryPropertyFlagBits::eHostCoherent,
        stagingBuffer,
        stagingBufferMemory
    );

    void* data = gvDevice.mapMemory(stagingBufferMemory, 0, bufferSize);
    memcpy(data, vertices.data(), (size_t)bufferSize);
    gvDevice.unmapMemory(stagingBufferMemory);

    create_buffer(
        bufferSize,
        vk::BufferUsageFlagBits::eTransferDst |
            vk::BufferUsageFlagBits::eVertexBuffer,
        vk::MemoryPropertyFlagBits::eDeviceLocal,
        vertexBuffer,
        vertexBufferMemory
    );

    copy_buffer(stagingBuffer, vertexBuffer, bufferSize);

    gvDevice.destroyBuffer(stagingBuffer);
    gvDevice.freeMemory(stagingBufferMemory);
}

void VulkanRenderDevice::create_index_buffer() {
    vk::DeviceSize bufferSize = sizeof(indices[0]) * indices.size();

    vk::Buffer stagingBuffer;
    vk::DeviceMemory stagingBufferMemory;
    create_buffer(
        bufferSize,
        vk::BufferUsageFlagBits::eTransferSrc,
        vk::MemoryPropertyFlagBits::eHostVisible |
            vk::MemoryPropertyFlagBits::eHostCoherent,
        stagingBuffer,
        stagingBufferMemory
    );

    void* data = gvDevice.mapMemory(stagingBufferMemory, 0, bufferSize);
    memcpy(data, indices.data(), (size_t)bufferSize);
    gvDevice.unmapMemory(stagingBufferMemory);

    create_buffer(
        bufferSize,
        vk::BufferUsageFlagBits::eTransferDst |
            vk::BufferUsageFlagBits::eVertexBuffer,
        vk::MemoryPropertyFlagBits::eDeviceLocal,
        vertexBuffer,
        vertexBufferMemory
    );

    copy_buffer(stagingBuffer, vertexBuffer, bufferSize);

    gvDevice.destroyBuffer(stagingBuffer);
    gvDevice.freeMemory(stagingBufferMemory);
}

void VulkanRenderDevice::create_uniform_buffers() {
    vk::DeviceSize bufferSize = sizeof(UniformBufferObject);

    uniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
    uniformBuffersMemory.resize(MAX_FRAMES_IN_FLIGHT);
    uniformBuffersMapped.resize(MAX_FRAMES_IN_FLIGHT);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        create_buffer(
            bufferSize,
            vk::BufferUsageFlagBits::eTransferDst |
                vk::BufferUsageFlagBits::eVertexBuffer,
            vk::MemoryPropertyFlagBits::eDeviceLocal,
            uniformBuffers[i],
            uniformBuffersMemory[i]
        );

        uniformBuffersMapped[i] =
            gvDevice.mapMemory(uniformBuffersMemory[i], 0, bufferSize);
    }
}

bool VulkanRenderDevice::create_descriptor_pool() {
    std::array<vk::DescriptorPoolSize, 2> poolSizes{
        vk::DescriptorPoolSize{
            vk::DescriptorType::eUniformBuffer,
            MAX_FRAMES_IN_FLIGHT
        },
        vk::DescriptorPoolSize{
            vk::DescriptorType::eCombinedImageSampler,
            MAX_FRAMES_IN_FLIGHT
        }
    };

    vk::DescriptorPoolCreateInfo poolInfo{
        {},
        MAX_FRAMES_IN_FLIGHT,
        poolSizes.size(),
        poolSizes.data()
    };

    gvDescriptorPool = gvDevice.createDescriptorPool(poolInfo);
}

// void createDescriptorSets() {
//     std::vector<vk::DescriptorSetLayout> layouts(
//         MAX_FRAMES_IN_FLIGHT,
//         descriptorSetLayout
//     );
//     vk::DescriptorSetAllocateInfo allocInfo{};
//     allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
//     allocInfo.descriptorPool = descriptorPool;
//     allocInfo.descriptorSetCount =
//     static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT); allocInfo.pSetLayouts =
//     layouts.data();

//     descriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
//     if (vkAllocateDescriptorSets(device, &allocInfo, descriptorSets.data())
//     !=
//         VK_SUCCESS) {
//         throw std::runtime_error("failed to allocate descriptor sets!");
//     }

//     for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
//         vk::DescriptorBufferInfo bufferInfo{};
//         bufferInfo.buffer = uniformBuffers[i];
//         bufferInfo.offset = 0;
//         bufferInfo.range = sizeof(UniformBufferObject);

//         vk::DescriptorImageInfo imageInfo{};
//         imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
//         imageInfo.imageView = textureImageView;
//         imageInfo.sampler = textureSampler;

//         std::array<vk::WriteDescriptorSet, 2> descriptorWrites{};

//         descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
//         descriptorWrites[0].dstSet = descriptorSets[i];
//         descriptorWrites[0].dstBinding = 0;
//         descriptorWrites[0].dstArrayElement = 0;
//         descriptorWrites[0].descriptorType =
//         VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
//         descriptorWrites[0].descriptorCount = 1;
//         descriptorWrites[0].pBufferInfo = &bufferInfo;

//         descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
//         descriptorWrites[1].dstSet = descriptorSets[i];
//         descriptorWrites[1].dstBinding = 1;
//         descriptorWrites[1].dstArrayElement = 0;
//         descriptorWrites[1].descriptorType =
//             VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
//         descriptorWrites[1].descriptorCount = 1;
//         descriptorWrites[1].pImageInfo = &imageInfo;

//         vkUpdateDescriptorSets(
//             device,
//             static_cast<uint32_t>(descriptorWrites.size()),
//             descriptorWrites.data(),
//             0,
//             nullptr
//         );
//     }
// }

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
    vk::DeviceSize size
) {
    vk::CommandBuffer commandBuffer = begin_single_time_commands();

    vk::BufferCopy copyRegion{};
    copyRegion.size = size;

    gvCommandBuffer.copyBuffer(srcBuffer, dstBuffer, 1, &copyRegion);

    end_single_time_commands(commandBuffer);
}

uint32_t VulkanRenderDevice::find_memory_type(
    uint32_t typeFilter,
    vk::MemoryPropertyFlags properties
) {
    vk::PhysicalDeviceMemoryProperties memProperties =
        gvPhysicalDevice.getMemoryProperties();

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i)) &&
            (memProperties.memoryTypes[i].propertyFlags & properties) ==
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

    vk::CommandBuffer commandBuffer;
    commandBuffers = gvDevice.allocateCommandBuffers(allocInfo);

    vk::CommandBufferBeginInfo beginInfo{
        vk::CommandBufferUsageFlagBits::eOneTimeSubmit
    };
    commandBuffer.begin(beginInfo);

    return commandBuffer;
}

void VulkanRenderDevice::end_single_time_commands(
    vk::CommandBuffer commandBuffer
) {
    vkEndCommandBuffer(commandBuffer);

    vk::SubmitInfo submitInfo{};
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;
    std::vector<vk::SubmitInfo> submitInfos{submitInfo};

    gvGraphicsQueue.submit(submitInfos, VK_NULL_HANDLE);

    gvGraphicsQueue.waitIdle();
    gvDevice.freeCommandBuffers(gvCommandPool, commandBuffers);
}

void VulkanRenderDevice::transition_image_layout(
    vk::Image image,
    vk::Format format,
    vk::ImageLayout oldLayout,
    vk::ImageLayout newLayout,
    uint32_t mipLevels
) {
    vk::CommandBuffer commandBuffer = begin_single_time_commands();

    vk::PipelineStageFlags sourceStage;
    vk::PipelineStageFlags destinationStage;

    vk::AccessFlags src, dst;
    if (oldLayout == vk::ImageLayout::eUndefined &&
        newLayout == vk::ImageLayout::eTransferDstOptimal) {
        src = vk::AccessFlagBits::eNone;
        dst = vk::AccessFlagBits::eTransferWrite;

        sourceStage = vk::PipelineStageFlagBits::eTopOfPipe;
        destinationStage = vk::PipelineStageFlagBits::eTransfer;
    } else if (oldLayout == vk::ImageLayout::eTransferDstOptimal &&
               newLayout == vk::ImageLayout::eReadOnlyOptimal) {
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

    gvCommandBuffer.pipelineBarrier(
        sourceStage,
        destinationStage,
        vk::DependencyFlagBits::eByRegion,  // THIS MIGHT NOT
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

    gvCommandBuffer.copyBufferToImage(
        buffer,
        image,
        vk::ImageLayout::eTransferDstOptimal,
        1,
        &region
    );

    end_single_time_commands(commandBuffer);
}

bool VulkanRenderDevice::create_command_buffers() {
    commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

    vk::CommandBufferAllocateInfo allocInfo{
        gvCommandPool,
        vk::CommandBufferLevel::ePrimary,
        static_cast<uint32_t>(commandBuffers.size())
    };

    commandBuffers = gvDevice.allocateCommandBuffers(allocInfo);
}

void VulkanRenderDevice::record_command_buffer(
    vk::CommandBuffer commandBuffer,
    uint32_t imageIndex
) {
    auto res = commandBuffer.begin({});  // result useless??

    std::array<vk::ClearValue, 2> clearValues{
        vk::ClearValue{{0.0F, 0.0F, 0.0F, 1.0F}},
        vk::ClearDepthStencilValue{1.0F, 0}
    };

    vk::RenderPassBeginInfo renderPassInfo{
        gvRenderPass,
        swapChainFramebuffers[imageIndex],
        {{0, 0}, swapChainExtent},
        clearValues.size(),
        clearValues.data()
    };

    commandBuffer.beginRenderPass(renderPassInfo, vk::SubpassContents::eInline);

    commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, gvPipeline);

    vk::Viewport viewport{
        0.0F,
        0.0F,
        static_cast<float>(swapChainExtent.width),
        static_cast<float>(swapChainExtent.height),
        0.0F,
        1.0F
    };

    commandBuffer.setViewport(0, 1, &viewport);

    vk::Rect2D scissor{{0, 0}, swapChainExtent};

    commandBuffer.setScissor(0, 1, &scissor);

    vk::Buffer vertexBuffers[] = {vertexBuffer};
    vk::DeviceSize offsets[] = {0};

    commandBuffer.bindVertexBuffers(0, 1, vertexBuffers, offsets);
    commandBuffer.bindIndexBuffer(indexBuffer, 0, vk::IndexType::eUint32);

    vkCmdBindIndexBuffer(commandBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT32);

    commandBuffer.bindDescriptorSets(
        vk::PipelineBindPoint::eGraphics,
        gvPipelineLayout,
        0,
        1,
        &descriptorSets[currentFrame],
        0,
        nullptr
    );

    commandBuffer.drawIndexed(indices.size(), 1, 0, 0, 0);

    commandBuffer.endRenderPass();
    commandBuffer.end();
}

bool VulkanRenderDevice::create_sync_objects() {
    imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

    vk::FenceCreateInfo fenceInfo{vk::FenceCreateFlagBits::eSignaled};

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        imageAvailableSemaphores[i] = gvDevice.createSemaphore({});
        renderFinishedSemaphores[i] = gvDevice.createSemaphore({});
        renderFinishedSemaphores[i] = gvDevice.createSemaphore({});
        inFlightFences[i] = gvDevice.createFence(fenceInfo);
    }
    return true;
}

void VulkanRenderDevice::update_uniform_buffer(uint32_t currentImage) {
    static auto startTime = std::chrono::high_resolution_clock::now();

    auto currentTime = std::chrono::high_resolution_clock::now();
    float time = std::chrono::duration<float, std::chrono::seconds::period>(
                     currentTime - startTime
    )
                     .count();

    UniformBufferObject ubo{};
    ubo.model = glm::rotate(
        glm::mat4(1.0F),
        time * glm::radians(90.0F),
        glm::vec3(0.0F, 0.0F, 1.0F)
    );
    ubo.view = glm::lookAt(
        glm::vec3(2.0F, 2.0F, 2.0F),
        glm::vec3(0.0F, 0.0F, 0.0F),
        glm::vec3(0.0F, 0.0F, 1.0F)
    );
    ubo.proj = glm::perspective(
        glm::radians(45.0F),
        swapChainExtent.width / (float)swapChainExtent.height,
        0.1f,
        10.0F
    );
    ubo.proj[1][1] *= -1;

    memcpy(uniformBuffersMapped[currentImage], &ubo, sizeof(ubo));
}

void VulkanRenderDevice::draw_frame() {
    gvDevice.waitForFences(inFlightFences, VK_TRUE, UINT64_MAX);

    uint32_t imageIndex;
    auto result = gvDevice.acquireNextImageKHR(
        gvSwapchainKHR,
        UINT64_MAX,
        imageAvailableSemaphores[currentFrame],
        {}
    );

    if (result.result == vk::Result::eErrorOutOfDateKHR) {
        recreate_swap_chain();
        return;
    } else if (result.result != vk::Result::eSuccess &&
               result.result != vk::Result::eSuboptimalKHR) {
        throw std::runtime_error("failed to acquire swap chain image!");
    }

    // Only reset the fence if we are submitting work
    gvDevice.resetFences(1, &inFlightFences[currentFrame]);
    commandBuffers[currentFrame].reset({});

    record_command_buffer(commandBuffers[currentFrame], imageIndex);

    update_uniform_buffer(currentFrame);
    vk::Semaphore waitSemaphores[] = {imageAvailableSemaphores[currentFrame]};
    vk::PipelineStageFlags waitStages[] = {
        vk::PipelineStageFlagBits::eColorAttachmentOutput
    };
    vk::Semaphore signalSemaphores[] = {renderFinishedSemaphores[currentFrame]};

    vk::SubmitInfo submitInfo{
        1,
        waitSemaphores,
        waitStages,
        1,
        &commandBuffers[currentFrame],
        1,
        signalSemaphores
    };

    gvGraphicsQueue.submit(1, &submitInfo, inFlightFences[currentFrame]);
    vk::SwapchainKHR swapChains[] = {gvSwapchainKHR};

    vk::PresentInfoKHR
        presentInfo{1, signalSemaphores, 1, swapChains, &imageIndex};

    auto presentResult = gvPresentQueue.presentKHR(&presentInfo);

    if (presentResult == vk::Result::eErrorOutOfDateKHR ||
        presentResult == vk::Result::eSuboptimalKHR) {
        framebufferResized = false;
        recreate_swap_chain();
    } else if (presentResult != vk::Result::eSuccess) {
        throw std::runtime_error("failed to present swap chain image!");
    }

    currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void VulkanRenderDevice::generate_mipmaps(
    vk::Image image,
    vk::Format imageFormat,
    int32_t texWidth,
    int32_t texHeight,
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

    int32_t mipWidth = texWidth;
    int32_t mipHeight = texHeight;

    for (uint32_t i = 1; i < mipLevels; i++) {
        barrier.subresourceRange.baseMipLevel = i - 1;
        barrier.oldLayout = vk::ImageLayout::eTransferDstOptimal;
        barrier.newLayout = vk::ImageLayout::eTransferSrcOptimal;
        barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
        barrier.dstAccessMask = vk::AccessFlagBits::eTransferRead;

        commandBuffer.pipelineBarrier(
            vk::PipelineStageFlagBits::eTransfer,
            vk::PipelineStageFlagBits::eTransfer,
            vk::DependencyFlagBits::
                eQueueFamilyOwnershipTransferUseAllStagesKHR,  // TODO
            0,
            nullptr,
            0,
            nullptr,
            1,
            &barrier
        );

        vk::ImageBlit blit{
            vk::ImageSubresourceLayers{
                vk::ImageAspectFlagBits::eColor,
                i - 1,
                0,
                1
            },
            std::array<vk::Offset3D, 2>{
                vk::Offset3D{0, 0, 0},
                vk::Offset3D{mipWidth, mipHeight, 1}
            },
            vk::ImageSubresourceLayers{
                vk::ImageAspectFlagBits::eColor,
                i,
                0,
                1
            },
            std::array<vk::Offset3D, 2>{
                vk::Offset3D{0, 0, 0},
                vk::Offset3D{
                    mipWidth > 1 ? mipWidth / 2 : 1,
                    mipHeight > 1 ? mipHeight / 2 : 1,
                    1
                }
            },
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
        barrier.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
        barrier.srcAccessMask = vk::AccessFlagBits::eTransferRead;
        barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;
        commandBuffer.pipelineBarrier(
            vk::PipelineStageFlagBits::eTransfer,
            vk::PipelineStageFlagBits::eFragmentShader,
            vk::DependencyFlagBits::
                eQueueFamilyOwnershipTransferUseAllStagesKHR,  // TODO
            0,
            nullptr,
            0,
            nullptr,
            1,
            &barrier
        );

        if (mipWidth > 1) mipWidth /= 2;
        if (mipHeight > 1) mipHeight /= 2;
    }
    barrier.subresourceRange.baseMipLevel = mipLevels - 1;
    barrier.oldLayout = vk::ImageLayout::eTransferDstOptimal;
    barrier.newLayout = vk::ImageLayout::eReadOnlyOptimal;
    barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
    barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;
    commandBuffer.pipelineBarrier(
        vk::PipelineStageFlagBits::eTransfer,
        vk::PipelineStageFlagBits::eFragmentShader,
        vk::DependencyFlagBits::
            eQueueFamilyOwnershipTransferUseAllStagesKHR,  // TODO
        0,
        nullptr,
        0,
        nullptr,
        1,
        &barrier
    );

    end_single_time_commands(commandBuffer);
}

VulkanRenderDevice::QueueFamilyIndices
VulkanRenderDevice::find_queue_families() {
    QueueFamilyIndices indices;
    auto queueFamilies = gvPhysicalDevice.getQueueFamilyProperties();

    int i = 0;
    for (const auto& queueFamily : queueFamilies) {
        vk::Bool32 presentSupport =
            gvPhysicalDevice.getSurfaceSupportKHR(i, gvSurface);
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
    int width, height;
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
        {{}, code.size(), reinterpret_cast<const uint32_t*>(code.data())}
    );
}
}  // namespace garnish