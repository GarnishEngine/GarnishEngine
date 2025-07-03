#pragma once

#include <stb_image.h>
#include <tiny_obj_loader.h>

#include <Vulkan.hpp>
#include <cstddef>
#include <cstdint>
#include <vector>

#include "read_file.hpp"
#include "render_device.hpp"
namespace garnish {
const std::vector<const char*> validationLayers = {
    "VK_LAYER_KHRONOS_validation"
};

const std::vector<const char*> deviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME,
    "VK_KHR_portability_subset"
};

struct GVMesh {
    uint32_t firstVertex;
    uint32_t vertexCount;
    uint32_t firstIndex;
    uint32_t indexCount;
};

struct GVTexture {
    uint32_t mipLevels;
    vk::Image textureImage;
    vk::DeviceMemory textureMemory;
    vk::ImageView textureImageView;
    vk::Sampler textureFormat;
};

class VulkanRenderDevice : public RenderDevice {
   public:
    VulkanRenderDevice() = default;
    VulkanRenderDevice(const VulkanRenderDevice&) = delete;
    VulkanRenderDevice& operator=(const VulkanRenderDevice&) = delete;
    VulkanRenderDevice(VulkanRenderDevice&&) = delete;
    VulkanRenderDevice& operator=(VulkanRenderDevice&&) = delete;
    ~VulkanRenderDevice() = default;

    bool init(InitInfo& info) override;
    bool init_vulkan();
    void set_size(unsigned int width, unsigned int height) override {}
    bool draw() override { return true; }
    void cleanup() override;
    // uint64_t get_flags() override;
    void update(ECSController& world) override;
    uint32_t setup_mesh(const std::string& mesh_path);
    uint32_t create_texture_image(const std::string& TEXTURE_PATH);

    // void delete_mesh(Mesh mesh);
    // rawmesh load_mesh(const std::string& mesh_path);
    // texture load_texture(const std::string& texture_path);

   private:
    vk::Instance gvInstance;
    vk::DebugUtilsMessengerEXT gvDebugMessenger;
    vk::SurfaceKHR gvSurface;
    vk::PhysicalDevice gvPhysicalDevice;
    vk::Device gvDevice;
    vk::Queue gvGraphicsQueue;
    vk::Queue gvPresentQueue;

    vk::SwapchainKHR gvSwapchainKHR;
    vk::Format gvSwapChainImageFormat{};
    vk::Extent2D gvSwapChainExtent;
    std::vector<vk::Image> swapChainImages;
    std::vector<vk::ImageView> swapChainImageViews;
    std::vector<vk::Framebuffer> swapChainFramebuffers;

    vk::RenderPass gvRenderPass;
    vk::PipelineLayout gvPipelineLayout;
    vk::Pipeline gvPipeline;
    vk::SampleCountFlagBits msaaSamples;

    vk::DescriptorPool gvDescriptorPool;
    vk::DescriptorSetLayout gvDescriptorSetLayout;
    vk::DescriptorSet gvDescriptorSet;
    std::vector<vk::DescriptorSet> descriptorSets;

    vk::CommandPool gvCommandPool;
    std::vector<vk::CommandBuffer> commandBuffers;

    vk::Semaphore gvPresentSemaphore;
    vk::Semaphore gvRenderSemaphore;

    vk::Fence gvRenderFence;

    vk::Image colorImage;
    vk::DeviceMemory colorImageMemory;
    vk::ImageView colorImageView;

    vk::Image depthImage;
    vk::DeviceMemory depthImageMemory;
    vk::ImageView depthImageView;
    // VmaAllocation gvDepthAllocation;
    vk::DeviceSize totalVertexBytes = 0;
    vk::DeviceSize totalIndexBytes = 0;

    vk::Buffer vertexBuffer;
    vk::Buffer indexBuffer;
    vk::DeviceMemory vertexBufferMemory;
    vk::DeviceMemory indexBufferMemory;
    std::vector<GVMesh> gvMeshes;
    std::vector<GVTexture> gvTextures;

    std::vector<vk::Buffer> uniformBuffers;
    std::vector<vk::DeviceMemory> uniformBuffersMemory;
    std::vector<void*> uniformBuffersMapped;

    uint32_t mipLevels = 0;
    uint32_t MAX_FRAMES_IN_FLIGHT = 2;

