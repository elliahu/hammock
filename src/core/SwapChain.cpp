module;

#include <vulkan/vulkan.h>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <memory>
#include <vector>

module hammock.core.swapchain;

import hammock.core.utilities;
import hammock.core.device;

namespace hammock::core {
    SwapChain::SwapChain(Device &deviceRef, const VkExtent2D extent)
        : device{deviceRef}, windowExtent{extent} {
        init();
    }

    SwapChain::SwapChain(Device &deviceRef, const VkExtent2D extent, const std::shared_ptr<SwapChain> &previous)
        : device{deviceRef}, windowExtent{extent}, oldSwapChain{previous} {
        init();
        oldSwapChain = nullptr;
    }

    VkResult SwapChain::acquireNextImage(uint32_t frameIndex, uint32_t *imageIndex) {
        auto& syncObjects = frameSyncObjects[frameIndex];

        // Wait for the fence from the previous frame
        vkWaitForFences(
            device.device(),
            1,
            &syncObjects.inFlightFence,
            VK_TRUE,
            UINT64_MAX
        );

        // Don't reset fence here - let the manager do it after successful acquire

        return vkAcquireNextImageKHR(
            device.device(),
            swapChain,
            UINT64_MAX,
            syncObjects.imageAvailable->getVulkanSemaphore(),
            VK_NULL_HANDLE,
            imageIndex
        );
    }

    VkResult SwapChain::present(uint32_t frameIndex, uint32_t imageIndex) {
        auto& syncObjects = frameSyncObjects[frameIndex];

        VkSemaphore signalSemaphores[] = {
            syncObjects.renderFinished->getVulkanSemaphore()
        };

        VkPresentInfoKHR presentInfo{};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = signalSemaphores;

        VkSwapchainKHR swapChains[] = {swapChain};
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = swapChains;
        presentInfo.pImageIndices = &imageIndex;

        return vkQueuePresentKHR(device.presentQueue(), &presentInfo);
    }

    void SwapChain::recordPipelineBarrier(
        std::uint32_t imageIndex,
        VkCommandBuffer cmd,
        VkPipelineStageFlags2 srcStageMask,
        VkAccessFlags2 srcAccessMask,
        VkPipelineStageFlags2 dstStageMask,
        VkAccessFlags2 dstAccessMask,
        VkImageLayout oldLayout,
        VkImageLayout newLayout,
        uint32_t srcQueueFamilyIndex,
        uint32_t dstQueueFamilyIndex) const {

        VkImageSubresourceRange subresourceRange{
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1
        };

        VkImageMemoryBarrier2 imageBarrier = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
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

        VkDependencyInfo depInfo = {
            .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
            .imageMemoryBarrierCount = 1,
            .pImageMemoryBarriers = &imageBarrier,
        };

        vkCmdPipelineBarrier2(cmd, &depInfo);
    }

    void SwapChain::init() {
        createSwapChain();
        createImageViews();
        createSyncObjects();
    }

