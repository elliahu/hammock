#include "presentation_engine.hpp"

#include <compare>
#include <functional>
#include <memory>
#include <thread>
#include <vulkan/vulkan.hpp>

#include "semaphore.hpp"
#include "utilities.hpp"

// ************ Base presentation strategy *************

std::optional<hammock::renderer::FrameContext> hammock::renderer::BasePresentationStrategy::beginFrame() {
    if (frameInProgress_) {
        throw std::runtime_error("Frame already in progress");
    }

    // Strategy-specific pre-frame logic
    if (!onBeginFrame()) {
        // Strategy says skip this frame (e.g., swapchain out of date)
        return std::nullopt;
    }

    // Per frame resources
    auto frameRes = perFrameResources_[currentFrameIndex_];

    // Swap buffers
    framebuffer_->swapImages();

    frameInProgress_ = true;

    return FrameContext{
        .renderTarget = framebuffer_->getFrontbufferImage(),
        .renderFinished = semaphores_.ref(frameRes.renderingFinished),
        .frameIndex = currentFrameIndex_,
    };
}

void hammock::renderer::BasePresentationStrategy::endFrame(const FrameContext& ctx) {
    if (!frameInProgress_) {
        throw std::runtime_error("No frame in progress");
    }

    // Strategy-specific presentation logic
    onPresent(ctx);

    // Advance frame index
    currentFrameIndex_ = (currentFrameIndex_ + 1) % framesInFlight_;
    frameInProgress_ = false;
}

vk::Extent2D hammock::renderer::BasePresentationStrategy::getResolution() const {
    return framebuffer_->getExtent();
}

hammock::core::ImageFormat hammock::renderer::BasePresentationStrategy::getFormat() const {
    return core::ImageFormat::R8G8B8A8Uint;  // TODO: get from framebuffer
}

hammock::renderer::BasePresentationStrategy::BasePresentationStrategy(
    core::Device& device, vk::Extent2D resolution, core::ImageFormat format, uint32_t framesInFlight)
    : device_(device), framesInFlight_(framesInFlight) {
    // Create framebuffer (shared by all strategies)
    framebuffer_ = std::make_unique<Framebuffer>(device_,
        framesInFlight_,
        math::Vec2{static_cast<float>(resolution.width), static_cast<float>(resolution.height)},
        format);

    // Create per-frame synchronization (shared by all strategies)
    perFrameResources_.resize(framesInFlight_);
    for (auto& frameRes : perFrameResources_) {
        frameRes.renderingFinished = semaphores_.create(device_);
    }
}

void hammock::renderer::BasePresentationStrategy::recreateFramebuffer(vk::Extent2D newResolution) {
    // Wait for GPU to finish
    device_.waitIdle();

    // Destroy old framebuffer
    framebuffer_.reset();

    // Create new framebuffer
    framebuffer_ = std::make_unique<Framebuffer>(device_,
        framesInFlight_,
        math::Vec2{static_cast<float>(newResolution.width), static_cast<float>(newResolution.height)},
        getFormat());

    // Notify subclass
    onFramebufferRecreated();

    // Notify listeners
    notifyResolutionChanged(newResolution.width, newResolution.height);
}

void hammock::renderer::BasePresentationStrategy::notifyResolutionChanged(uint32_t width, uint32_t height) {
    for (auto& callback : resolutionCallbacks_) {
        callback(width, height);
    }
}

// ***************** Surface presentation strategy *************

hammock::renderer::SurfacePresentationStrategy::SurfacePresentationStrategy(
    core::Device& device, core::SurfaceProviderIface& surfaceProvider, Mode mode)
    : BasePresentationStrategy(device, vk::Extent2D{1920, 1080},  // Initial size, will be updated
          core::ImageFormat::R8G8B8A8Uint,
          core::SwapChain::MAX_FRAMES_IN_FLIGHT  // Surface rendering typically uses 2 frames in flight
          ),
      mode_(mode) {
    // Initialize swapchain manager
    swapchainManager_ = std::make_unique<core::SwapChainManager>(surfaceProvider, device);

    // Register swapchain recreation callback
    swapchainManager_->registerOnSwapChainRecreatedCallback(
        [this](uint32_t width, uint32_t height) { recreateFramebuffer(vk::Extent2D{width, height}); });

    // Create surface-specific resources

    surfacePerFrameResources_.resize(framesInFlight_);
    for (auto& surfaceRes : surfacePerFrameResources_) {
        surfaceRes.presentCommandBuffer = commandBuffers_.create(device, core::CommandQueueFamily::Graphics);

        if (mode_ == Mode::Tooling) {
            surfaceRes.uiFinished = semaphores_.create(device_);
            surfaceRes.uiCommandBuffer = commandBuffers_.create(device, core::CommandQueueFamily::Graphics);
        }
    }

    // Sync framebuffer size with swapchain

    auto extent = swapchainManager_->getSwapChain().getExtent();
    if (framebuffer_->getExtent() != extent) {
        recreateFramebuffer(extent);
    }
}

