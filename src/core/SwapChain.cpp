module;

#include <cstdint>
#include <limits>
#include <stdexcept>
#include <memory>
#include <vector>
#include <vulkan/vulkan.hpp>

module hammock.core.swapchain;

import hammock.core.utilities;
import hammock.core.device;

namespace hammock::core {
    SwapChain::SwapChain(Device &deviceRef, const vk::Extent2D extent)
        : device{deviceRef}, windowExtent{extent} {
        init();
    }

    SwapChain::SwapChain(Device &deviceRef, const vk::Extent2D extent, const std::shared_ptr<SwapChain> &previous)
        : device{deviceRef}, windowExtent{extent}, oldSwapChain{previous} {
        init();
        oldSwapChain = nullptr;
    }

    vk::Result SwapChain::acquireNextImage(uint32_t frameIndex, uint32_t *imageIndex) {
        auto& syncObjects = frameSyncObjects[frameIndex];

        // Wait for the fence from the previous frame
        if (device.device().waitForFences(
            1,
            &syncObjects.inFlightFence,
            true,
            UINT64_MAX
        ) != vk::Result::eSuccess) {
            throw std::runtime_error("failed to wait for in flight fence");
        }

        // Don't reset fence here - let the manager do it after successful acquire

        return device.device().acquireNextImageKHR(
            swapChain,
            UINT64_MAX,
            syncObjects.imageAvailable->getVulkanSemaphore(),
            nullptr,
            imageIndex
        );
    }

    vk::Result SwapChain::present(uint32_t frameIndex, uint32_t imageIndex) {
        auto& syncObjects = frameSyncObjects[frameIndex];

        vk::Semaphore signalSemaphores[] = {
            syncObjects.renderFinished->getVulkanSemaphore()
        };

        vk::PresentInfoKHR presentInfo{};
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = signalSemaphores;

        vk::SwapchainKHR swapChains[] = {swapChain};
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = swapChains;
        presentInfo.pImageIndices = &imageIndex;

        return device.presentQueue().presentKHR(&presentInfo);
    }

    void SwapChain::recordPipelineBarrier(
        std::uint32_t imageIndex,
        vk::CommandBuffer cmd,
        vk::PipelineStageFlags2 srcStageMask,
        vk::AccessFlags2 srcAccessMask,
        vk::PipelineStageFlags2 dstStageMask,
        vk::AccessFlags2 dstAccessMask,
        vk::ImageLayout oldLayout,
        vk::ImageLayout newLayout,
        uint32_t srcQueueFamilyIndex,
        uint32_t dstQueueFamilyIndex) const {

        vk::ImageSubresourceRange subresourceRange{
            .aspectMask = vk::ImageAspectFlagBits::eColor,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1
        };

        vk::ImageMemoryBarrier2 imageBarrier = {
            .srcStageMask = srcStageMask,
            .srcAccessMask = srcAccessMask,
            .dstStageMask = dstStageMask,
            .dstAccessMask = dstAccessMask,
            .oldLayout = oldLayout,
            .newLayout = newLayout,
            .srcQueueFamilyIndex = srcQueueFamilyIndex,
            .dstQueueFamilyIndex = dstQueueFamilyIndex,
            .image = swapChainImages[imageIndex],
            .subresourceRange = subresourceRange,
        };

        vk::DependencyInfo depInfo = {
            .imageMemoryBarrierCount = 1,
            .pImageMemoryBarriers = &imageBarrier,
        };

        cmd.pipelineBarrier2(&depInfo);
    }

    void SwapChain::init() {
        createSwapChain();
        createImageViews();
        createSyncObjects();
    }

