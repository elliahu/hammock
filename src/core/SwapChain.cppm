module;

#include <vulkan/vulkan.h>
#include <memory>
#include <functional>

export module hammock.core.swapchain;

import hammock.core.device;
import hammock.core.semaphore;


// TODO change this to contexpr
#define FB_COLOR_FORMAT VK_FORMAT_R8G8B8A8_UNORM

namespace hammock::core {
    export class SwapChain {
    public:
        static constexpr int MAX_FRAMES_IN_FLIGHT = 2;

        SwapChain(Device &deviceRef, VkExtent2D windowExtent);

        SwapChain(Device &deviceRef, VkExtent2D windowExtent, const std::shared_ptr<SwapChain> &previous);

        ~SwapChain();

        SwapChain(const SwapChain &) = delete;

        SwapChain &operator=(const SwapChain &) = delete;

        [[nodiscard]] VkImage getImage(const int index) const { return swapChainImages[index]; }
        [[nodiscard]] VkImageView getImageView(const int index) const { return swapChainImageViews[index]; }
        [[nodiscard]] size_t imageCount() const { return swapChainImages.size(); }
        [[nodiscard]] VkFormat getSwapChainImageFormat() const { return swapChainImageFormat; }
        [[nodiscard]] VkExtent2D getSwapChainExtent() const { return swapChainExtent; }

        [[nodiscard]] VkFormat getSupportedDepthFormat() const;

        VkResult acquireNextImage(uint32_t *imageIndex) const;

        [[nodiscard]] bool compareSwapFormats(const SwapChain &swapChain) const {
            return swapChain.swapChainImageFormat == swapChainImageFormat;
        }

        static void forEachFrameInFlight(const std::function<void(int frame)> &cb) {
            for (int frame = 0; frame < MAX_FRAMES_IN_FLIGHT; frame++) {
                cb(frame);
            }
        }

        [[nodiscard]] Semaphore &getImageAvailableSemaphore() const {
            return *imageAvailableSemaphores[currentFrame];
        }

        [[nodiscard]] Semaphore &getRenderFinishedSemaphore() const {
            return *renderFinishedSemaphores[currentFrame];
        }

        [[nodiscard]] VkFence getInFlightFence() const {
            return inFlightFences[currentFrame];
        }

        [[nodiscard]] uint32_t getCurrentFrame() const {
            return currentFrame;
        }

        VkResult present(uint32_t imageIndex);

        void recordPipelineBarrier(
            std::uint32_t image,
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

        VkFormat swapChainImageFormat;
        VkExtent2D swapChainExtent;


        std::vector<VkImage> swapChainImages;
        std::vector<VkImageView> swapChainImageViews;


        Device &device;
        VkExtent2D windowExtent;

        VkSwapchainKHR swapChain;
        std::shared_ptr<SwapChain> oldSwapChain;

        std::vector<std::unique_ptr<Semaphore> > imageAvailableSemaphores;
        std::vector<std::unique_ptr<Semaphore> > renderFinishedSemaphores;
        std::vector<VkFence> inFlightFences;
        size_t currentFrame = 0;
    };
} // namespace Hmck
