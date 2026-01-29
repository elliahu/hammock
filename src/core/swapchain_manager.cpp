#include <iostream>
#include <string>
#include <cassert>
#include <compare>
#include <memory>
#include <vulkan/vulkan.hpp>

#include "swapchain_manager.hpp"
#include "resource_manager.hpp"
#include "image.hpp"


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

    isFrameStarted_ = true;
    return true;
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

void hammock::core::SwapChainManager::blitToSwapChainImage(CommandBuffer &commandBuffer, ResourceHandle src) {
    auto image = ResourceManager::getInstance().getResource<Image>(src);

    swapChain_->recordPipelineBarrier(
        currentImageIndex_,
        commandBuffer.getCommandBuffer(),
        vk::PipelineStageFlagBits2::eNone,
        vk::AccessFlagBits2::eNone,
        vk::PipelineStageFlagBits2::eTransfer,
        vk::AccessFlagBits2::eTransferWrite,
        vk::ImageLayout::eUndefined,
        vk::ImageLayout::eTransferDstOptimal,
        vk::QueueFamilyIgnored,
        vk::QueueFamilyIgnored
    );

    // Blit
    vk::ImageBlit blit{};
    blit.srcSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
    blit.srcSubresource.mipLevel = 0;
    blit.srcSubresource.baseArrayLayer = 0;
    blit.srcSubresource.layerCount = 1;
    blit.srcOffsets[0] = vk::Offset3D{0, 0, 0};
    blit.srcOffsets[1] = vk::Offset3D{
        static_cast<std::int32_t>(image->getExtent().width),
        static_cast<std::int32_t>(image->getExtent().height),
        1
    };

    blit.dstSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
    blit.dstSubresource.mipLevel = 0;
    blit.dstSubresource.baseArrayLayer = 0;
    blit.dstSubresource.layerCount = 1;
    blit.dstOffsets[0] = vk::Offset3D{0, 0, 0};
    blit.dstOffsets[1] = vk::Offset3D{
        static_cast<std::int32_t>(swapChain_->getExtent().width),
        static_cast<std::int32_t>(swapChain_->getExtent().height),
        1
    };

    commandBuffer.getCommandBuffer().blitImage(
        image->getImage(), vk::ImageLayout::eTransferSrcOptimal,
        swapChain_->getImage(currentImageIndex_), vk::ImageLayout::eTransferDstOptimal,
        1,
        &blit,
        vk::Filter::eLinear
    );


    swapChain_->recordPipelineBarrier(
        currentImageIndex_,
        commandBuffer.getCommandBuffer(),
        vk::PipelineStageFlagBits2::eTransfer,
        vk::AccessFlagBits2::eTransferWrite,
        vk::PipelineStageFlagBits2::eBottomOfPipe,
        vk::AccessFlagBits2::eMemoryRead,
        vk::ImageLayout::eTransferDstOptimal,
        vk::ImageLayout::ePresentSrcKHR,
        vk::QueueFamilyIgnored,
        vk::QueueFamilyIgnored
    );
}