    SwapChain::~SwapChain() {
        for (const auto imageView: swapChainImageViews) {
            device.device().destroyImageView(imageView, nullptr);
        }
        swapChainImageViews.clear();

        if (swapChain != nullptr) {
            device.device().destroySwapchainKHR(swapChain, nullptr);
            swapChain = nullptr;
        }

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            device.device().destroyFence(frameSyncObjects[i].inFlightFence, nullptr);
        }
    }

    void SwapChain::createSwapChain() {
        const auto [capabilities, formats, presentModes] = device.getSwapChainSupport();

        auto [format, colorSpace] = chooseSwapSurfaceFormat(formats);
        const vk::PresentModeKHR presentMode = chooseSwapPresentMode(presentModes);
        const vk::Extent2D extent = chooseSwapExtent(capabilities);

        uint32_t imageCount = capabilities.minImageCount + 1;
        if (capabilities.maxImageCount > 0 && imageCount > capabilities.maxImageCount) {
            imageCount = capabilities.maxImageCount;
        }

        vk::SwapchainCreateInfoKHR createInfo = {};
        createInfo.surface = device.surface();

        createInfo.minImageCount = imageCount;
        createInfo.imageFormat = format;
        createInfo.imageColorSpace = colorSpace;
        createInfo.imageExtent = extent;
        createInfo.imageArrayLayers = 1;
        createInfo.imageUsage = vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferDst;

        QueueFamilyIndices indices = device.findPhysicalQueueFamilies();
        uint32_t queueFamilyIndices[] = {indices.graphicsFamily, indices.presentFamily};

        if (indices.graphicsFamily != indices.presentFamily) {
            createInfo.imageSharingMode = vk::SharingMode::eConcurrent;
            createInfo.queueFamilyIndexCount = 2;
            createInfo.pQueueFamilyIndices = queueFamilyIndices;
        } else {
            createInfo.imageSharingMode = vk::SharingMode::eExclusive;
            createInfo.queueFamilyIndexCount = 0;
            createInfo.pQueueFamilyIndices = nullptr;
        }

        createInfo.preTransform = capabilities.currentTransform;
        createInfo.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
        createInfo.presentMode = presentMode;
        createInfo.clipped = true;
        createInfo.oldSwapchain = oldSwapChain == nullptr ? nullptr : oldSwapChain->swapChain;

        if (device.device().createSwapchainKHR(&createInfo, nullptr, &swapChain) != vk::Result::eSuccess) {
            throw std::runtime_error("failed to create swap chain!");
        }

        swapChainImages = device.device().getSwapchainImagesKHR(swapChain);

        swapChainImageFormat = format;
        swapChainExtent = extent;
    }

    void SwapChain::createImageViews() {
        swapChainImageViews.resize(swapChainImages.size());
        for (size_t i = 0; i < swapChainImages.size(); i++) {
            vk::ImageViewCreateInfo viewInfo{};
            viewInfo.image = swapChainImages[i];
            viewInfo.viewType = vk::ImageViewType::e2D;
            viewInfo.format = swapChainImageFormat;
            viewInfo.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
            viewInfo.subresourceRange.baseMipLevel = 0;
            viewInfo.subresourceRange.levelCount = 1;
            viewInfo.subresourceRange.baseArrayLayer = 0;
            viewInfo.subresourceRange.layerCount = 1;

            if (device.device().createImageView(&viewInfo, nullptr, &swapChainImageViews[i]) != vk::Result::eSuccess) {
                throw std::runtime_error("failed to create texture image view!");
            }
        }
    }

    void SwapChain::createSyncObjects() {
        vk::FenceCreateInfo fenceInfo = {};
        fenceInfo.flags = vk::FenceCreateFlagBits::eSignaled;

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            frameSyncObjects[i].imageAvailable = std::make_unique<Semaphore>(device);
            frameSyncObjects[i].renderFinished = std::make_unique<Semaphore>(device);

            if (device.device().createFence(&fenceInfo, nullptr, &frameSyncObjects[i].inFlightFence) != vk::Result::eSuccess) {
                throw std::runtime_error("failed to create synchronization objects for a frame!");
            }
        }
    }

     vk::SurfaceFormatKHR SwapChain::chooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR> &availableFormats) {
        for (const auto &availableFormat: availableFormats) {
            if (availableFormat.format == vk::Format::eB8G8R8A8Unorm &&
                availableFormat.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear) {
                return availableFormat;
            }
        }
        return availableFormats[0];
    }

    vk::PresentModeKHR SwapChain::chooseSwapPresentMode(const std::vector<vk::PresentModeKHR> &availablePresentModes) {
        for (const auto &availablePresentMode: availablePresentModes) {
            if (availablePresentMode == vk::PresentModeKHR::eMailbox) {
                Logger::info("Present mode: Mailbox");
                return availablePresentMode;
            }
            if (availablePresentMode == vk::PresentModeKHR::eImmediate) {
                Logger::info("Present mode: Immediate");
                return availablePresentMode;
            }
            if (availablePresentMode == vk::PresentModeKHR::eFifoRelaxed) {
                Logger::info("Present mode: V-Sync Relaxed");
                return availablePresentMode;
            }
        }
        Logger::info("Present mode: V-Sync");
        return vk::PresentModeKHR::eFifo;
    }

    vk::Extent2D SwapChain::chooseSwapExtent(const vk::SurfaceCapabilitiesKHR &capabilities) const {
        if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
            return capabilities.currentExtent;
        } else {
            vk::Extent2D actualExtent = windowExtent;
            actualExtent.width = std::max(
                capabilities.minImageExtent.width,
                std::min(capabilities.maxImageExtent.width, actualExtent.width));
            actualExtent.height = std::max(
                capabilities.minImageExtent.height,
                std::min(capabilities.maxImageExtent.height, actualExtent.height));
            return actualExtent;
        }
    }

    vk::Format SwapChain::getSupportedDepthFormat() const {
        return device.findSupportedFormat(
            {vk::Format::eD32Sfloat, vk::Format::eD32SfloatS8Uint, vk::Format::eD24UnormS8Uint},
            vk::ImageTiling::eOptimal,
            vk::FormatFeatureFlagBits::eDepthStencilAttachment);
    }
} // namespace hammock::core