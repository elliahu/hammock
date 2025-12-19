module;
#include <vector>
#include <stdexcept>
#include <vulkan/vulkan.h>

export module hammock.core.device;

import hammock.core.instance;
import hammock.core.memory_allocator;

namespace hammock::core {
    export struct SwapChainSupportDetails {
        VkSurfaceCapabilitiesKHR capabilities;
        std::vector<VkSurfaceFormatKHR> formats;
        std::vector<VkPresentModeKHR> presentModes;
    };

    export struct QueueFamilyIndices {
        uint32_t graphicsFamily;
        uint32_t presentFamily;
        uint32_t computeFamily;
        uint32_t transferFamily;
        bool graphicsFamilyHasValue = false;
        bool presentFamilyHasValue = false;
        bool computeFamilyHasValue = false;
        bool transferFamilyHasValue = false;

        [[nodiscard]] bool isComplete() const {
            return graphicsFamilyHasValue && presentFamilyHasValue && computeFamilyHasValue && transferFamilyHasValue;
        }
    };

    export enum class CommandQueueFamily { Ignored, Graphics, Compute, Transfer };

    export class Device {
    public:
        Device(Instance &instance, VkSurfaceKHR surface = VK_NULL_HANDLE);

        ~Device();

        // Not copyable or movable
        Device(const Device &) = delete;

        Device &operator=(const Device &) = delete;

        Device(Device &&) = delete;

        Device &operator=(Device &&) = delete;

        [[nodiscard]] VkCommandPool getGraphicsCommandPool() const { return graphicsCommandPool; }
        [[nodiscard]] VkCommandPool getTransferCommandPool() const { return transferCommandPool; }
        [[nodiscard]] VkCommandPool getComputeCommandPool() const { return computeCommandPool; }
        [[nodiscard]] VkDevice device() const { return device_; }
        [[nodiscard]] allocator::Allocator allocator() const { return allocator_; }
        [[nodiscard]] VkInstance getInstance() const { return instance.getInstance(); }
        [[nodiscard]] VkPhysicalDevice getPhysicalDevice() const { return physicalDevice; }
        [[nodiscard]] VkSurfaceKHR surface() const { return surface_; }
        [[nodiscard]] VkQueue graphicsQueue() const { return graphicsQueue_; }
        [[nodiscard]] uint32_t getGraphicsQueueFamilyIndex() const { return graphicsQueueFamilyIndex_; }
        [[nodiscard]] VkPhysicalDeviceProperties &getPhysicalDeviceProperties() { return physicalDeviceProperties; }

        [[nodiscard]] VkQueue presentQueue() const {
            if (runningHeadless()) {
                throw std::runtime_error("Present queue unavailable in headless mode");
            }
            return presentQueue_;
        }

        [[nodiscard]] VkQueue computeQueue() const { return computeQueue_; }
        [[nodiscard]] uint32_t getComputeQueueFamilyIndex() const { return computeQueueFamilyIndex_; }
        [[nodiscard]] VkQueue transferQueue() const { return transferQueue_; }
        [[nodiscard]] uint32_t getTransferQueueFamilyIndex() const { return transferQueueFamilyIndex_; }

        [[nodiscard]] SwapChainSupportDetails getSwapChainSupport() const {
            return querySwapChainSupport(physicalDevice);
        }

        [[nodiscard]] uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) const;

        [[nodiscard]] QueueFamilyIndices findPhysicalQueueFamilies() { return findQueueFamilies(physicalDevice); }

        [[nodiscard]] VkFormat findSupportedFormat(
            const std::vector<VkFormat> &candidates, VkImageTiling tiling, VkFormatFeatureFlags features) const;


        [[nodiscard]] VkCommandBuffer beginSingleTimeCommands() const;

        void endSingleTimeCommands(VkCommandBuffer commandBuffer) const;


        void transitionImageLayout(VkImage image, VkImageLayout layoutOld, VkImageLayout layoutNew,
                                   uint32_t layerCount = 1, uint32_t baseLayer = 0, uint32_t levelCount = 1,
                                   uint32_t baseLevel = 0,
                                   VkImageAspectFlags aspectMask = VK_IMAGE_ASPECT_COLOR_BIT) const;


        void waitIdle();

    private:
        void pickPhysicalDevice();

        void createLogicalDevice();

        void createCommandPools();

        void createMemoryAllocator();

        bool isDeviceSuitable(VkPhysicalDevice device);

        QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);

        bool checkDeviceExtensionSupport(VkPhysicalDevice device) const;

        bool runningHeadless() const { return surface_ == VK_NULL_HANDLE; }

        SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device) const;

        Instance &instance;
        VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
        VkCommandPool graphicsCommandPool;
        VkCommandPool transferCommandPool;
        VkCommandPool computeCommandPool;

        VkDevice device_;
        VkSurfaceKHR surface_;
        VkQueue graphicsQueue_;
        uint32_t graphicsQueueFamilyIndex_;
        VkQueue presentQueue_;
        VkQueue transferQueue_;
        uint32_t transferQueueFamilyIndex_;
        VkQueue computeQueue_;
        uint32_t computeQueueFamilyIndex_;

        allocator::Allocator allocator_;

        const std::vector<const char *> deviceExtensions = {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME,
            VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME,
            VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME,
        };

        VkPhysicalDeviceProperties physicalDeviceProperties;
    };
} // namespace hammock::core
