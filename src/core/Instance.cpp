module;

#include <iostream>
#include <compare>
#include <vulkan/vulkan.hpp>


module hammock_core.instance;

hammock::core::Instance::Instance() {
    createInstance();
    setupDebugMessenger();
}

hammock::core::Instance::~Instance() {
    if (enableValidationLayers && debugMessenger_ && pfnDestroyDebugUtilsMessengerEXT) {
        // Use the manually loaded function
        pfnDestroyDebugUtilsMessengerEXT(
            instance_,
            debugMessenger_,
            nullptr);
    }
    if (instance_) {
        instance_.destroy();
    }
}

void hammock::core::Instance::setupDebugMessenger() {
    if (!enableValidationLayers) {
        return;
    }

    // Load both functions
    pfnCreateDebugUtilsMessengerEXT =
            reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(
                instance_.getProcAddr("vkCreateDebugUtilsMessengerEXT"));

    pfnDestroyDebugUtilsMessengerEXT =
            reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(
                instance_.getProcAddr("vkDestroyDebugUtilsMessengerEXT"));

    if (!pfnCreateDebugUtilsMessengerEXT) {
        Logger::log(LOG_LEVEL_WARN, "Failed to load vkCreateDebugUtilsMessengerEXT");
        return;
    }

    vk::DebugUtilsMessengerCreateInfoEXT createInfo =
            populateDebugMessengerCreateInfo();

    VkDebugUtilsMessengerEXT rawMessenger;
    VkResult result = pfnCreateDebugUtilsMessengerEXT(
        instance_,
        reinterpret_cast<const VkDebugUtilsMessengerCreateInfoEXT*>(&createInfo),
        nullptr,
        &rawMessenger);

    if (result == VK_SUCCESS) {
        debugMessenger_ = rawMessenger;
    } else {
        Logger::log(LOG_LEVEL_WARN, "Failed to create debug messenger");
    }
}

void hammock::core::Instance::createInstance() {
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
                                       ? static_cast<uint32_t>(validationLayers_.size())
                                       : 0;
    createInfo.ppEnabledLayerNames = enableValidationLayers
                                         ? validationLayers_.data()
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

    instance_ = vk::createInstance(createInfo);
}

bool hammock::core::Instance::validationLayersSupported() const {
    auto availableLayers =
            vk::enumerateInstanceLayerProperties();

    for (const char *layerName: validationLayers_) {
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

std::vector<const char *> hammock::core::Instance::getRequiredExtensions() const {
    std::vector<const char *> extensions;

    // Common extension for all platforms
    extensions.push_back(vk::KHRSurfaceExtensionName);
    extensions.push_back(vk::KHRGetSurfaceCapabilities2ExtensionName);
    // For release semaphore on swapchain
    extensions.push_back(vk::EXTSurfaceMaintenance1ExtensionName);

#if defined(_WIN32)
    // Add Win32-specific extension
    extensions.push_back(vk::KHRWin32SurfaceExtensionName);
#elif defined(__linux__)
    // We use X11 on linux as Wayland is a dumpster fire and pain to develop for
    extensions.push_back(vk::KHRXlibSurfaceExtensionName);
#endif

    if (enableValidationLayers) {
        extensions.push_back(vk::EXTDebugUtilsExtensionName);
    }

    return extensions;
}

/// @brief debug callback function for collecting messages from the API
static vk::Bool32 debugCallback(
    vk::DebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    vk::DebugUtilsMessageTypeFlagsEXT messageType,
    const vk::DebugUtilsMessengerCallbackDataEXT *pCallbackData,
    void *pUserData) {
    std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;
    return vk::False;
}

vk::DebugUtilsMessengerCreateInfoEXT hammock::core::Instance::populateDebugMessengerCreateInfo() {
    auto debug = vk::DebugUtilsMessengerCreateInfoEXT{};
    debug.messageSeverity = vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
                            vk::DebugUtilsMessageSeverityFlagBitsEXT::eError;
    debug.messageType = vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
                        vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation |
                        vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance;
    debug.pfnUserCallback = debugCallback;
    return debug;
}
