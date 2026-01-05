module;
#include <vector>
#include <compare>
#include <vulkan/vulkan.hpp>

export module hammock.core.device;

import hammock.core.instance;
import hammock.core.memory_allocator;

namespace hammock::core {
    /// @struct SwapChainSupportDetails
    /// @brief This struct represents detailed info about a swapchain support for the current device
    export struct SwapChainSupportDetails {
        vk::SurfaceCapabilitiesKHR capabilities;
        std::vector<vk::SurfaceFormatKHR> formats;
        std::vector<vk::PresentModeKHR> presentModes;
    };

    /// @struct QueueFamilyIndices
    /// @brief Helper struct for discovering device queue families and its indices
    export struct QueueFamilyIndices {
        uint32_t graphicsFamily;
        uint32_t presentFamily;
        uint32_t computeFamily;
        uint32_t transferFamily;
        bool graphicsFamilyHasValue = false;
        bool presentFamilyHasValue = false;
        bool computeFamilyHasValue = false;
        bool transferFamilyHasValue = false;

        /// @brief check for queue completeness (device has all queue types)
        [[nodiscard]] bool isComplete() const {
            return graphicsFamilyHasValue && presentFamilyHasValue && computeFamilyHasValue && transferFamilyHasValue;
        }
    };

    /// Queue family
    export enum class CommandQueueFamily { Ignored, Graphics, Compute, Transfer };

    /**
     * @class Device
     * @brief This class represents both logical and physical graphics device (GPU).
     * Along with Instance, Device is a core component of a Vulkan application
     */
    export class Device {
    public:
        explicit Device(Instance &instance, vk::SurfaceKHR surface = vk::SurfaceKHR{});

        ~Device();

        // Not copyable or movable
        Device(const Device &) = delete;

        Device &operator=(const Device &) = delete;

        Device(Device &&) = delete;

        Device &operator=(Device &&) = delete;

        /// @brief Get Graphics command pool
        [[nodiscard]] vk::CommandPool getGraphicsCommandPool() const { return graphicsCommandPool_; }

        /// @brief Get transfer command pool
        [[nodiscard]] vk::CommandPool getTransferCommandPool() const { return transferCommandPool_; }

        /// @brief Get compute command pool
        [[nodiscard]] vk::CommandPool getComputeCommandPool() const { return computeCommandPool_; }

        /// @brief Get Vulkan device
        [[nodiscard]] vk::Device device() const { return device_; }

        /// @brief Get Vulkan Memory Allocator (VMA)
        [[nodiscard]] allocator::Allocator allocator() const { return allocator_; }

        /// @brief Get Vulkan physical device
        [[nodiscard]] vk::PhysicalDevice getPhysicalDevice() const { return physicalDevice_; }

        /// @brief Get graphics queue
        [[nodiscard]] vk::Queue graphicsQueue() const { return graphicsQueue_; }

        /// @brief Get graphics queue family index
        [[nodiscard]] uint32_t getGraphicsQueueFamilyIndex() const { return graphicsQueueFamilyIndex_; }

        /// @brief Get present queue
        [[nodiscard]] vk::Queue getPresentQueue() const;

        /// @brief Get compute queue
        [[nodiscard]] vk::Queue getComputeQueue() const { return computeQueue_; }

        /// @brief Get compute queue family index
        [[nodiscard]] uint32_t getComputeQueueFamilyIndex() const { return computeQueueFamilyIndex_; }

        /// @brief Get transfer queue
        [[nodiscard]] vk::Queue getTransferQueue() const { return transferQueue_; }

        /// @brief Get transfer queue family index
        [[nodiscard]] uint32_t getTransferQueueFamilyIndex() const { return transferQueueFamilyIndex_; }

        /// @brief Get physical device properties
        [[nodiscard]] vk::PhysicalDeviceProperties &getPhysicalDeviceProperties() { return physicalDeviceProperties_; }

        /// @brief Get swap chain support details
        [[nodiscard]] SwapChainSupportDetails getSwapChainSupport() const;

        /// @brief Get queue families for the current physical device
        [[nodiscard]] QueueFamilyIndices getPhysicalQueueFamilies() { return findQueueFamilies(physicalDevice_); }

        /// @brief Find memory type that supports the filtered values
        [[nodiscard]] uint32_t findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties) const;

        /// @brief Find supported format.
        /// Returns first format in a list that is supported by the GPU.
        /// @param candidates should be ordered list of candidate formats.
        [[nodiscard]] vk::Format findSupportedFormat(
            const std::vector<vk::Format> &candidates, vk::ImageTiling tiling, vk::FormatFeatureFlags features) const;

        /// @brief Allocates a command buffer from the graphics queue
        /// Call endSingleTimeCommands to submit
        [[nodiscard]] vk::CommandBuffer beginSingleTimeCommands() const;

        /// @brief submits given command buffer into graphics queue
        void endSingleTimeCommands(vk::CommandBuffer commandBuffer) const;

        /// @brief Submits a layout transition barrier into graphics queue
        [[deprecated("Manual pipeline barrier preferred")]]
        void queueImageLayoutTransition(vk::Image image, vk::ImageLayout layoutOld, vk::ImageLayout layoutNew,
                                        uint32_t layerCount = 1, uint32_t baseLayer = 0, uint32_t levelCount = 1,
                                        uint32_t baseLevel = 0,
                                        vk::ImageAspectFlags aspectMask = vk::ImageAspectFlagBits::eColor) const;

        /// @brief Block all GPU executions until all queues are empty
        /// Do not call in frame unless you know what you are doing.
        /// This performs a hard block and may cause stutters and performance problems.
        void waitIdle();

    private:
        void pickPhysicalDevice();

        void createLogicalDevice();

        void createCommandPools();

        void createMemoryAllocator();

        bool isDeviceSuitable(vk::PhysicalDevice device);

        QueueFamilyIndices findQueueFamilies(vk::PhysicalDevice device);

        bool checkDeviceExtensionSupport(vk::PhysicalDevice device) const;

        bool runningHeadless() const { return surface_ == vk::SurfaceKHR{}; }

        SwapChainSupportDetails querySwapChainSupport(vk::PhysicalDevice device) const;

        Instance &instance_;
        vk::PhysicalDevice physicalDevice_;
        vk::CommandPool graphicsCommandPool_;
        vk::CommandPool transferCommandPool_;
        vk::CommandPool computeCommandPool_;
        vk::Device device_;
        vk::SurfaceKHR surface_;
        vk::Queue graphicsQueue_;
        uint32_t graphicsQueueFamilyIndex_;
        vk::Queue presentQueue_;
        vk::Queue transferQueue_;
        uint32_t transferQueueFamilyIndex_;
        vk::Queue computeQueue_;
        uint32_t computeQueueFamilyIndex_;
        allocator::Allocator allocator_;
        vk::PhysicalDeviceProperties physicalDeviceProperties_;

        // Enabled extensions
        const std::vector<const char *> deviceExtensions_ = {
            // SwapChain support
            vk::KHRSwapchainExtensionName,
            // Dynamic rendering is preferred over render passes
            vk::KHRDynamicRenderingExtensionName,
            // Descriptor indexing is used
            vk::EXTDescriptorIndexingExtensionName,

            // These are commented here as most of the recent linux drivers still don't support them even tho on windows they are pretty much standard now
            // Looking at you NVidia ...
            // Unified image layouts
            // vk::KHRUnifiedImageLayoutsExtensionName,
            // SwapChain maintenance enables release fence in swapchain
            // vk::KHRSwapchainMaintenance1ExtensionName,

        };
    };
} // namespace hammock::core
