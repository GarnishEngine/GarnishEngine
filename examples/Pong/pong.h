#include <garnish_app.hpp>
#include <limits>
#include <memory>
#include <shared.hpp>

const int32_t FRAME_RATE = 90;

using namespace garnish;

/// error: invalid conversion from vk::Bool32
/// (*)(vk::DebugUtilsMessageSeverityFlagBitsEXT,
/// vk::DebugUtilsMessageTypeFlagsEXT, const
/// vk::DebugUtilsMessengerCallbackDataEXT*, void*)
/// {aka unsigned int
/// (*)(vk::DebugUtilsMessageSeverityFlagBitsEXT,
/// vk::Flags<vk::DebugUtilsMessageTypeFlagBitsEXT>, const
/// vk::DebugUtilsMessengerCallbackDataEXT*, void*)} to
/// PFN_vkDebugUtilsMessengerCallbackEXT
/// {aka unsigned int
/// (*)(VkDebugUtilsMessageSeverityFlagBitsEXT,
/// unsigned int, const VkDebugUtilsMessengerCallbackDataEXT*, void*)}