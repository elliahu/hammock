module;

#include <iostream>
#include <vector>

export module hammock.core.instance;

import hammock.core.utilities;
import vulkan_hpp;



namespace hammock::core {
    // local callback functions
    static vk::Bool32 debugCallback(
    vk::DebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    vk::DebugUtilsMessageTypeFlagsEXT messageType,
    const vk::DebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData)
    {
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
            if (enableValidationLayers && debugMessenger) {
                instance.destroyDebugUtilsMessengerEXT(debugMessenger);
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

            vk::DebugUtilsMessengerCreateInfoEXT createInfo =
                populateDebugMessengerCreateInfo();

            debugMessenger =
                instance.createDebugUtilsMessengerEXT(createInfo);
        }

        void createInstance() {
            if (enableValidationLayers && !validationLayersSupported()) {
                Logger::log(
                    LOG_LEVEL_WARN,
                    "Validation layers requested, but not available. Validation layers not used!");
                enableValidationLayers = false;
            }

            vk::ApplicationInfo appInfo{
                "hammock::core Engine App",
                vk::makeVersion( 1, 0, 0),
                "hammock::core Engine",
                vk::makeVersion( 1, 0, 0),
                vk::makeApiVersion(0,1,3,0)
            };

            auto extensions = getRequiredExtensions();

            vk::InstanceCreateInfo createInfo{
                    {},
                    &appInfo,
                    enableValidationLayers
                        ? static_cast<uint32_t>(validationLayers.size())
                        : 0,
                    enableValidationLayers
                        ? validationLayers.data()
                        : nullptr,
                    static_cast<uint32_t>(extensions.size()),
                    extensions.data()
                };

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

            for (const char* layerName : validationLayers) {
                bool layerFound = false;

                for (const auto& layerProperties : availableLayers) {
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
            return vk::DebugUtilsMessengerCreateInfoEXT{
                    {},
                    vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
                    vk::DebugUtilsMessageSeverityFlagBitsEXT::eError,
                    vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
                    vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation |
                    vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance,
                    debugCallback,
                    nullptr
                };
        }

        vk::Instance instance{};
        vk::DebugUtilsMessengerEXT debugMessenger;
        const std::vector<const char *> validationLayers = {"VK_LAYER_KHRONOS_validation"};
    };
}
