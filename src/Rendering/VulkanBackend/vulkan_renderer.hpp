#pragma once
#include <cstdint>
#include <span>
#include <vector>
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>

#include "render_device.hpp"
#include "shared.hpp"

namespace garnish::vulkan {
namespace vkr = vk::raii;
const std::vector<const char*> validationLayers = {"VK_LAYER_KHRONOS_validation"};

class VulkanRenderDevice : public RenderDevice {
   public:
    VulkanRenderDevice(const RenderDevice::InitInfo& info);
    VulkanRenderDevice(const VulkanRenderDevice&) = delete;
    VulkanRenderDevice& operator=(const VulkanRenderDevice&) = delete;
    VulkanRenderDevice(VulkanRenderDevice&&) = delete;
    VulkanRenderDevice& operator=(VulkanRenderDevice&&) = delete;
    ~VulkanRenderDevice() { cleanup(); }

    // Lifecycle
    void cleanup() override;

    // Core rendering
    bool draw_frame(ECSController& world) override;
    void update(ECSController& world) override;

    // Resource loading
    using RenderDevice::setup_mesh;
    uint32_t setup_mesh(const Geometry& geometry) override;
    uint32_t load_texture(const std::string& path) override;

   private:
    static constexpr uint32_t kMAX_FRAMES_IN_FLIGHT = 2;
    static constexpr uint32_t kbufferDefaultSize = 1024 * 1024;
    static constexpr bool kEnableValidationLayers = true;
    static constexpr float kSampleRateShadingMinFraction = 0.2F;
    static constexpr size_t kMat4Align = 16;
    // TODO: MAYABE NOT MAKE THESE HARD CODED
    static constexpr std::string_view kVertexShaderPath = "shaders/vert.spv";
    static constexpr std::string_view kFragmentShaderPath = "shaders/frag.spv";
    struct GVMesh {
        uint32_t firstVertex;
        uint32_t vertexCount;
        uint32_t firstIndex;
        uint32_t indexCount;
    };
    struct GVTexture {
        uint32_t mipLevels;
        vkr::Image textureImage;
        vkr::DeviceMemory textureMemory;
        vkr::ImageView textureImageView;
    };
    struct CameraUBO {
        alignas(kMat4Align) glm::mat4 view;
        alignas(kMat4Align) glm::mat4 proj;
    } cameraUbo_{};

    // don't change this to std::string, things need it to be char*
    std::vector<const char*> deviceExtensions_ = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};

    vkr::Context gvContext_;
    vkr::Instance gvInstance_;
    vkr::SurfaceKHR gvSurface_;
    vkr::DebugUtilsMessengerEXT gvDebugMessenger_;

    vk::SampleCountFlagBits msaaSamples_ = vk::SampleCountFlagBits::e1;
    vkr::PhysicalDevice gvPhysicalDevice_;
    vkr::Device gvDevice_;

    static constexpr uint32_t kInitialModelCapacity = 256;
    struct ModelBufferAllocation {
        std::vector<vkr::Buffer> buffers;
        std::vector<vkr::DeviceMemory> memory;
        std::vector<void*> mapped;
        uint32_t capacity;
    } modelBufferAlloc_;

    vkr::Queue gvGraphicsQueue_;
    vkr::Queue gvPresentQueue_;

    vk::Extent2D gvSwapChainExtent_;
    vk::Format gvSwapChainImageFormat_;
    std::vector<vk::Image> swapChainImages_;
    vkr::SwapchainKHR gvSwapchainKHR_;
    std::vector<vkr::ImageView> swapChainImageViews_;

    vkr::Sampler gvTextureSampler_;
    vkr::RenderPass gvRenderPass_;
    vkr::DescriptorSetLayout gvDescriptorSetLayout_;
    vkr::PipelineLayout gvPipelineLayout_;
    vkr::Pipeline gvPipeline_;

    vkr::CommandPool gvCommandPool_;

    struct ColorResources {
        vkr::Image image;
        vkr::DeviceMemory memory;
        vkr::ImageView view;
    } colorResources_;
    struct DepthResources {
        vkr::Image image;
        vkr::DeviceMemory memory;
        vkr::ImageView view;
    } depthResources_;
    std::vector<vkr::Framebuffer> swapChainFramebuffers_;

    struct VertexBufferAllocation {
        vkr::Buffer buffer;
        vkr::DeviceMemory memory;
    } vertexBuffer_;
    struct IndexBufferAllocation {
        vkr::Buffer buffer;
        vkr::DeviceMemory memory;
    } indexBuffer_;
    vk::DeviceSize totalVertexBytes_ = 0;
    vk::DeviceSize totalIndexBytes_ = 0;

    struct UniformBufferAllocation {
        std::vector<vkr::Buffer> buffers;
        std::vector<vkr::DeviceMemory> memory;
        std::vector<void*> mapped;
    } uniformBufferAlloc_;
    vkr::DescriptorPool gvDescriptorPool_;
    std::vector<vkr::DescriptorSet> descriptorSets_;
    std::vector<vkr::CommandBuffer> gvCommandBuffers_;

