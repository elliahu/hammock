module;

#include <memory>
#include <functional>
#include <array>
#include <vulkan/vulkan.hpp>

export module hammock.core.swapchain;

import hammock.core.device;
import hammock.core.semaphore;

#define FB_COLOR_FORMAT VK_FORMAT_R8G8B8A8_UNORM

namespace hammock::core {
    // Struct to hold per-frame synchronization objects
    export struct FrameSyncObjects {
        std::unique_ptr<Semaphore> imageAvailable;
        std::unique_ptr<Semaphore> renderFinished;
        vk::Fence inFlightFence;
    };

    export class SwapChain {
    public:
        static constexpr int MAX_FRAMES_IN_FLIGHT = 2;

        SwapChain(Device &deviceRef, vk::Extent2D windowExtent);
        SwapChain(Device &deviceRef, vk::Extent2D windowExtent, const std::shared_ptr<SwapChain> &previous);
        ~SwapChain();

        SwapChain(const SwapChain &) = delete;
        SwapChain &operator=(const SwapChain &) = delete;

        // Image accessors
        [[nodiscard]] vk::Image getImage(const int index) const { return swapChainImages[index]; }
        [[nodiscard]] vk::ImageView getImageView(const int index) const { return swapChainImageViews[index]; }
        [[nodiscard]] size_t imageCount() const { return swapChainImages.size(); }
        [[nodiscard]] vk::Format getSwapChainImageFormat() const { return swapChainImageFormat; }
        [[nodiscard]] vk::Extent2D getSwapChainExtent() const { return swapChainExtent; }
        [[nodiscard]] vk::Format getSupportedDepthFormat() const;

        // Synchronization accessors - now takes frame index as parameter
        [[nodiscard]] FrameSyncObjects& getSyncObjects(uint32_t frameIndex) {
            return frameSyncObjects[frameIndex];
        }

        [[nodiscard]] const FrameSyncObjects& getSyncObjects(uint32_t frameIndex) const {
            return frameSyncObjects[frameIndex];
        }

        // Acquire next image for rendering
        vk::Result acquireNextImage(uint32_t frameIndex, uint32_t *imageIndex);

        // Present the rendered image
        vk::Result present(uint32_t frameIndex, uint32_t imageIndex);

        // Pipeline barrier helper
        void recordPipelineBarrier(
            std::uint32_t imageIndex,
            vk::CommandBuffer cmd,
            vk::PipelineStageFlags2 srcStageMask,
            vk::AccessFlags2 srcAccessMask,
            vk::PipelineStageFlags2 dstStageMask,
            vk::AccessFlags2 dstAccessMask,
            vk::ImageLayout oldLayout,
            vk::ImageLayout newLayout,
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
        static vk::SurfaceFormatKHR chooseSwapSurfaceFormat(
            const std::vector<vk::SurfaceFormatKHR> &availableFormats);
        static vk::PresentModeKHR chooseSwapPresentMode(
            const std::vector<vk::PresentModeKHR> &availablePresentModes);
        vk::Extent2D chooseSwapExtent(const vk::SurfaceCapabilitiesKHR &capabilities) const;

        // SwapChain resources
        vk::Format swapChainImageFormat;
        vk::Extent2D swapChainExtent;
        std::vector<vk::Image> swapChainImages;
        std::vector<vk::ImageView> swapChainImageViews;

        Device &device;
        vk::Extent2D windowExtent;
        vk::SwapchainKHR swapChain;
        std::shared_ptr<SwapChain> oldSwapChain;

        // Per-frame synchronization objects (indexed by frame, not by SwapChain)
        std::array<FrameSyncObjects, MAX_FRAMES_IN_FLIGHT> frameSyncObjects;
    };
} // namespace hammock::core
