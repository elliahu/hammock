module;

#include <iostream>
#include <vector>
#include <vulkan/vulkan.hpp>

export module hammock.core.instance;

import hammock.core.utilities;


namespace hammock::core {
    // local callback functions
    static vk::Bool32 debugCallback(
        vk::DebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        vk::DebugUtilsMessageTypeFlagsEXT messageType,
        const vk::DebugUtilsMessengerCallbackDataEXT *pCallbackData,
        void *pUserData) {
        std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;
        return vk::False;
    }

    export class Instance {
#ifdef NDEBUG
        bool enableValidationLayers = false;
#else
        bool enableValidationLayers = true;
#endif

    public:
        Instance() {
            createInstance();
            setupDebugMessenger();
        }

        ~Instance() {
            if (enableValidationLayers && debugMessenger && pfnDestroyDebugUtilsMessengerEXT) {
                // Use the manually loaded function
                pfnDestroyDebugUtilsMessengerEXT(
                    instance,
                    debugMessenger,
                    nullptr);
            }
            if (instance) {
                instance.destroy();
            }
        }

        [[nodiscard]] vk::Instance getInstance() const { return instance; };

    private:
        void setupDebugMessenger() {
            if (!enableValidationLayers) {
                return;
            }

            // Load both functions
            pfnCreateDebugUtilsMessengerEXT =
                reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(
                    instance.getProcAddr("vkCreateDebugUtilsMessengerEXT"));

            pfnDestroyDebugUtilsMessengerEXT =
                reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(
                    instance.getProcAddr("vkDestroyDebugUtilsMessengerEXT"));

            if (!pfnCreateDebugUtilsMessengerEXT) {
                Logger::log(LOG_LEVEL_WARN, "Failed to load vkCreateDebugUtilsMessengerEXT");
                return;
            }

            vk::DebugUtilsMessengerCreateInfoEXT createInfo =
                    populateDebugMessengerCreateInfo();

            VkDebugUtilsMessengerEXT rawMessenger;
            VkResult result = pfnCreateDebugUtilsMessengerEXT(
                instance,
                reinterpret_cast<const VkDebugUtilsMessengerCreateInfoEXT*>(&createInfo),
                nullptr,
                &rawMessenger);

            if (result == VK_SUCCESS) {
                debugMessenger = rawMessenger;
            } else {
                Logger::log(LOG_LEVEL_WARN, "Failed to create debug messenger");
            }
        }

        void createInstance() {
            if (enableValidationLayers && !validationLayersSupported()) {
                Logger::log(
                    LOG_LEVEL_WARN,
                    "Validation layers requested, but not available. Validation layers not used!");
                enableValidationLayers = false;
            }

            vk::ApplicationInfo appInfo{
                .pApplicationName = "hammock::core Engine App",
                .applicationVersion = vk::makeVersion(1, 0, 0),
                .pEngineName = "hammock::core Engine",
                .engineVersion = vk::makeVersion(0, 5, 0),
                .apiVersion = vk::makeApiVersion(0, 1, 3, 0)
            };

            auto extensions = getRequiredExtensions();

            vk::InstanceCreateInfo createInfo{};
            createInfo.pApplicationInfo = &appInfo;
            createInfo.enabledLayerCount = enableValidationLayers
                                               ? static_cast<uint32_t>(validationLayers.size())
                                               : 0;
            createInfo.ppEnabledLayerNames = enableValidationLayers
                                                 ? validationLayers.data()
                                                 : nullptr;
            createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
            createInfo.ppEnabledExtensionNames = extensions.data();


            vk::DebugUtilsMessengerCreateInfoEXT debugCreateInfo;
            if (enableValidationLayers) {
                Logger::log(LOG_LEVEL_INFO, "Validation layers available");
                debugCreateInfo = populateDebugMessengerCreateInfo();
                createInfo.pNext = &debugCreateInfo;
            } else {
                Logger::log(LOG_LEVEL_INFO, "Validation layers unavailable");
                createInfo.pNext = nullptr;
            }

            instance = vk::createInstance(createInfo);
        }

        [[nodiscard]] bool validationLayersSupported() const {
            auto availableLayers =
                    vk::enumerateInstanceLayerProperties();

            for (const char *layerName: validationLayers) {
                bool layerFound = false;

                for (const auto &layerProperties: availableLayers) {
                    if (std::strcmp(layerName, layerProperties.layerName) == 0) {
                        layerFound = true;
                        break;
                    }
                }

                if (!layerFound) {
                    return false;
                }
            }

            return true;
        }

        [[nodiscard]] std::vector<const char *> getRequiredExtensions() const {
            std::vector<const char *> extensions;

            // Common extension for all platforms
            extensions.push_back(vk::KHRSurfaceExtensionName);

#if defined(_WIN32)
            // Add Win32-specific extension
            extensions.push_back(vk::KHRWin32SurfaceExtensionName);
#elif defined(__linux__)
            // We use X11 on linux as Wayland is a dumpster fire and pain to develop for
            extensions.push_back(vk::KhrXlibSurfaceExtensionName);
#endif

            if (enableValidationLayers) {
                extensions.push_back(vk::EXTDebugUtilsExtensionName);
            }

            return extensions;
        }

        static vk::DebugUtilsMessengerCreateInfoEXT
        populateDebugMessengerCreateInfo() {
            auto debug = vk::DebugUtilsMessengerCreateInfoEXT{};
            debug.messageSeverity = vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
                                    vk::DebugUtilsMessageSeverityFlagBitsEXT::eError;
            debug.messageType = vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
                                vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation |
                                vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance;
            debug.pfnUserCallback = debugCallback;
            return debug;
        }

        vk::Instance instance{};
        vk::DebugUtilsMessengerEXT debugMessenger;
        const std::vector<const char *> validationLayers = {"VK_LAYER_KHRONOS_validation"};
        PFN_vkCreateDebugUtilsMessengerEXT pfnCreateDebugUtilsMessengerEXT = nullptr;
        PFN_vkDestroyDebugUtilsMessengerEXT pfnDestroyDebugUtilsMessengerEXT = nullptr;
    };
}
