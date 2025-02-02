#include "Vulkan.hpp"


// idk where to put surface creation
namespace garnish::vk {
    class gvr_device {
        public:
        void createInstance(VkInstanceCreateInfo createInfo, VkApplicationInfo appInfo, bool enableValidationLayers = false);

        gvr_device() = delete;
        ~gvr_device();

        gvr_device(gvr_device&&) = delete; 
        gvr_device& operator=(gvr_device&&) = delete;

        private:
            VkInstance instance;
            // VkSurfaceKHR surface;

            VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
            VkDevice device;

            VkQueue graphicsQueue;
            VkQueue presentQueue;

            bool validationLayersEnabled = false;
            std::vector<const char*> validationLayers;
            VkDebugUtilsMessengerEXT debugMessenger;        

            void createSurface();

            void pickPhysicalDevice();
            void createLogicalDevice();

            void setupDebugMessenger();
            std::vector<const char*> getRequiredExtensions();
    };
}
