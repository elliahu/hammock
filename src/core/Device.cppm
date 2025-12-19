module;
#include <vector>
#include <stdexcept>

export module hammock.core.device;

import hammock.core.instance;
import hammock.core.memory_allocator;
import vulkan_hpp;

namespace hammock::core {
    export struct SwapChainSupportDetails {
        vk::SurfaceCapabilitiesKHR capabilities;
        std::vector<vk::SurfaceFormatKHR> formats;
        std::vector<vk::PresentModeKHR> presentModes;
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
        Device(Instance &instance, vk::SurfaceKHR surface = vk::SurfaceKHR{});

        ~Device();

        // Not copyable or movable
        Device(const Device &) = delete;

        Device &operator=(const Device &) = delete;

        Device(Device &&) = delete;

        Device &operator=(Device &&) = delete;

        [[nodiscard]] vk::CommandPool getGraphicsCommandPool() const { return graphicsCommandPool; }
        [[nodiscard]] vk::CommandPool getTransferCommandPool() const { return transferCommandPool; }
        [[nodiscard]] vk::CommandPool getComputeCommandPool() const { return computeCommandPool; }
        [[nodiscard]] vk::Device device() const { return device_; }
        [[nodiscard]] allocator::Allocator allocator() const { return allocator_; }
        [[nodiscard]] vk::Instance getInstance() const { return instance.getInstance(); }
        [[nodiscard]] vk::PhysicalDevice getPhysicalDevice() const { return physicalDevice; }
        [[nodiscard]] vk::SurfaceKHR surface() const { return surface_; }
        [[nodiscard]] vk::Queue graphicsQueue() const { return graphicsQueue_; }
        [[nodiscard]] uint32_t getGraphicsQueueFamilyIndex() const { return graphicsQueueFamilyIndex_; }
        [[nodiscard]] vk::PhysicalDeviceProperties &getPhysicalDeviceProperties() { return physicalDeviceProperties; }

        [[nodiscard]] vk::Queue presentQueue() const {
            if (runningHeadless()) {
                throw std::runtime_error("Present queue unavailable in headless mode");
            }
            return presentQueue_;
        }

        [[nodiscard]] vk::Queue computeQueue() const { return computeQueue_; }
        [[nodiscard]] uint32_t getComputeQueueFamilyIndex() const { return computeQueueFamilyIndex_; }
        [[nodiscard]] vk::Queue transferQueue() const { return transferQueue_; }
        [[nodiscard]] uint32_t getTransferQueueFamilyIndex() const { return transferQueueFamilyIndex_; }

        [[nodiscard]] SwapChainSupportDetails getSwapChainSupport() const {
            return querySwapChainSupport(physicalDevice);
        }

        [[nodiscard]] uint32_t findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties) const;

        [[nodiscard]] QueueFamilyIndices findPhysicalQueueFamilies() { return findQueueFamilies(physicalDevice); }

        [[nodiscard]] vk::Format findSupportedFormat(
            const std::vector<vk::Format> &candidates, vk::ImageTiling tiling, vk::FormatFeatureFlags features) const;


        [[nodiscard]] vk::CommandBuffer beginSingleTimeCommands() const;

        void endSingleTimeCommands(vk::CommandBuffer commandBuffer) const;


        void transitionImageLayout(vk::Image image, vk::ImageLayout layoutOld, vk::ImageLayout layoutNew,
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