    std::vector<GVMesh> gvMeshes_;
    std::vector<GVTexture> gvTextures_;
    std::unordered_map<size_t, uint32_t> loadedTextures_;
    uint32_t textureLimit_ = 0;
    vk::PhysicalDeviceDescriptorIndexingFeatures supportedIndexingFeatures_;
    vk::PhysicalDeviceDescriptorIndexingProperties indexingProperties_;
    vk::PhysicalDeviceVulkan12Features vulkan12Features_;

    uint32_t mipLevels = 0;

    std::vector<vkr::Fence> inFlightFences_;
    std::vector<vkr::Semaphore> renderFinishedSemaphores_;
    std::vector<vk::Fence> imageInFlight_;  // Non-owning handles
    std::vector<vkr::Semaphore> imageAvailableSemaphores_;

    bool framebufferResized_ = false;
    uint32_t currentFrame_ = 0;

    // Instance and surface initialization
    vkr::Instance create_instance();
    vkr::SurfaceKHR create_surface();
    vkr::DebugUtilsMessengerEXT setup_debug_messenger();

    // Physical and logical device selection
    struct QueueFamilyIndices {
        std::optional<uint32_t> graphicsFamily;
        std::optional<uint32_t> presentFamily;

        [[nodiscard]] constexpr bool isComplete() const noexcept {
            return graphicsFamily.has_value() && presentFamily.has_value();
        }
    };
    vkr::PhysicalDevice pick_physical_device();
    vkr::Device create_logical_device();
    bool is_device_suitable(const vkr::PhysicalDevice& device);
    bool check_device_extension_support(const vkr::PhysicalDevice& device);
    [[nodiscard]] QueueFamilyIndices find_queue_families(const vkr::PhysicalDevice& device);
    [[nodiscard]] vk::SampleCountFlagBits max_usable_sample_count() const;

    // Swap chain creation and management
    struct SwapChainSupportDetails {
        vk::SurfaceCapabilitiesKHR capabilities;
        std::vector<vk::SurfaceFormatKHR> formats;
        std::vector<vk::PresentModeKHR> presentModes;
    };
    [[nodiscard]] vk::Extent2D create_extent();
    [[nodiscard]] vk::Format create_swap_chain_image_format() const;
    [[nodiscard]] vkr::SwapchainKHR create_swap_chain();
    [[nodiscard]] SwapChainSupportDetails query_swap_chain_support(
        const vk::PhysicalDevice& device
    ) const;
    [[nodiscard]] static vk::PresentModeKHR choose_present_mode(
        std::span<const vk::PresentModeKHR> availableModes
    );
    [[nodiscard]] static constexpr uint32_t choose_image_count(
        const vk::SurfaceCapabilitiesKHR& capabilities
    );
    [[nodiscard]] vk::SwapchainCreateInfoKHR build_swapchain_create_info(
        const SwapChainSupportDetails& support,
        const vk::SurfaceFormatKHR& surfaceFormat,
        const vk::PresentModeKHR& presentMode,
        const QueueFamilyIndices& indices
    ) const;
    bool recreate_swap_chain();
    bool cleanup_swap_chain();

    // Image views
    struct ImageViewCreateParams {
        vk::Image image;
        vk::Format format;
        vk::ImageAspectFlags aspectFlags;
        uint32_t mipLevels;
    };
    [[nodiscard]] std::vector<vkr::ImageView> create_image_views();
    [[nodiscard]] vkr::ImageView create_image_view(const ImageViewCreateParams& params);

    // Texture sampler
    [[nodiscard]] vkr::Sampler create_texture_sampler();
    vkr::ImageView create_texture_image_view();

    // Render pass
    [[nodiscard]] vkr::RenderPass create_render_pass();
    vk::Format find_depth_format();
    vk::Format find_supported_format(
        std::span<const vk::Format> candidates,
        vk::ImageTiling tiling,
        vk::FormatFeatureFlags features
    );

    // Descriptor set layout and pipeline
    vkr::DescriptorSetLayout create_descriptor_set_layout();
    vkr::PipelineLayout create_pipeline_layout();
    vkr::Pipeline create_graphics_pipeline(const std::string& assetPath);
    vkr::ShaderModule create_shader_module(std::span<const char> code);

    // Command pool
    [[nodiscard]] vkr::CommandPool create_command_pool();
    [[nodiscard]] vkr::CommandBuffer begin_single_time_commands();
    void end_single_time_commands(vkr::CommandBuffer& commandBuffer);

    // Color and depth resources
    [[nodiscard]] ColorResources create_color_resources();
    [[nodiscard]] DepthResources create_depth_resources();

    // Framebuffers
    [[nodiscard]] std::vector<vkr::Framebuffer> create_framebuffers();

    // Vertex, index, and uniform buffers
    [[nodiscard]] VertexBufferAllocation create_vertex_buffer();
    [[nodiscard]] IndexBufferAllocation create_index_buffer();
    [[nodiscard]] UniformBufferAllocation create_uniform_buffers();
    [[nodiscard]] ModelBufferAllocation create_model_buffers(uint32_t minCapacity);
    void destroy_model_buffers();
    void ensure_model_capacity(uint32_t requiredModelCount);

