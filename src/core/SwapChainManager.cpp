module;
#include <iostream>
#include <string>
#include <cassert>
#include <compare>
#include <vulkan/vulkan.hpp>

module hammock.core.swapchain_manager;

import hammock.core.resource_manager;
import hammock.core.base_surface_provider;
import hammock.core.base_resource;
import hammock.core.image;


hammock::core::SwapChainManager::SwapChainManager(
    BaseSurfaceProvider& i_surfaceProvider,
    Device &device)
    : surfaceProvider_{i_surfaceProvider}, device_{device} {
    recreateSwapChain();
}

void hammock::core::SwapChainManager::recreateSwapChain() {
    auto extent = surfaceProvider_.getExtent();

    while (extent.width == 0 || extent.height == 0) {
        extent = surfaceProvider_.getExtent();
        std::cout << "Window resized\n";
    }

    device_.waitIdle();

    if (swapChain_ == nullptr) {
        swapChain_ = std::make_unique<SwapChain>(device_, surfaceProvider_.getSurface(), extent);
    } else {
        std::shared_ptr<SwapChain> oldSwapChain = std::move(swapChain_);
        swapChain_ = std::make_unique<SwapChain>(device_, surfaceProvider_.getSurface(), extent, oldSwapChain);

        if (!oldSwapChain->compareSwapFormats(*swapChain_.get())) {
            throw std::runtime_error("SwapChain image format has changed");
        }
    }

    // Trigger callbacks
    for (auto &callback : onSwapChainRecreated_) {
        if (callback) {
            callback(extent.width, extent.height);
        }
    }
}

bool hammock::core::SwapChainManager::beginFrame() {
    assert(!isFrameStarted_ && "Cannot call beginFrame while already in progress");

    auto result = swapChain_->acquireNextImage(currentFrameIndex_, &currentImageIndex_);

    if (result == vk::Result::eErrorOutOfDateKHR) {
        recreateSwapChain();
        return false;
    }

    if (result != vk::Result::eSuccess && result != vk::Result::eSuboptimalKHR) {
        throw std::runtime_error("failed to acquire swap chain image!");
    }

    // Reset fence after successful acquire
    auto& syncObjects = swapChain_->getSyncObjects(currentFrameIndex_);
    if (device_.device().resetFences(1, &syncObjects.inFlightFence) != vk::Result::eSuccess) {
        throw std::runtime_error("failed to reset in flight fences");
    }

    isFrameStarted_ = true;
    return true;
}

void hammock::core::SwapChainManager::initialize(BaseSurfaceProvider &i_surfaceProvider, Device &device) {
    Singleton<SwapChainManager>::initialize(i_surfaceProvider, device);
}

int hammock::core::SwapChainManager::getFrameIndex() const {
    if (!isFrameStarted_)
        throw std::runtime_error("Cannot get frame index when frame not in progress");
    return currentFrameIndex_;
}

int hammock::core::SwapChainManager::getSwapChainImageIndex() const {
    if (!isFrameStarted_)
        throw std::runtime_error("Cannot get image index when frame not in progress");
    return currentImageIndex_;
}

void hammock::core::SwapChainManager::present() {
    assert(isFrameStarted_ && "Cannot call present while frame is not in progress");

    auto result = swapChain_->present(currentFrameIndex_, currentImageIndex_);

    if (result == vk::Result::eErrorOutOfDateKHR ||
        result == vk::Result::eSuboptimalKHR ||
        surfaceProvider_.wasResized()) {
        surfaceProvider_.resetResized();
        recreateSwapChain();
    } else if (result != vk::Result::eSuccess) {
        throw std::runtime_error("failed to present swap chain image");
    }
}

void hammock::core::SwapChainManager::endFrame() {
    assert(isFrameStarted_ && "Cannot call endFrame while frame is not in progress");
    isFrameStarted_ = false;
    currentFrameIndex_ = (currentFrameIndex_ + 1) % SwapChain::MAX_FRAMES_IN_FLIGHT;
}

void hammock::core::SwapChainManager::
registerOnSwapChainRecreatedCallback(const OnSwapChainRecreatedCallback &callback) {
    onSwapChainRecreated_.push_back(std::move(callback));
}
