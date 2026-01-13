module;
#include <memory>
#include <functional>
#include <compare>
#include <vulkan/vulkan.hpp>

module hammock_engine;

import hammock_core;
import hammock_renderer;

// ************ Base presentation strategy *************

std::optional<hammock::engine::FrameContext> hammock::engine::BasePresentationStrategy::beginFrame() {
    if (frameInProgress_) {
        throw std::runtime_error("Frame already in progress");
    }

    // Strategy-specific pre-frame logic
    if (!onBeginFrame()) {
        // Strategy says skip this frame (e.g., swapchain out of date)
        return std::nullopt;
    }

    // Per frame resources
    auto &frameRes = perFrameResources_[currentFrameIndex_];

    // Swap buffers
    framebuffer_->swapImages();

    frameInProgress_ = true;
    return FrameContext{
        .renderTarget = framebuffer_->getFrontbufferImage(),
        .renderFinished = *frameRes.renderingFinished,
        .frameIndex = currentFrameIndex_
    };
}

void hammock::engine::BasePresentationStrategy::endFrame(const FrameContext &ctx) {
    if (!frameInProgress_) {
        throw std::runtime_error("No frame in progress");
    }

    // Strategy-specific presentation logic
    onPresent(ctx);

    // Advance frame index
    currentFrameIndex_ = (currentFrameIndex_ + 1) % framesInFlight_;
    frameInProgress_ = false;
}

vk::Extent2D hammock::engine::BasePresentationStrategy::getResolution() const {
    return framebuffer_->getExtent();
}

hammock::core::ImageFormat hammock::engine::BasePresentationStrategy::getFormat() const {
    return core::ImageFormat::R8G8B8A8Uint; // TODO: get from framebuffer
}


hammock::engine::BasePresentationStrategy::BasePresentationStrategy(core::Device &device, vk::Extent2D resolution,
                                                                    core::ImageFormat format,
                                                                    uint32_t framesInFlight) : device_(device),
    framesInFlight_(framesInFlight) {
    // Create framebuffer (shared by all strategies)
    framebuffer_ = std::make_unique<Framebuffer>(
        device_,
        framesInFlight_,
        math::Vec2{
            static_cast<float>(resolution.width),
            static_cast<float>(resolution.height)
        },
        format
    );

    // Create per-frame synchronization (shared by all strategies)
    perFrameResources_.resize(framesInFlight_);
    for (auto &frameRes: perFrameResources_) {
        frameRes.renderingFinished = std::make_unique<core::Semaphore>(device_);
    }
}

void hammock::engine::BasePresentationStrategy::recreateFramebuffer(vk::Extent2D newResolution) {
    // Wait for GPU to finish
    device_.waitIdle();

    // Destroy old framebuffer
    framebuffer_.reset();

    // Create new framebuffer
    framebuffer_ = std::make_unique<Framebuffer>(
        device_,
        framesInFlight_,
        math::Vec2{
            static_cast<float>(newResolution.width),
            static_cast<float>(newResolution.height)
        },
        getFormat()
    );

    // Notify subclass
    onFramebufferRecreated();

    // Notify listeners
    notifyResolutionChanged(newResolution.width, newResolution.height);
}

void hammock::engine::BasePresentationStrategy::notifyResolutionChanged(uint32_t width, uint32_t height) {
    for (auto &callback: resolutionCallbacks_) {
        callback(width, height);
    }
}

// **************** Headless presentation strategy ************

hammock::engine::HeadlessPresentationStrategy::HeadlessPresentationStrategy(core::Device &device,
                                                                            vk::Extent2D resolution, core::ImageFormat format,
                                                                            uint32_t
                                                                            framesInFlight) : BasePresentationStrategy(
    device, resolution, format, framesInFlight) {
    // Base class handles everything
}

hammock::core::ResourceHandle hammock::engine::HeadlessPresentationStrategy::getRenderedImage() const {
    // Return the current front buffer for readback
    return framebuffer_->getFrontbufferImage();
}

void hammock::engine::HeadlessPresentationStrategy::setResolution(vk::Extent2D newResolution) {
    recreateFramebuffer(newResolution);
}

void hammock::engine::HeadlessPresentationStrategy::onPresent(const FrameContext &ctx) {
    // No presentation needed!
    // Framebuffer is already the final output
    // Just need to ensure GPU work is submitted with fence

    auto &frameRes = perFrameResources_[ctx.frameIndex];

    // Create a dummy command buffer just to submit the fence
    // (in reality, the renderer's command buffer should have already
    // submitted with the fence, so this might not be needed)

    // Alternative: The renderer itself submits with the fence
    // In that case, this method could be empty!
}

// ***************** Surface presentation strategy *************

