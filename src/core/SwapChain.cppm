module;

#include <vulkan/vulkan.h>
#include <memory>
#include <functional>
#include <array>

export module hammock.core.swapchain;

import hammock.core.device;
import hammock.core.semaphore;

#define FB_COLOR_FORMAT VK_FORMAT_R8G8B8A8_UNORM

namespace hammock::core {
    // Struct to hold per-frame synchronization objects
    export struct FrameSyncObjects {
        std::unique_ptr<Semaphore> imageAvailable;
        std::unique_ptr<Semaphore> renderFinished;
        VkFence inFlightFence;
    };

    export class SwapChain {
    public:
        static constexpr int MAX_FRAMES_IN_FLIGHT = 2;

        SwapChain(Device &deviceRef, VkExtent2D windowExtent);
        SwapChain(Device &deviceRef, VkExtent2D windowExtent, const std::shared_ptr<SwapChain> &previous);
        ~SwapChain();

        SwapChain(const SwapChain &) = delete;
        SwapChain &operator=(const SwapChain &) = delete;

        // Image accessors
        [[nodiscard]] VkImage getImage(const int index) const { return swapChainImages[index]; }
        [[nodiscard]] VkImageView getImageView(const int index) const { return swapChainImageViews[index]; }
        [[nodiscard]] size_t imageCount() const { return swapChainImages.size(); }
        [[nodiscard]] VkFormat getSwapChainImageFormat() const { return swapChainImageFormat; }
        [[nodiscard]] VkExtent2D getSwapChainExtent() const { return swapChainExtent; }
        [[nodiscard]] VkFormat getSupportedDepthFormat() const;

        // Synchronization accessors - now takes frame index as parameter
        [[nodiscard]] FrameSyncObjects& getSyncObjects(uint32_t frameIndex) {
            return frameSyncObjects[frameIndex];
        }

        [[nodiscard]] const FrameSyncObjects& getSyncObjects(uint32_t frameIndex) const {
            return frameSyncObjects[frameIndex];
        }

        // Acquire next image for rendering
        VkResult acquireNextImage(uint32_t frameIndex, uint32_t *imageIndex);

        // Present the rendered image
        VkResult present(uint32_t frameIndex, uint32_t imageIndex);

        // Pipeline barrier helper
        void recordPipelineBarrier(
            std::uint32_t imageIndex,
            VkCommandBuffer cmd,
            VkPipelineStageFlags2 srcStageMask,
            VkAccessFlags2 srcAccessMask,
            VkPipelineStageFlags2 dstStageMask,
            VkAccessFlags2 dstAccessMask,
            VkImageLayout oldLayout,
            VkImageLayout newLayout,
            uint32_t srcQueueFamilyIndex,
            uint32_t dstQueueFamilyIndex
        ) const;

        [[nodiscard]] bool compareSwapFormats(const SwapChain &swapChain) const {
            return swapChain.swapChainImageFormat == swapChainImageFormat;
        }

        static void forEachFrameInFlight(const std::function<void(int frame)> &cb) {
            for (int frame = 0; frame < MAX_FRAMES_IN_FLIGHT; frame++) {
                cb(frame);
            }
        }

    private:
        void init();
        void createSwapChain();
        void createImageViews();
        void createSyncObjects();

        // Helper functions
        static VkSurfaceFormatKHR chooseSwapSurfaceFormat(
            const std::vector<VkSurfaceFormatKHR> &availableFormats);
        static VkPresentModeKHR chooseSwapPresentMode(
            const std::vector<VkPresentModeKHR> &availablePresentModes);
        VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities) const;

        // SwapChain resources
        VkFormat swapChainImageFormat;
        VkExtent2D swapChainExtent;
        std::vector<VkImage> swapChainImages;
        std::vector<VkImageView> swapChainImageViews;

        Device &device;
        VkExtent2D windowExtent;
        VkSwapchainKHR swapChain;
        std::shared_ptr<SwapChain> oldSwapChain;

        // Per-frame synchronization objects (indexed by frame, not by SwapChain)
        std::array<FrameSyncObjects, MAX_FRAMES_IN_FLIGHT> frameSyncObjects;
    };
} // namespace hammock::core
