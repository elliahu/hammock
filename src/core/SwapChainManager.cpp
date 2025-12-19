module;
#include <vulkan/vulkan.h>
#include <iostream>
#include <string>
#include <cassert>

module hammock.core.swapchain_manager;

import hammock.core.resource_manager;
import hammock.core.base_surface_provider;
import hammock.core.base_resource;
import hammock.core.image;

hammock::core::SwapChainManager::SwapChainManager(
    BaseSurfaceProvider& i_surfaceProvider,
    Device &device)
    : surfaceProvider{i_surfaceProvider}, device{device} {
    recreateSwapChain();
}

void hammock::core::SwapChainManager::recreateSwapChain() {
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

    // Trigger callbacks
    for (auto &callback : onSwapChainRecreated) {
        if (callback) {
            callback(extent.width, extent.height);
        }
    }
}

bool hammock::core::SwapChainManager::beginFrame() {
    assert(!isFrameStarted && "Cannot call beginFrame while already in progress");

    auto result = swapChain->acquireNextImage(currentFrameIndex, &currentImageIndex);

    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        recreateSwapChain();
        return false;
    }

    if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        throw std::runtime_error("failed to acquire swap chain image!");
    }

    // Reset fence after successful acquire
    auto& syncObjects = swapChain->getSyncObjects(currentFrameIndex);
    vkResetFences(device.device(), 1, &syncObjects.inFlightFence);

    isFrameStarted = true;
    return true;
}

void hammock::core::SwapChainManager::present() {
    assert(isFrameStarted && "Cannot call present while frame is not in progress");

    auto result = swapChain->present(currentFrameIndex, currentImageIndex);

    if (result == VK_ERROR_OUT_OF_DATE_KHR ||
        result == VK_SUBOPTIMAL_KHR ||
        surfaceProvider.wasResized()) {
        surfaceProvider.resetResized();
        recreateSwapChain();
    } else if (result != VK_SUCCESS) {
        throw std::runtime_error("failed to present swap chain image");
    }
}

void hammock::core::SwapChainManager::endFrame() {
    assert(isFrameStarted && "Cannot call endFrame while frame is not in progress");
    isFrameStarted = false;
    currentFrameIndex = (currentFrameIndex + 1) % SwapChain::MAX_FRAMES_IN_FLIGHT;
}