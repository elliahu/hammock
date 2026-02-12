#include <memory>
#include <functional>
#include <compare>
#include <vulkan/vulkan.hpp>

#include "presentation_engine.hpp"
#include "core/vulkan_context.hpp"


// ************ Base presentation strategy *************

std::optional<hammock::app::FrameContext> hammock::app::BasePresentationStrategy::beginFrame() {
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

void hammock::app::BasePresentationStrategy::endFrame(const FrameContext &ctx) {
    if (!frameInProgress_) {
        throw std::runtime_error("No frame in progress");
    }

    // Strategy-specific presentation logic
    onPresent(ctx);

    // Advance frame index
    currentFrameIndex_ = (currentFrameIndex_ + 1) % framesInFlight_;
    frameInProgress_ = false;
}

vk::Extent2D hammock::app::BasePresentationStrategy::getResolution() const {
    return framebuffer_->getExtent();
}

hammock::core::ImageFormat hammock::app::BasePresentationStrategy::getFormat() const {
    return core::ImageFormat::R8G8B8A8Uint; // TODO: get from framebuffer
}


hammock::app::BasePresentationStrategy::BasePresentationStrategy(core::VulkanContext &ctx, vk::Extent2D resolution,
                                                                    core::ImageFormat format,
                                                                    uint32_t framesInFlight) : ctx_(ctx),
    framesInFlight_(framesInFlight) {
    // Create framebuffer (shared by all strategies)
    framebuffer_ = std::make_unique<Framebuffer>(
        ctx_,
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
        frameRes.renderingFinished = std::make_unique<core::Semaphore>(*ctx_.device);
    }
}

void hammock::app::BasePresentationStrategy::recreateFramebuffer(vk::Extent2D newResolution) {
    // Wait for GPU to finish
    ctx_.device->waitIdle();

    // Destroy old framebuffer
    framebuffer_.reset();

    // Create new framebuffer
    framebuffer_ = std::make_unique<Framebuffer>(
        ctx_,
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

void hammock::app::BasePresentationStrategy::notifyResolutionChanged(uint32_t width, uint32_t height) {
    for (auto &callback: resolutionCallbacks_) {
        callback(width, height);
    }
}

// ***************** Surface presentation strategy *************

hammock::app::SurfacePresentationStrategy::SurfacePresentationStrategy(core::VulkanContext &ctx,
                                                                          core::BaseSurfaceProvider &surfaceProvider,
                                                                          Mode mode)
    : BasePresentationStrategy(
          ctx,
          vk::Extent2D{1920, 1080}, // Initial size, will be updated
          core::ImageFormat::R8G8B8A8Uint,
          core::SwapChain::MAX_FRAMES_IN_FLIGHT // Surface rendering typically uses 2 frames in flight
      ),
      mode_(mode) {
    // Initialize swapchain manager
    swapchainManager_ = std::make_unique<core::SwapChainManager>(surfaceProvider, *ctx_.device);

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
            *ctx_.device, core::CommandQueueFamily::Graphics
        );

        if (mode_ == Mode::Tooling) {
            surfaceRes.uiFinished = std::make_unique<core::Semaphore>(*ctx_.device);
        }
    }

    // Sync framebuffer size with swapchain

    auto extent = swapchainManager_->getSwapChain().getExtent();
    if (framebuffer_->getExtent() != extent) {
        recreateFramebuffer(extent);
    }
}

void hammock::app::SurfacePresentationStrategy::submitUI(
    const FrameContext &ctx,
    std::function<void(core::ResourceHandle, std::uint32_t, core::Semaphore &, core::Semaphore &)> uiRenderFunc
) {
    if (mode_ != Mode::Tooling) {
        return;
    }
    auto frameIndex = swapchainManager_->getFrameIndex();

    uiRenderFunc(
        framebuffer_->getFrontbufferImage(),
        frameIndex,
        *perFrameResources_[frameIndex].renderingFinished,
        *surfacePerFrameResources_[frameIndex].uiFinished);
}

vk::ImageView hammock::app::SurfacePresentationStrategy::getSwapchainImageView(uint32_t index) const {
    return swapchainManager_->getSwapChain().getImageView(index);
}

bool hammock::app::SurfacePresentationStrategy::onBeginFrame() {
    // Acquire swapchain image
    if (!swapchainManager_->beginFrame()) {
        // Swapchain out of date, skip this frame
        return false;
    }

    currentSwapchainImageIndex_ = swapchainManager_->getSwapChainImageIndex();
    return true;
}

void hammock::app::SurfacePresentationStrategy::onPresent(const FrameContext &ctx) {
    auto &swapChain = swapchainManager_->getSwapChain();
    auto &syncObjects = swapChain.getSyncObjects(ctx.frameIndex);
    auto &frameRes = perFrameResources_[ctx.frameIndex];
    auto &surfaceRes = surfacePerFrameResources_[ctx.frameIndex];
    auto &presentCmd = *surfaceRes.presentCommandBuffer;

    // Determine which semaphore to wait on

    core::Semaphore *waitSemaphore = nullptr;

    if (mode_ == Mode::Tooling) {
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

void hammock::app::SurfacePresentationStrategy::onFramebufferRecreated() {
    BasePresentationStrategy::onFramebufferRecreated();
    // Surface strategy might need to do additional work here
    // (though currently nothing needed)
}

void hammock::app::SurfacePresentationStrategy::blitFramebufferToSwapchain(const FrameContext &ctx) {
    auto &surfaceRes = surfacePerFrameResources_[ctx.frameIndex];
    auto &presentCmd = *surfaceRes.presentCommandBuffer;
    auto imageHandle = framebuffer_->getFrontbufferImage();
    auto image = ctx_.resourceManager->getResource<core::Image>(imageHandle);

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

    swapchainManager_->blitToSwapChainImage(presentCmd, image);
}
