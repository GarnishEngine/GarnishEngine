#include "Vulkan.hpp"

namespace garnish {
    class instance {

        

        void createInstance();
        std::vector<const char*> getRequiredExtensions();
        private:
            VkInstanceCreateInfo createInfo{};
            VkApplicationInfo appInfo{};
            std::vector<const char*> validationLayers;
            bool enableValidationLayers = false;

            bool checkValidationLayerSupport();


    };
}