    // VmaAllocation gvTextureAllocation;

    std::vector<vk::Semaphore> imageAvailableSemaphores;
    std::vector<vk::Semaphore> renderFinishedSemaphores;
    std::vector<vk::Fence> inFlightFences;

    bool framebufferResized = false;
    uint32_t currentFrame = 0;
    const uint32_t maxTextures = 16;
    const bool enableValidationLayers = true;

    struct UniformBufferObject {
        alignas(16) glm::mat4 model;
        alignas(16) glm::mat4 view;
        alignas(16) glm::mat4 proj;
    };
    // UniformBufferObject ubo;

    struct QueueFamilyIndices {
        std::optional<uint32_t> graphicsFamily;
        std::optional<uint32_t> presentFamily;

        bool isComplete() {
            return graphicsFamily.has_value() && presentFamily.has_value();
        }
    };
    QueueFamilyIndices queueFamilyIndicies;

    struct SwapChainSupportDetails {
        vk::SurfaceCapabilitiesKHR capabilities;
        std::vector<vk::SurfaceFormatKHR> formats;
        std::vector<vk::PresentModeKHR> presentModes;
    };

    bool create_instance();
    bool setup_debug_messenger();
    bool create_surface();

    bool pick_physical_device();
    bool check_device_extension_support(vk::PhysicalDevice& device);
    bool is_device_suitable(vk::PhysicalDevice& device);
    [[nodiscard]] vk::SampleCountFlagBits max_usable_sample_count() const;
    bool create_logical_device();

    bool create_swap_chain();
    bool recreate_swap_chain();
    bool cleanup_swap_chain();

    bool create_image_views();
    vk::ImageView create_image_view(
        vk::Image image,
        vk::Format format,
        vk::ImageAspectFlags aspectFlags,
        uint32_t mipLevels
    );

    bool create_render_pass();
    vk::Format find_supported_format(
        const std::vector<vk::Format>& candidates,
        vk::ImageTiling tiling,
        vk::FormatFeatureFlags features
    );
    vk::Format find_depth_format();
    bool create_descriptor_set_layout();
    bool create_graphics_pipeline();
    bool create_command_pool();

    bool create_color_resources();
    bool create_depth_resources();
    bool create_framebuffers();

    bool create_texture_image_view();
    bool create_texture_sampler();
    void update_descriptor_sets();
    void transition_image_layout(
        vk::Image image,
        vk::Format format,
        vk::ImageLayout oldLayout,
        vk::ImageLayout newLayout,
        uint32_t mipLevels
    );
    void create_vertex_buffer();
    void create_index_buffer();
    void create_uniform_buffers();
    void update_uniform_buffer(uint32_t currentImage);

    bool create_descriptor_pool();
    bool create_descriptor_sets();
    bool create_command_buffers();
    bool create_sync_objects();
    void record_command_buffer(
        vk::CommandBuffer commandBuffer,
        uint32_t imageIndex,
        ECSController& world
    );

    void draw_frame(ECSController& world);

    QueueFamilyIndices find_queue_families(vk::PhysicalDevice& device);
    vk::Extent2D create_extent();
    vk::ShaderModule create_shader_module(const std::vector<char>& code);
    vk::CommandBuffer begin_single_time_commands();
    void end_single_time_commands(vk::CommandBuffer& commandBuffer);
    // TODO maybe seperate parameters below into differens structs
    void create_image(
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
    );
    uint32_t
    find_memory_type(uint32_t typeFilter, vk::MemoryPropertyFlags properties);
    void create_buffer(
        vk::DeviceSize size,
        vk::BufferUsageFlags usage,
        vk::MemoryPropertyFlags properties,
        vk::Buffer& buffer,
        vk::DeviceMemory& bufferMemory
    );
    void copy_buffer(
        vk::Buffer srcBuffer,
        vk::Buffer dstBuffer,
        vk::DeviceSize size,
        vk::DeviceSize dstOffset
    );

    void copy_buffer_to_image(
        vk::Buffer buffer,
        vk::Image image,
        uint32_t width,
        uint32_t height
    );
    void generate_mipmaps(
        vk::Image image,
        vk::Format imageFormat,
        int32_t texWidth,
        int32_t texHeight,
        uint32_t mipLevels
    );
};
}  // namespace garnish