void hammock::renderer::SurfacePresentationStrategy::submitUI(
    const FrameContext& ctx, RenderUiCallback uiRenderFunc) {
    if (mode_ != Mode::Tooling) {
        return;
    }
    auto frameIndex = swapchainManager_->getFrameIndex();

    auto targetImage = framebuffer_->getFrontbufferImage();
    vk::RenderingAttachmentInfo colorAttachment = targetImage->getRenderingAttachmentInfo();
    colorAttachment.loadOp = vk::AttachmentLoadOp::eLoad;
    colorAttachment.storeOp = vk::AttachmentStoreOp::eStore;

    vk::RenderingInfo renderInfo = {};
    renderInfo.renderArea =
        vk::Rect2D{{0, 0}, {targetImage->getExtent().width, targetImage->getExtent().height}};
    renderInfo.layerCount = 1;
    renderInfo.colorAttachmentCount = 1;
    renderInfo.pColorAttachments = &colorAttachment;

    auto cmd = commandBuffers_.ref(surfacePerFrameResources_[frameIndex].uiCommandBuffer);
    cmd->addWaitSemaphore(semaphores_.ref(perFrameResources_[frameIndex].renderingFinished),
        vk::PipelineStageFlagBits2::eColorAttachmentOutput);
    cmd->addSignalSemaphore(semaphores_.ref(surfacePerFrameResources_[frameIndex].uiFinished));
    cmd->begin();
    cmd->getCommandBuffer().beginRendering(&renderInfo);
    uiRenderFunc(cmd.get(), frameIndex);
    cmd->getCommandBuffer().endRendering();
    cmd->submit();
}

vk::ImageView hammock::renderer::SurfacePresentationStrategy::getSwapchainImageView(uint32_t index) const {
    return swapchainManager_->getSwapChain().getImageView(index);
}

bool hammock::renderer::SurfacePresentationStrategy::onBeginFrame() {
    // Acquire swapchain image
    if (!swapchainManager_->beginFrame()) {
        // Swapchain out of date, skip this frame
        return false;
    }

    currentSwapchainImageIndex_ = swapchainManager_->getSwapChainImageIndex();
    return true;
}

void hammock::renderer::SurfacePresentationStrategy::onPresent(const FrameContext& ctx) {
    auto& swapChain = swapchainManager_->getSwapChain();
    auto syncObjects = swapChain.getSyncObjects(ctx.frameIndex);
    auto presentCmd = commandBuffers_.ref(surfacePerFrameResources_[ctx.frameIndex].presentCommandBuffer);

    // Determine which semaphore to wait on

    core::ResourceRef<core::Semaphore> waitSemaphore;

    if (mode_ == Mode::Tooling) {
        // Wait for UI to finish
        waitSemaphore = semaphores_.ref(surfacePerFrameResources_[ctx.frameIndex].uiFinished);
    } else {
        // Wait for scene rendering to finish
        waitSemaphore = semaphores_.ref(perFrameResources_[ctx.frameIndex].renderingFinished);
    }

    // Set up presentation command buffer
    // Wai wait for two semaphores here
    presentCmd->addWaitSemaphore(waitSemaphore, vk::PipelineStageFlagBits2::eTopOfPipe);
    presentCmd->addWaitSemaphore(
        syncObjects.imageAvailable, vk::PipelineStageFlagBits2::eColorAttachmentOutput);
    presentCmd->addSignalSemaphore(syncObjects.frameFinished);

    // Record blit operation

    presentCmd->begin();
    blitFramebufferToSwapchain(ctx);
    presentCmd->submit(
        swapchainManager_->getSwapChain().getSyncObjects(swapchainManager_->getFrameIndex()).inFlightFence);

    // Present
    swapchainManager_->present();
    swapchainManager_->endFrame();
}

void hammock::renderer::SurfacePresentationStrategy::onFramebufferRecreated() {
    BasePresentationStrategy::onFramebufferRecreated();
    // Surface strategy might need to do additional work here
    // (though currently nothing needed)
}

void hammock::renderer::SurfacePresentationStrategy::blitFramebufferToSwapchain(const FrameContext& ctx) {
    auto& surfaceRes = surfacePerFrameResources_[ctx.frameIndex];
    auto presentCmd = commandBuffers_.ref(surfaceRes.presentCommandBuffer);
    auto image = framebuffer_->getFrontbufferImage();

    // Transition the target image to transfer scr optimal
    presentCmd->imagePipelineBarrier(image,
        vk::PipelineStageFlagBits2::eColorAttachmentOutput,
        vk::AccessFlagBits2::eColorAttachmentWrite,
        vk::PipelineStageFlagBits2::eTransfer,
        vk::AccessFlagBits2::eTransferRead,
        vk::ImageLayout::eColorAttachmentOptimal,
        vk::ImageLayout::eTransferSrcOptimal);

    swapchainManager_->blitToSwapChainImage(presentCmd, image);
}
