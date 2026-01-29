#pragma once
#include <memory>
#include <functional>
#include <array>
#include <compare>
#include <vulkan/vulkan.hpp>

#include "device.hpp"
#include "semaphore.hpp"

#define FB_COLOR_FORMAT VK_FORMAT_R8G8B8A8_UNORM

namespace hammock::core {
    /// @struct FrameSyncObjects
    /// @brief Struct to hold per-frame synchronization objects
    struct FrameSyncObjects {
        std::unique_ptr<Semaphore> imageAvailable;
        std::unique_ptr<Semaphore> frameFinished;
        vk::Fence inFlightFence;
        vk::Fence releaseFence;
    };

    /// @class SwapChain
    /// @brief Wrapper around a vulkan swapchain.
    /// On its own, it does not do much but with SwapChainManager, it is used to handle image submition and presentation.
    class SwapChain {
    public:
        /// Describes how many images are being worked at while different image is being presented
        static constexpr int MAX_FRAMES_IN_FLIGHT = 2;

        SwapChain(Device &deviceRef, vk::SurfaceKHR surface, vk::Extent2D windowExtent);
        SwapChain(Device &deviceRef, vk::SurfaceKHR surface, vk::Extent2D windowExtent, const std::shared_ptr<SwapChain> &previous);
        ~SwapChain();

        SwapChain(const SwapChain &) = delete;
        SwapChain &operator=(const SwapChain &) = delete;

        /// @brief Get Vulkan handle of the given swapchain image
        [[nodiscard]] vk::Image getImage(const int index) const { return swapChainImages_[index]; }

        /// @brief Get vulkan handle of the given swapchain image view
        [[nodiscard]] vk::ImageView getImageView(const int index) const { return swapChainImageViews_[index]; }

        /// @brief Get the count of images (usually MAX_FRAMES_IN_FLIGHT + 1, not guaranteed)
        [[nodiscard]] size_t imageCount() const { return swapChainImages_.size(); }

        /// @brief Get image format of the swapchain images
        [[nodiscard]] vk::Format getSwapChainImageFormat() const { return swapChainImageFormat_; }

        /// @brief Get the extent of the swapchain images
        [[nodiscard]] vk::Extent2D getExtent() const { return swapChainExtent_; }

        /// @brief Get supported depth format
        [[nodiscard]] vk::Format getSupportedDepthFormat() const;

        /// @brief Get the sync objects for a given frame
        [[nodiscard]] FrameSyncObjects& getSyncObjects(const uint32_t frameIndex);

        [[nodiscard]] const FrameSyncObjects& getSyncObjects(const uint32_t frameIndex) const;

        /// @brief Acquire next image for rendering
        vk::Result acquireNextImage(uint32_t frameIndex, uint32_t *imageIndex);

        /// @brief Present the rendered image
        vk::Result present(uint32_t frameIndex, uint32_t imageIndex);

        /// @brief Pipeline barrier helper.
        /// This is used to perform layout transition on the swapchain images
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

        [[nodiscard]] bool compareSwapFormats(const SwapChain &swapChain) const;


        static void forEachFrameInFlight(const std::function<void(int frame)> &cb);

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
        [[nodiscard]] vk::Extent2D chooseSwapExtent(const vk::SurfaceCapabilitiesKHR &capabilities) const;

        // SwapChain resources
        vk::Format swapChainImageFormat_{};
        vk::Extent2D swapChainExtent_;
        std::vector<vk::Image> swapChainImages_;
        std::vector<vk::ImageView> swapChainImageViews_;

        Device &device_;
        vk::SurfaceKHR surface_;
        vk::Extent2D windowExtent_;
        vk::SwapchainKHR swapChain_;
        std::shared_ptr<SwapChain> oldSwapChain_;

        // Per-frame synchronization objects (indexed by frame, not by SwapChain)
        std::array<FrameSyncObjects, MAX_FRAMES_IN_FLIGHT> frameSyncObjects_;
    };
} // namespace hammock::core