    // Descriptor pool and sets
    [[nodiscard]] vkr::DescriptorPool create_descriptor_pool();
    [[nodiscard]] std::vector<vkr::DescriptorSet> create_descriptor_sets();
    void update_descriptor_sets();

    // Command buffers
    [[nodiscard]] std::vector<vkr::CommandBuffer> create_command_buffers();
    void record_command_buffer(
        vkr::CommandBuffer& commandBuffer,
        uint32_t imageIndex,
        ECSController& world
    );

    // Synchronization objects
    [[nodiscard]] std::vector<vkr::Semaphore> create_sync_objects();

    // Low-level resource creation helpers
    struct ImageCreateInfo {
        uint32_t width;
        uint32_t height;
        uint32_t mipLevels;
        vk::SampleCountFlagBits samples;
        vk::Format format;
        vk::ImageTiling tiling;
        vk::ImageUsageFlags usage;
        vk::MemoryPropertyFlags memoryProperties;
    };
    struct ImageAllocation {
        vkr::Image image;
        vkr::DeviceMemory memory;
    };
    struct BufferCreateInfo {
        vk::DeviceSize size;
        vk::BufferUsageFlags usage;
        vk::MemoryPropertyFlags memoryProperties;
    };
    struct BufferAllocation {
        vkr::Buffer buffer;
        vkr::DeviceMemory memory;
    };
    [[nodiscard]] ImageAllocation create_image(const ImageCreateInfo& info);
    [[nodiscard]] BufferAllocation create_buffer(const BufferCreateInfo& info);
    [[nodiscard]] uint32_t
    find_memory_type(uint32_t typeFilter, vk::MemoryPropertyFlags properties);
    template <typename T>
    std::pair<vkr::Buffer, vkr::DeviceMemory> create_staging_buffer(std::span<T> data);

    // Resource transfer and manipulation
    struct ImageLayoutTransitionInfo {
        vk::Image image;
        vk::ImageLayout oldLayout;
        vk::ImageLayout newLayout;
        uint32_t mipLevels;
    };
    struct CopyBufferInfo {
        vk::Buffer src;
        vk::Buffer dst;
        vk::DeviceSize size;
        vk::DeviceSize dstOffset = 0;
    };
    struct CopyBufferToImageInfo {
        vk::Buffer buffer;
        vk::Image image;
        uint32_t width;
        uint32_t height;
    };
    struct TextureSize {
        int32_t width;
        int32_t height;
    };
    struct MipmapGenerationInfo {
        vk::Image image;
        vk::Format format;
        TextureSize size;
        uint32_t mipLevels;
    };
    void transition_image_layout(const ImageLayoutTransitionInfo& info);
    void copy_buffer(const CopyBufferInfo& info);
    void copy_buffer_to_image(const CopyBufferToImageInfo& info);
    void generate_mipmaps(const MipmapGenerationInfo& info);

    // Runtime update functions
    void update_camera_buffer(uint32_t currentImage);
    void update_model_buffer(uint32_t currentImage, std::span<const glm::mat4> models);

    // Historic (deprecated)
    bool init_vulkan(const InitInfo& info);
};  // namespace garnish::vulkan

static vk::VertexInputBindingDescription getBindingDescription() {
    auto bindingDescription = vk::VertexInputBindingDescription{}
                                  .setBinding(0)
                                  .setStride(sizeof(Vertex))
                                  .setInputRate(vk::VertexInputRate::eVertex);
    return bindingDescription;
}

static std::array<vk::VertexInputAttributeDescription, 3> getAttributeDescriptions() {
    std::array<vk::VertexInputAttributeDescription, 3> attributeDescriptions{};
    attributeDescriptions[0] = vk::VertexInputAttributeDescription{}
                                   .setBinding(0)
                                   .setLocation(0)
                                   .setFormat(vk::Format::eR32G32B32Sfloat)
                                   .setOffset(offsetof(Vertex, position));

    attributeDescriptions[1] = vk::VertexInputAttributeDescription{}
                                   .setBinding(0)
                                   .setLocation(1)
                                   .setFormat(vk::Format::eR32G32B32Sfloat)
                                   .setOffset(offsetof(Vertex, normal));
    attributeDescriptions[2] = vk::VertexInputAttributeDescription{}
                                   .setBinding(0)
                                   .setLocation(2)
                                   .setFormat(vk::Format::eR32G32Sfloat)
                                   .setOffset(offsetof(Vertex, uv));
    return attributeDescriptions;
}
}  // namespace garnish::vulkan

namespace std {
template <>
struct hash<Vertex> {
    [[nodiscard]] constexpr size_t operator()(Vertex const& vertex) const noexcept {
        return ((hash<glm::vec3>()(vertex.position) ^ (hash<glm::vec3>()(vertex.normal) << 1)) >>
                1) ^
               (hash<glm::vec2>()(vertex.uv) << 1);
    }
};
}  // namespace std