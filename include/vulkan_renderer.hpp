#pragma once
#include <Vulkan.hpp>

#include "render_device.hpp"

namespace garnish {
struct VkRenderData;
class VulkanRenderDevice : RenderDevice {
   public:
    bool init_vulkan();
    void cleanup();
    bool draw();

   private:
    vk::Instance gInstance;
    VkSurfaceKHR gSurface;
    vk::Device gvDevice;
    vk::Queue gvGraphicsQueue;
    vk::Queue gvPresentQueue;

    vk::Image gvDepthImage;
    vk::ImageView gvDepthImageView;
    vk::Format gvDepthFormat;
    VmaAllocation gvDepthAllocation;

    vk::RenderPass gvRenderPass;
    vk::PipelineLayout gvPipelineLayout;
    vk::Pipeline gvPipeline;

    vk::CommandPool gvCommandPool;
    vk::CommandBuffer gvCommandBuffer;

    vk::Semaphore gvPresentSemaphore;
    vk::Semaphore gvRenderSemaphore;

    vk::Fence gvRenderFence;

    vk::Image gvTextureImage;
    vk::ImageView gvTextureImageView;
    vk::Sampler gvTextureFormat;
    VmaAllocation gvTextureAllocation;

    vk::DescriptorPool gvDescriptorPool;
    vk::DescriptorSetLayout gvDescriptorSetLayout;
    vk::DescriptorSet gvDescriptorSet;

    bool create_instance();
    bool setup_debug_messenger();
    bool create_surface();
    bool pick_physical_device();
    bool create_logical_device();
    bool create_swap_chain();
    bool create_image_views();
    bool create_render_pass();
    bool create_descriptor_set_layout();
    bool create_graphics_pipeline();
    bool create_command_pool();

    // bool create_color_resources();
    bool create_depth_resources();
    bool create_framebuffers();

    bool create_texture_image();
    bool create_texture_image_view();
    bool create_texture_sampler();

    bool create_descriptor_pool();
    bool create_descriptor_sets();
    bool create_command_buffers();
    bool create_sync_objects();
};
}  // namespace garnish