#include "hammock/core/FrameManager.h"
#include <iostream>
#include "hammock/core/ResourceManager.h"

Hammock::FrameManager::FrameManager(Window &window, Device &device) : window{window}, device{device}{
    recreateSwapChain();
}

Hammock::FrameManager::~FrameManager() {
}



void Hammock::FrameManager::recreateSwapChain() {
    auto extent = window.getExtent();

    while (extent.width == 0 || extent.height == 0) {
        window.pollEvents();
        extent = window.getExtent();
        std::cout << "Window resized\n";
    }
    vkDeviceWaitIdle(device.device());

    if (swapChain == nullptr) {
        swapChain = std::make_unique<SwapChain>(device, extent);
    } else {
        std::shared_ptr<SwapChain> oldSwapChain = std::move(swapChain);
        swapChain = std::make_unique<SwapChain>(device, extent, oldSwapChain);

        if (!oldSwapChain->compareSwapFormats(*swapChain.get())) {
            throw std::runtime_error("SwapChain image format has changed");
        }
    }

    // Clear previous images
    for (auto handle: handlesOfSwapImages) {
        ResourceManager::getInstance().releaseResource(handle.getUid());
    }

    handlesOfSwapImages.clear();

    // Track new images
    for(int i = 0; i < swapChain->imageCount(); i++) {

        ResourceHandle handle = ResourceManager::getInstance().addResource<Image>("SWAPCHAIN_IMAGE_" + std::to_string(i), ImageDesc{
            .width = extent.width,
            .height = extent.height,
            .image = swapChain->getImage(i),
            .view = swapChain->getImageView(i),
            .format = swapChain->getSwapChainImageFormat(),
            .imageViewType = VK_IMAGE_VIEW_TYPE_2D,
        });

        handlesOfSwapImages.push_back(handle);
    }
}


void Hammock::FrameManager::submitPresentCommandBuffer(VkCommandBuffer commandBuffer,  const std::vector<VkSemaphore>& wait, const std::vector<VkPipelineStageFlags>& waitStages) {
    if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
        throw std::runtime_error("failed to record command buffer");
    }

    auto result = swapChain->submitCommandBuffers(&commandBuffer, &currentImageIndex, wait, waitStages);
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || window.wasWindowResized()) {
        // Window was resized (resolution was changed)
        window.resetWindowResizedFlag();
        recreateSwapChain();
    } else if (result != VK_SUCCESS) {
        throw std::runtime_error("failed to present swap chain image");
    }
}

bool Hammock::FrameManager::beginFrame() {
    assert(!isFrameStarted && "Cannot call beginFrame while already in progress");

    auto result = swapChain->acquireNextImage(&currentImageIndex);

    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        recreateSwapChain();
        return false;
    }

    if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        throw std::runtime_error("failed to aquire swap chain image!");
    }

    isFrameStarted = true;
    return true;
}

void Hammock::FrameManager::endFrame() {
    assert(isFrameStarted && "Cannot call endFrame while frame is not in progress");
    isFrameStarted = false;
    currentFrameIndex = (currentFrameIndex + 1) % SwapChain::MAX_FRAMES_IN_FLIGHT;
}
