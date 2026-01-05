module;

#include <cstdint>
#include <limits>
#include <stdexcept>
#include <memory>
#include <vector>
#include <functional>
#include <compare>
#include <vulkan/vulkan.hpp>

module hammock.core.swapchain;

import hammock.core.utilities;
import hammock.core.device;

namespace hammock::core {
    SwapChain::SwapChain(Device &deviceRef, vk::SurfaceKHR surface, const vk::Extent2D extent)
        : device_{deviceRef}, surface_{surface}, windowExtent_{extent} {
        init();
    }

    SwapChain::SwapChain(Device &deviceRef, vk::SurfaceKHR surface, const vk::Extent2D extent, const std::shared_ptr<SwapChain> &previous)
        : device_{deviceRef}, surface_{surface}, windowExtent_{extent}, oldSwapChain_{previous} {
        init();
        oldSwapChain_ = nullptr;
    }

    vk::Result SwapChain::acquireNextImage(uint32_t frameIndex, uint32_t *imageIndex) {
        auto &syncObjects = frameSyncObjects_[frameIndex];

        // Wait for both: GPU execution AND Presentation release
        std::vector<vk::Fence> fences = {
            syncObjects.inFlightFence
        };

        if (isSwapChainMaintenance1FeatureSupported()) {
            fences.push_back(syncObjects.releaseFence);
        }

        if (device_.device().waitForFences(
                fences.size(),
                fences.data(),
                true, // Wait for ALL
                UINT64_MAX
            ) != vk::Result::eSuccess) {
            throw std::runtime_error("failed to wait for fences");
        }

        // Resetting here is fine
        if (device_.device().resetFences(1, &syncObjects.inFlightFence) != vk::Result::eSuccess) {
            throw std::runtime_error("failed to reset in flight fence");
        }

        return device_.device().acquireNextImageKHR(
            swapChain_,
            UINT64_MAX,
            syncObjects.imageAvailable->getVulkanSemaphore(),
            nullptr,
            imageIndex
        );
    }

    vk::Result SwapChain::present(uint32_t frameIndex, uint32_t imageIndex) {
        auto &syncObjects = frameSyncObjects_[frameIndex];

        // Reset the release fence before using it in present
        if (isSwapChainMaintenance1FeatureSupported()) {
            if (device_.device().resetFences(1, &syncObjects.releaseFence) != vk::Result::eSuccess) {
                throw std::runtime_error("failed to reset release fence");
            }
        }

        vk::Semaphore signalSemaphores[] = {
            syncObjects.renderFinished->getVulkanSemaphore()
        };

        // Attach the fence to the presentation via pNext
        vk::PresentInfoKHR presentInfo{};
        if (isSwapChainMaintenance1FeatureSupported()) {
            vk::SwapchainPresentFenceInfoEXT presentFenceInfo{};
            presentFenceInfo.swapchainCount = 1;
            presentFenceInfo.pFences = &syncObjects.releaseFence;
            presentInfo.pNext = &presentFenceInfo; // The modern way
        } else {
            presentInfo.pNext = nullptr;
        }

        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = signalSemaphores;

        vk::SwapchainKHR swapChains[] = {swapChain_};
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = swapChains;
        presentInfo.pImageIndices = &imageIndex;

        return device_.getPresentQueue().presentKHR(&presentInfo);
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
            .image = swapChainImages_[imageIndex],
            .subresourceRange = subresourceRange,
        };

        vk::DependencyInfo depInfo = {
            .imageMemoryBarrierCount = 1,
            .pImageMemoryBarriers = &imageBarrier,
        };

