#pragma once
#include <vector>
#include <vulkan/vulkan.hpp>


namespace hammock::core {
    /// @class Instance
    /// @brief Wrapper around vulkan instance
    class Instance final{
#ifdef NDEBUG
        bool enableValidationLayers = false;
#else
        bool enableValidationLayers = true;
#endif

    public:
        Instance();

        ~Instance();

        /// @brief Get the vulkan instance handle
        [[nodiscard]] vk::Instance getInstance() const { return instance_; };

    private:
        void setupDebugMessenger();

        void createInstance();

        [[nodiscard]] bool validationLayersSupported() const;

        [[nodiscard]] std::vector<const char *> getRequiredExtensions() const;

        static vk::DebugUtilsMessengerCreateInfoEXT
        populateDebugMessengerCreateInfo();

        vk::Instance instance_{};
        vk::DebugUtilsMessengerEXT debugMessenger_;
        const std::vector<const char *> validationLayers_ = {"VK_LAYER_KHRONOS_validation"};
        PFN_vkCreateDebugUtilsMessengerEXT pfnCreateDebugUtilsMessengerEXT = nullptr;
        PFN_vkDestroyDebugUtilsMessengerEXT pfnDestroyDebugUtilsMessengerEXT = nullptr;
    };
}
