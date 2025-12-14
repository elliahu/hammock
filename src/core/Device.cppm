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
        Device(Instance &instance, VkSurfaceKHR surface);

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
        [[nodiscard]] VkQueue presentQueue() const { return presentQueue_; }
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

        // Buffer Helper Functions
        void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties,
                          VkBuffer &buffer, VkDeviceMemory &bufferMemory) const;

        [[nodiscard]] VkCommandBuffer beginSingleTimeCommands() const;

        void endSingleTimeCommands(VkCommandBuffer commandBuffer) const;

        void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) const;

        void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height, uint32_t layerCount,
                               uint32_t baseArrayLayer = 0, uint32_t depth = 1) const;

        // Image helper functionss
        void createImageWithInfo(const VkImageCreateInfo &imageInfo, VkMemoryPropertyFlags properties, VkImage &image,
                                 VkDeviceMemory &imageMemory) const;

        void transitionImageLayout(VkImage image, VkImageLayout layoutOld, VkImageLayout layoutNew,
                                   uint32_t layerCount = 1, uint32_t baseLayer = 0, uint32_t levelCount = 1,
                                   uint32_t baseLevel = 0,
                                   VkImageAspectFlags aspectMask = VK_IMAGE_ASPECT_COLOR_BIT) const;

        // TODO this should be named copyImageToImage
        // only used in env generator atm
        void copyImageToHostVisibleImage(VkImage srcImage, VkImage dstImage, uint32_t width, uint32_t height) const;

        void waitIdle();

        template<CommandQueueFamily Queue>
        [[nodiscard]] inline std::vector<VkCommandBuffer> createVulkanCommandBuffers(const uint32_t commandBufferCount) const {
            VkCommandBufferAllocateInfo allocInfo{};
            allocInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
            allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
            allocInfo.commandBufferCount = commandBufferCount;
            if constexpr (Queue == CommandQueueFamily::Graphics) {
                allocInfo.commandPool = getGraphicsCommandPool();
            }
            if constexpr  (Queue == CommandQueueFamily::Compute) {
                allocInfo.commandPool = getComputeCommandPool();
            }
            if constexpr  (Queue == CommandQueueFamily::Transfer) {
                allocInfo.commandPool = getTransferCommandPool();
            }

            std::vector<VkCommandBuffer> commandBuffers{};
            commandBuffers.resize(commandBufferCount);

            if (vkAllocateCommandBuffers(device_, &allocInfo, commandBuffers.data()) != VkResult::VK_SUCCESS) {
                throw std::runtime_error("failed to allocate command buffer");
            }
            return commandBuffers;
        }

        // Begins a command buffer
        static auto beginVulkanCommandBuffer(VkCommandBuffer commandBuffer) -> void;

        // Submits and ends command buffers, waits on semaphores and signals semaphores
        static void submitVulkanCommandBuffers(VkQueue queue, uint32_t waitSemaphoreInfoCount,
                                               const VkSemaphoreSubmitInfo *pWaitSemaphoreInfos,
                                               uint32_t commandBufferInfoCount,
                                               const VkCommandBufferSubmitInfo *pCommandBufferInfos,
                                               uint32_t signalSemaphoreInfoCount,
                                               const VkSemaphoreSubmitInfo *pSignalSemaphoreInfos);

        // TODO should be encapsed and accessed by getter
        VkPhysicalDeviceProperties properties;

    private:
        void pickPhysicalDevice();

        void createLogicalDevice();

        void createCommandPools();

        void createMemoryAllocator();

        // helper functions
        bool isDeviceSuitable(VkPhysicalDevice device);

        QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);

        bool checkDeviceExtensionSupport(VkPhysicalDevice device) const;

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
    };
} // namespace hammock::core