        cmd.pipelineBarrier2(&depInfo);
    }

    bool SwapChain::compareSwapFormats(const SwapChain &swapChain) const {
        return swapChain.swapChainImageFormat_ == swapChainImageFormat_;
    }

    void SwapChain::forEachFrameInFlight(const std::function<void(int frame)> &cb) {
        for (int frame = 0; frame < MAX_FRAMES_IN_FLIGHT; frame++) {
            cb(frame);
        }
    }

    void SwapChain::init() {
        createSwapChain();
        createImageViews();
        createSyncObjects();
    }

    SwapChain::~SwapChain() {
        for (const auto imageView: swapChainImageViews_) {
            device_.device().destroyImageView(imageView, nullptr);
        }
        swapChainImageViews_.clear();

        if (swapChain_ != nullptr) {
            device_.device().destroySwapchainKHR(swapChain_, nullptr);
            swapChain_ = nullptr;
        }

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            device_.device().destroyFence(frameSyncObjects_[i].inFlightFence, nullptr);
            device_.device().destroyFence(frameSyncObjects_[i].releaseFence, nullptr);
        }
    }

    void SwapChain::createSwapChain() {
        const auto [capabilities, formats, presentModes] = device_.getSwapChainSupport();

        auto [format, colorSpace] = chooseSwapSurfaceFormat(formats);
        const vk::PresentModeKHR presentMode = chooseSwapPresentMode(presentModes);
        const vk::Extent2D extent = chooseSwapExtent(capabilities);

        uint32_t imageCount = capabilities.minImageCount + 1;
        if (capabilities.maxImageCount > 0 && imageCount > capabilities.maxImageCount) {
            imageCount = capabilities.maxImageCount;
        }

        vk::SwapchainCreateInfoKHR createInfo = {};
        createInfo.surface = surface_;

        createInfo.minImageCount = imageCount;
        createInfo.imageFormat = format;
        createInfo.imageColorSpace = colorSpace;
        createInfo.imageExtent = extent;
        createInfo.imageArrayLayers = 1;
        createInfo.imageUsage = vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferDst;

        QueueFamilyIndices indices = device_.getPhysicalQueueFamilies();
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
        createInfo.oldSwapchain = oldSwapChain_ == nullptr ? nullptr : oldSwapChain_->swapChain_;

        if (device_.device().createSwapchainKHR(&createInfo, nullptr, &swapChain_) != vk::Result::eSuccess) {
            throw std::runtime_error("failed to create swap chain!");
        }

        swapChainImages_ = device_.device().getSwapchainImagesKHR(swapChain_);

        swapChainImageFormat_ = format;
        swapChainExtent_ = extent;
    }

    void SwapChain::createImageViews() {
        swapChainImageViews_.resize(swapChainImages_.size());
        for (size_t i = 0; i < swapChainImages_.size(); i++) {
            vk::ImageViewCreateInfo viewInfo{};
            viewInfo.image = swapChainImages_[i];
            viewInfo.viewType = vk::ImageViewType::e2D;
            viewInfo.format = swapChainImageFormat_;
            viewInfo.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
            viewInfo.subresourceRange.baseMipLevel = 0;
            viewInfo.subresourceRange.levelCount = 1;
            viewInfo.subresourceRange.baseArrayLayer = 0;
            viewInfo.subresourceRange.layerCount = 1;

            if (device_.device().createImageView(&viewInfo, nullptr, &swapChainImageViews_[i]) != vk::Result::eSuccess) {
                throw std::runtime_error("failed to create texture image view!");
            }
        }
    }

    void SwapChain::createSyncObjects() {
        vk::FenceCreateInfo fenceInfo = {};
        fenceInfo.flags = vk::FenceCreateFlagBits::eSignaled;

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            frameSyncObjects_[i].imageAvailable = std::make_unique<Semaphore>(device_);
            frameSyncObjects_[i].renderFinished = std::make_unique<Semaphore>(device_);

            if (device_.device().createFence(&fenceInfo, nullptr, &frameSyncObjects_[i].inFlightFence) != vk::Result::eSuccess) {
                throw std::runtime_error("failed to create synchronization objects for a frame!");
            }

            if (device_.device().createFence(&fenceInfo, nullptr, &frameSyncObjects_[i].releaseFence) != vk::Result::eSuccess) {
                throw std::runtime_error("failed to create synchronization objects for a frame!");
            }
        }
    }

    bool SwapChain::isSwapChainMaintenance1FeatureSupported() const {
        // First, check if the device actually exposes the extension
        std::vector<vk::ExtensionProperties> deviceExtensions = device_.getPhysicalDevice().enumerateDeviceExtensionProperties();
        bool hasSwapchainMaintenance1Ext = false;
        for (const auto &ext: deviceExtensions) {
            if (strcmp(ext.extensionName, vk::KHRSwapchainMaintenance1ExtensionName) == 0) {
                hasSwapchainMaintenance1Ext = true;
                break;
            }
        }

        if (!hasSwapchainMaintenance1Ext) {
            return false;
        }

        // Query the feature only if the extension exists
        vk::PhysicalDeviceSwapchainMaintenance1FeaturesEXT features{};
        vk::PhysicalDeviceFeatures2 features2{};
        features2.pNext = &features;
        device_.getPhysicalDevice().getFeatures2(&features2);

        return features.swapchainMaintenance1 == VK_TRUE;
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
            vk::Extent2D actualExtent = windowExtent_;
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
        return device_.findSupportedFormat(
            {vk::Format::eD32Sfloat, vk::Format::eD32SfloatS8Uint, vk::Format::eD24UnormS8Uint},
            vk::ImageTiling::eOptimal,
            vk::FormatFeatureFlagBits::eDepthStencilAttachment);
    }

    FrameSyncObjects &SwapChain::getSyncObjects(const uint32_t frameIndex) {
        return frameSyncObjects_[frameIndex];
    }

    const FrameSyncObjects &SwapChain::getSyncObjects(const uint32_t frameIndex) const {
        return frameSyncObjects_[frameIndex];
    }
} // namespace hammock::core
