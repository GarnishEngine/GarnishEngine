#pragma once
#include <cstdint>
#include <shared.hpp>
#include <vector>
#include <vulkan/vulkan.hpp>

#include "render_device.hpp"

namespace garnish {
const std::vector<const char*> validationLayers = {
    "VK_LAYER_KHRONOS_validation"
};

struct TextureSize { int32_t width; int32_t height; };

class VulkanRenderDevice : public RenderDevice {
   public:
    VulkanRenderDevice() = default;
    VulkanRenderDevice(const VulkanRenderDevice&) = delete;
    VulkanRenderDevice& operator=(const VulkanRenderDevice&) = delete;
    VulkanRenderDevice(VulkanRenderDevice&&) = delete;
    VulkanRenderDevice& operator=(VulkanRenderDevice&&) = delete;
    ~VulkanRenderDevice() = default;

    bool init(InitInfo& info) override;
    bool draw_frame(ECSController& world) override;
    void cleanup() override;
    void update(ECSController& world) override;
    uint32_t setup_mesh(const std::string& mesh_path) override;
    uint32_t load_texture(const std::string& path) override;

   private:
    std::vector<const char*> deviceExtensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME
    };
    const uint32_t MAX_FRAMES_IN_FLIGHT = 2;
    const uint32_t bufferDefaultSize = 1024 * 1024;
    const bool enableValidationLayers = true;
    static constexpr float kSampleRateShadingMinFraction = 0.2F;
    static constexpr size_t kMat4Align = 16;

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
    };

    struct QueueFamilyIndices {
        std::optional<uint32_t> graphicsFamily;
        std::optional<uint32_t> presentFamily;

        bool isComplete() {
            return graphicsFamily.has_value() && presentFamily.has_value();
        }
    };
    struct SwapChainSupportDetails {
        vk::SurfaceCapabilitiesKHR capabilities;
        std::vector<vk::SurfaceFormatKHR> formats;
        std::vector<vk::PresentModeKHR> presentModes;
    };
    struct CameraUBO {
        alignas(kMat4Align) glm::mat4 view;
        alignas(kMat4Align) glm::mat4 proj;
    };
    CameraUBO cameraUbo{};

    std::vector<vk::Buffer> modelBuffers;
    std::vector<vk::DeviceMemory> modelBuffersMemory;
    std::vector<void*> modelBuffersMapped;
    uint32_t modelBufferCapacity = 0;
    static constexpr uint32_t kInitialModelCapacity = 256;

    vk::DebugUtilsMessengerEXT gvDebugMessenger;

    vk::Instance gvInstance;
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
    vk::SampleCountFlagBits msaaSamples = vk::SampleCountFlagBits::e1;

    vk::DescriptorPool gvDescriptorPool;
    vk::DescriptorSetLayout gvDescriptorSetLayout;
    vk::DescriptorSet gvDescriptorSet;
    std::vector<vk::DescriptorSet> descriptorSets;

    vk::CommandPool gvCommandPool;
    std::vector<vk::CommandBuffer> gvCommandBuffers;

    vk::Semaphore gvPresentSemaphore;
    vk::Semaphore gvRenderSemaphore;

    vk::Fence gvRenderFence;

    vk::Image colorImage;
    vk::DeviceMemory colorImageMemory;
    vk::ImageView colorImageView;

    vk::Image depthImage;
    vk::DeviceMemory depthImageMemory;
    vk::ImageView depthImageView;
    vk::DeviceSize totalVertexBytes = 0;
    vk::DeviceSize totalIndexBytes = 0;

    vk::Buffer vertexBuffer;
    vk::Buffer indexBuffer;
    vk::DeviceMemory vertexBufferMemory;
    vk::DeviceMemory indexBufferMemory;

    vk::Sampler gvTextureSampler;
    std::vector<GVMesh> gvMeshes;
    std::vector<GVTexture> gvTextures;
    std::unordered_map<size_t, uint32_t> loadedTextures;
    uint32_t textureLimit = 0;

    std::vector<vk::Buffer> uniformBuffers;
    std::vector<vk::DeviceMemory> uniformBuffersMemory;
    std::vector<void*> uniformBuffersMapped;

    uint32_t mipLevels = 0;

    std::vector<vk::Semaphore> imageAvailableSemaphores;
    std::vector<vk::Semaphore> renderFinishedSemaphores;
    std::vector<vk::Fence> inFlightFences;
    std::vector<vk::Fence> imageInFlight;

    bool framebufferResized = false;
    uint32_t currentFrame = 0;

    bool init_vulkan();

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
    void create_model_buffers(uint32_t minCapacity);
    void destroy_model_buffers();
    void ensure_model_capacity(uint32_t requiredModelCount);
    void update_camera_buffer(uint32_t currentImage);
    void update_model_buffer(uint32_t currentImage, const std::vector<glm::mat4>& models);

    bool create_descriptor_pool();
    bool create_descriptor_sets();
    bool create_command_buffers();
    bool create_sync_objects();
    void record_command_buffer(
        vk::CommandBuffer commandBuffer,
        uint32_t imageIndex,
        ECSController& world
    );

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
        TextureSize size,
        uint32_t mipLevels
    );
};

struct GVVertex3d {
    glm::vec3 pos;
    glm::vec3 color;
    glm::vec2 texCoord;

    static vk::VertexInputBindingDescription getBindingDescription() {
        vk::VertexInputBindingDescription bindingDescription{};
        bindingDescription.binding = 0;
        bindingDescription.stride = sizeof(GVVertex3d);
        bindingDescription.inputRate = vk::VertexInputRate::eVertex;

        return bindingDescription;
    }

    static std::array<vk::VertexInputAttributeDescription, 3>
    getAttributeDescriptions() {
        std::array<vk::VertexInputAttributeDescription, 3>
            attributeDescriptions{};

        attributeDescriptions[0].binding = 0;
        attributeDescriptions[0].location = 0;
        attributeDescriptions[0].format = vk::Format::eR32G32B32Sfloat;
        attributeDescriptions[0].offset = offsetof(GVVertex3d, pos);

        attributeDescriptions[1].binding = 0;
        attributeDescriptions[1].location = 1;
        attributeDescriptions[1].format = vk::Format::eR32G32B32Sfloat;
        attributeDescriptions[1].offset = offsetof(GVVertex3d, color);

        attributeDescriptions[2].binding = 0;
        attributeDescriptions[2].location = 2;
        attributeDescriptions[2].format = vk::Format::eR32G32Sfloat;
        attributeDescriptions[2].offset = offsetof(GVVertex3d, texCoord);

        return attributeDescriptions;
    }
    bool operator==(const GVVertex3d& other) const {
        return pos == other.pos && color == other.color &&
               texCoord == other.texCoord;
    }
};
}  // namespace garnish

namespace std {
template <>
struct hash<garnish::GVVertex3d> {
    size_t operator()(garnish::GVVertex3d const& vertex) const {
        return ((hash<glm::vec3>()(vertex.pos) ^
                 (hash<glm::vec3>()(vertex.color) << 1)) >>
                1) ^
               (hash<glm::vec2>()(vertex.texCoord) << 1);
    }
};
}  // namespace std