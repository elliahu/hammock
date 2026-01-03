module;
#include <iostream>
#include <string>
#include <cassert>
#include <vulkan/vulkan.hpp>

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

    device.waitIdle();

    if (swapChain == nullptr) {
        swapChain = std::make_unique<SwapChain>(device, surfaceProvider.getSurface(), extent);
    } else {
        std::shared_ptr<SwapChain> oldSwapChain = std::move(swapChain);
        swapChain = std::make_unique<SwapChain>(device, surfaceProvider.getSurface(), extent, oldSwapChain);

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

    if (result == vk::Result::eErrorOutOfDateKHR) {
        recreateSwapChain();
        return false;
    }

    if (result != vk::Result::eSuccess && result != vk::Result::eSuboptimalKHR) {
        throw std::runtime_error("failed to acquire swap chain image!");
    }

    // Reset fence after successful acquire
    auto& syncObjects = swapChain->getSyncObjects(currentFrameIndex);
    if (device.device().resetFences(1, &syncObjects.inFlightFence) != vk::Result::eSuccess) {
        throw std::runtime_error("failed to reset in flight fences");
    }

    isFrameStarted = true;
    return true;
}

void hammock::core::SwapChainManager::present() {
    assert(isFrameStarted && "Cannot call present while frame is not in progress");

    auto result = swapChain->present(currentFrameIndex, currentImageIndex);

    if (result == vk::Result::eErrorOutOfDateKHR ||
        result == vk::Result::eSuboptimalKHR ||
        surfaceProvider.wasResized()) {
        surfaceProvider.resetResized();
        recreateSwapChain();
    } else if (result != vk::Result::eSuccess) {
        throw std::runtime_error("failed to present swap chain image");
    }
}

void hammock::core::SwapChainManager::endFrame() {
    assert(isFrameStarted && "Cannot call endFrame while frame is not in progress");
    isFrameStarted = false;
    currentFrameIndex = (currentFrameIndex + 1) % SwapChain::MAX_FRAMES_IN_FLIGHT;
}