    SwapChain::~SwapChain() {
        for (const auto imageView: swapChainImageViews) {
            vkDestroyImageView(device.device(), imageView, nullptr);
        }
        swapChainImageViews.clear();

        if (swapChain != VK_NULL_HANDLE) {
            vkDestroySwapchainKHR(device.device(), swapChain, nullptr);
            swapChain = VK_NULL_HANDLE;
        }

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            vkDestroyFence(device.device(), frameSyncObjects[i].inFlightFence, nullptr);
        }
    }

    void SwapChain::createSwapChain() {
        const auto [capabilities, formats, presentModes] = device.getSwapChainSupport();

        auto [format, colorSpace] = chooseSwapSurfaceFormat(formats);
        const VkPresentModeKHR presentMode = chooseSwapPresentMode(presentModes);
        const VkExtent2D extent = chooseSwapExtent(capabilities);

        uint32_t imageCount = capabilities.minImageCount + 1;
        if (capabilities.maxImageCount > 0 && imageCount > capabilities.maxImageCount) {
            imageCount = capabilities.maxImageCount;
        }

        VkSwapchainCreateInfoKHR createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        createInfo.surface = device.surface();

        createInfo.minImageCount = imageCount;
        createInfo.imageFormat = format;
        createInfo.imageColorSpace = colorSpace;
        createInfo.imageExtent = extent;
        createInfo.imageArrayLayers = 1;
        createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;

        QueueFamilyIndices indices = device.findPhysicalQueueFamilies();
        uint32_t queueFamilyIndices[] = {indices.graphicsFamily, indices.presentFamily};

        if (indices.graphicsFamily != indices.presentFamily) {
            createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
            createInfo.queueFamilyIndexCount = 2;
            createInfo.pQueueFamilyIndices = queueFamilyIndices;
        } else {
            createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
            createInfo.queueFamilyIndexCount = 0;
            createInfo.pQueueFamilyIndices = nullptr;
        }

        createInfo.preTransform = capabilities.currentTransform;
        createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        createInfo.presentMode = presentMode;
        createInfo.clipped = VK_TRUE;
        createInfo.oldSwapchain = oldSwapChain == nullptr ? VK_NULL_HANDLE : oldSwapChain->swapChain;

        if (vkCreateSwapchainKHR(device.device(), &createInfo, nullptr, &swapChain) != VK_SUCCESS) {
            throw std::runtime_error("failed to create swap chain!");
        }

        vkGetSwapchainImagesKHR(device.device(), swapChain, &imageCount, nullptr);
        swapChainImages.resize(imageCount);
        vkGetSwapchainImagesKHR(device.device(), swapChain, &imageCount, swapChainImages.data());

        swapChainImageFormat = format;
        swapChainExtent = extent;
    }

    void SwapChain::createImageViews() {
        swapChainImageViews.resize(swapChainImages.size());
        for (size_t i = 0; i < swapChainImages.size(); i++) {
            VkImageViewCreateInfo viewInfo{};
            viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            viewInfo.image = swapChainImages[i];
            viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            viewInfo.format = swapChainImageFormat;
            viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            viewInfo.subresourceRange.baseMipLevel = 0;
            viewInfo.subresourceRange.levelCount = 1;
            viewInfo.subresourceRange.baseArrayLayer = 0;
            viewInfo.subresourceRange.layerCount = 1;

            if (vkCreateImageView(device.device(), &viewInfo, nullptr, &swapChainImageViews[i]) != VK_SUCCESS) {
                throw std::runtime_error("failed to create texture image view!");
            }
        }
    }

    void SwapChain::createSyncObjects() {
        VkFenceCreateInfo fenceInfo = {};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            frameSyncObjects[i].imageAvailable = std::make_unique<Semaphore>(device);
            frameSyncObjects[i].renderFinished = std::make_unique<Semaphore>(device);

            if (vkCreateFence(device.device(), &fenceInfo, nullptr, &frameSyncObjects[i].inFlightFence) != VK_SUCCESS) {
                throw std::runtime_error("failed to create synchronization objects for a frame!");
            }
        }
    }

    VkSurfaceFormatKHR SwapChain::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &availableFormats) {
        for (const auto &availableFormat: availableFormats) {
            if (availableFormat.format == VK_FORMAT_B8G8R8A8_UNORM &&
                availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
                return availableFormat;
            }
        }
        return availableFormats[0];
    }

    VkPresentModeKHR SwapChain::chooseSwapPresentMode(const std::vector<VkPresentModeKHR> &availablePresentModes) {
        for (const auto &availablePresentMode: availablePresentModes) {
            if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
                Logger::info("Present mode: Mailbox");
                return availablePresentMode;
            }
            if (availablePresentMode == VK_PRESENT_MODE_IMMEDIATE_KHR) {
                Logger::info("Present mode: Immediate");
                return availablePresentMode;
            }
            if (availablePresentMode == VK_PRESENT_MODE_FIFO_RELAXED_KHR) {
                Logger::info("Present mode: V-Sync Relaxed");
                return availablePresentMode;
            }
        }
        Logger::info("Present mode: V-Sync");
        return VK_PRESENT_MODE_FIFO_KHR;
    }

    VkExtent2D SwapChain::chooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities) const {
        if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
            return capabilities.currentExtent;
        } else {
            VkExtent2D actualExtent = windowExtent;
            actualExtent.width = std::max(
                capabilities.minImageExtent.width,
                std::min(capabilities.maxImageExtent.width, actualExtent.width));
            actualExtent.height = std::max(
                capabilities.minImageExtent.height,
                std::min(capabilities.maxImageExtent.height, actualExtent.height));
            return actualExtent;
        }
    }

    VkFormat SwapChain::getSupportedDepthFormat() const {
        return device.findSupportedFormat(
            {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
            VK_IMAGE_TILING_OPTIMAL,
            VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
    }
} // namespace hammock::core