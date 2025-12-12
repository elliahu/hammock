#include "FrameManager.h"
#include <iostream>
#include "ResourceManager.h"

hammock::core::FrameManager::FrameManager(SurfaceProvider& i_surfaceProvider, Device &device) : surfaceProvider{i_surfaceProvider}, device{device}{
    recreateSwapChain();
}

hammock::core::FrameManager::~FrameManager() {
}



void hammock::core::FrameManager::recreateSwapChain() {
    auto extent = surfaceProvider.getExtent();

    while (extent.width == 0 || extent.height == 0) {
        extent = surfaceProvider.getExtent();
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


void hammock::core::FrameManager::submitPresentCommandBuffer(VkCommandBuffer commandBuffer,  const std::vector<VkSemaphore>& wait, const std::vector<VkPipelineStageFlags>& waitStages) {
    if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
        throw std::runtime_error("failed to record command buffer");
    }

    auto result = swapChain->submitCommandBuffers(&commandBuffer, &currentImageIndex, wait, waitStages);
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || surfaceProvider.wasResized()) {
        // Window was resized (resolution was changed)
        surfaceProvider.resetResized();
        recreateSwapChain();
    } else if (result != VK_SUCCESS) {
        throw std::runtime_error("failed to present swap chain image");
    }
}

bool hammock::core::FrameManager::beginFrame() {
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

void hammock::core::FrameManager::endFrame() {
    assert(isFrameStarted && "Cannot call endFrame while frame is not in progress");
    isFrameStarted = false;
    currentFrameIndex = (currentFrameIndex + 1) % SwapChain::MAX_FRAMES_IN_FLIGHT;
}