hammock::engine::SurfacePresentationStrategy::SurfacePresentationStrategy(core::Device &device,
                                                                          core::BaseSurfaceProvider &surfaceProvider,
                                                                          Mode mode)
    : BasePresentationStrategy(
          device,
          vk::Extent2D{1920, 1080}, // Initial size, will be updated
          core::ImageFormat::R8G8B8A8Uint,
          core::SwapChain::MAX_FRAMES_IN_FLIGHT // Surface rendering typically uses 2 frames in flight
      ),
      mode_(mode) {
    // Initialize swapchain manager
    swapchainManager_ = std::make_unique<core::SwapChainManager>(surfaceProvider, device_);

    // Register swapchain recreation callback
    swapchainManager_->registerOnSwapChainRecreatedCallback(
        [this](uint32_t width, uint32_t height) {
            recreateFramebuffer(vk::Extent2D{width, height});
        }
    );

    // Create surface-specific resources

    surfacePerFrameResources_.resize(framesInFlight_);
    for (auto &surfaceRes: surfacePerFrameResources_) {
        surfaceRes.presentCommandBuffer = std::make_unique<core::CommandBuffer>(
            device_, core::CommandQueueFamily::Graphics
        );

        if (mode_ == Mode::Editor) {
            surfaceRes.uiFinished = std::make_unique<core::Semaphore>(device_);
        }
    }

    // Sync framebuffer size with swapchain

    auto extent = swapchainManager_->getSwapChain().getExtent();
    if (framebuffer_->getExtent() != extent) {
        recreateFramebuffer(extent);
    }
}

void hammock::engine::SurfacePresentationStrategy::submitUI(
    const FrameContext &ctx,
    std::function<void(core::ResourceHandle, std::uint32_t, core::Semaphore &, core::Semaphore &)> uiRenderFunc
) {
    if (mode_ != Mode::Editor) {
        return;
    }
    auto frameIndex = swapchainManager_->getFrameIndex();

    uiRenderFunc(
        framebuffer_->getFrontbufferImage(),
        frameIndex,
        *perFrameResources_[frameIndex].renderingFinished,
        *surfacePerFrameResources_[frameIndex].uiFinished);
}

vk::ImageView hammock::engine::SurfacePresentationStrategy::getSwapchainImageView(uint32_t index) const {
    return swapchainManager_->getSwapChain().getImageView(index);
}

bool hammock::engine::SurfacePresentationStrategy::onBeginFrame() {
    // Acquire swapchain image
    if (!swapchainManager_->beginFrame()) {
        // Swapchain out of date, skip this frame
        return false;
    }

    currentSwapchainImageIndex_ = swapchainManager_->getSwapChainImageIndex();
    return true;
}

void hammock::engine::SurfacePresentationStrategy::onPresent(const FrameContext &ctx) {
    auto &swapChain = swapchainManager_->getSwapChain();
    auto &syncObjects = swapChain.getSyncObjects(ctx.frameIndex);
    auto &frameRes = perFrameResources_[ctx.frameIndex];
    auto &surfaceRes = surfacePerFrameResources_[ctx.frameIndex];
    auto &presentCmd = *surfaceRes.presentCommandBuffer;

    // Determine which semaphore to wait on

    core::Semaphore *waitSemaphore = nullptr;

    if (mode_ == Mode::Editor) {
        // Wait for UI to finish
        waitSemaphore = surfaceRes.uiFinished.get();
    } else {
        // Wait for scene rendering to finish
        waitSemaphore = frameRes.renderingFinished.get();
    }

    // Set up presentation command buffer
    // Wai wait for two semaphores here
    presentCmd.addWaitSemaphore(*waitSemaphore,
                                vk::PipelineStageFlagBits2::eTopOfPipe);
    presentCmd.addWaitSemaphore(*syncObjects.imageAvailable,
                                vk::PipelineStageFlagBits2::eColorAttachmentOutput);
    presentCmd.addSignalSemaphore(*syncObjects.frameFinished);

    // Record blit operation

    presentCmd.begin();
    blitFramebufferToSwapchain(ctx);
    presentCmd.submit(
        swapchainManager_->getSwapChain().getSyncObjects(swapchainManager_->getFrameIndex()).inFlightFence);

    // Present
    swapchainManager_->present();
    swapchainManager_->endFrame();
}

void hammock::engine::SurfacePresentationStrategy::onFramebufferRecreated() {
    BasePresentationStrategy::onFramebufferRecreated();
    // Surface strategy might need to do additional work here
    // (though currently nothing needed)
}

void hammock::engine::SurfacePresentationStrategy::blitFramebufferToSwapchain(const FrameContext &ctx) {
    auto &surfaceRes = surfacePerFrameResources_[ctx.frameIndex];
    auto &presentCmd = *surfaceRes.presentCommandBuffer;
    auto imageHandle = framebuffer_->getFrontbufferImage();
    auto image = core::ResourceManager::getInstance().getResource<core::Image>(imageHandle);

    // Transition the target image to transfer scr optimal
    image->recordPipelineBarrier(
        presentCmd.getCommandBuffer(),
        vk::PipelineStageFlagBits2::eColorAttachmentOutput,
        vk::AccessFlagBits2::eColorAttachmentWrite,
        vk::PipelineStageFlagBits2::eTransfer,
        vk::AccessFlagBits2::eTransferRead,
        vk::ImageLayout::eColorAttachmentOptimal,
        vk::ImageLayout::eTransferSrcOptimal,
        vk::QueueFamilyIgnored,
        vk::QueueFamilyIgnored
    );

    swapchainManager_->blitToSwapChainImage(presentCmd, imageHandle);
}
