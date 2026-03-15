#include "presenter.hpp"

#include <stdexcept>

#include "utilities.hpp"
#include "utils/math.hpp"

namespace hammock::renderer {

    Presenter::Presenter(
        core::Device& device, core::SurfaceProviderIface& surfaceProvider, const PresenterDesc& desc)
        : device_(device), framesInFlight_(core::SwapChain::MAX_FRAMES_IN_FLIGHT), desc_(desc) {
        // The swapchain is always the size of the surface
        swapchainManager_ = std::make_unique<core::SwapChainManager>(surfaceProvider, device_);

        // Create the framebuffer
        auto framebufferSuze = getFramebufferSize();
        core::Logger::debug("Initializing presenter with framebuffer res %.0f px X %.0f px",
            framebufferSuze.X,
            framebufferSuze.Y);
        framebuffer_ = std::make_unique<Framebuffer>(
            device_, framesInFlight_, framebufferSuze, core::ImageFormat::R8G8B8A8Uint);

        // Recreate framebuffer whenever the swapchain is rebuilt
        swapchainManager_->registerOnSwapChainRecreatedCallback(
            [this](uint32_t w, uint32_t h) { recreateFramebuffer(vk::Extent2D{w, h}); });

        // Per-frame GPU resources
        perFrameResources_.resize(framesInFlight_);
        for (auto& res : perFrameResources_) {
            res.renderingFinished = semaphores_.create(device_);
            res.presentCommandBuffer = commandBuffers_.create(device_, core::CommandQueueFamily::Graphics);
        }
    }

    std::optional<FrameContext> Presenter::beginFrame() {
        if (frameInProgress_) throw std::runtime_error("Frame already in progress");

        // Returns false when the swapchain is out of date; caller should retry next tick
        if (!swapchainManager_->beginFrame()) return std::nullopt;

        currentSwapchainImageIndex_ = swapchainManager_->getSwapChainImageIndex();
        framebuffer_->swapImages();
        frameInProgress_ = true;

        return FrameContext{
            .renderTarget = framebuffer_->getFrontbufferImage(),
            .renderFinished = semaphores_.ref(perFrameResources_[currentFrameIndex_].renderingFinished),
            .frameIndex = currentFrameIndex_,
        };
    }

    void Presenter::endFrame(const FrameContext& ctx) {
        if (!frameInProgress_) throw std::runtime_error("No frame in progress");

        auto& res = perFrameResources_[ctx.frameIndex];
        auto& swapChain = swapchainManager_->getSwapChain();
        auto syncObjects = swapChain.getSyncObjects(ctx.frameIndex);
        auto presentCmd = commandBuffers_.ref(res.presentCommandBuffer);

        // Wait for the renderer to finish writing and for the swapchain image to be free
        presentCmd->addWaitSemaphore(
            semaphores_.ref(res.renderingFinished), vk::PipelineStageFlagBits2::eTopOfPipe);
        presentCmd->addWaitSemaphore(
            syncObjects.imageAvailable, vk::PipelineStageFlagBits2::eColorAttachmentOutput);
        presentCmd->addSignalSemaphore(syncObjects.frameFinished);

        presentCmd->begin();
        blitFramebufferToSwapchain(ctx);
        presentCmd->submit(swapChain.getSyncObjects(swapchainManager_->getFrameIndex()).inFlightFence);

        swapchainManager_->present();
        swapchainManager_->endFrame();

        currentFrameIndex_ = (currentFrameIndex_ + 1) % framesInFlight_;
        frameInProgress_ = false;
    }

    vk::Extent2D Presenter::getResolution() const { return framebuffer_->getExtent(); }

    core::ImageFormat Presenter::getFormat() const { return core::ImageFormat::R8G8B8A8Uint; }

    void Presenter::onResolutionChanged(std::function<void(uint32_t, uint32_t)> callback) {
        resolutionCallbacks_.push_back(std::move(callback));
    }

    void Presenter::recreateFramebuffer(vk::Extent2D newResolution) {
        device_.waitIdle();  // Wait for the device to be idle before recreating the framebuffer
        framebuffer_.reset();

        auto framebufferSuze = getFramebufferSize();
        core::Logger::debug(
            "Recreating framebuffer with res %.0f px X %.0f px", framebufferSuze.X, framebufferSuze.Y);

        framebuffer_ =
            std::make_unique<Framebuffer>(device_, framesInFlight_, getFramebufferSize(), getFormat());
        notifyResolutionChanged(newResolution.width, newResolution.height);
    }

    void Presenter::blitFramebufferToSwapchain(const FrameContext& ctx) {
        auto presentCmd = commandBuffers_.ref(perFrameResources_[ctx.frameIndex].presentCommandBuffer);
        auto image = framebuffer_->getFrontbufferImage();

        presentCmd->imagePipelineBarrier(image,
            vk::PipelineStageFlagBits2::eColorAttachmentOutput,
            vk::AccessFlagBits2::eColorAttachmentWrite,
            vk::PipelineStageFlagBits2::eTransfer,
            vk::AccessFlagBits2::eTransferRead,
            vk::ImageLayout::eColorAttachmentOptimal,
            vk::ImageLayout::eTransferSrcOptimal);

        swapchainManager_->blitToSwapChainImage(presentCmd, image);
    }

    void Presenter::notifyResolutionChanged(uint32_t width, uint32_t height) {
        for (auto& cb : resolutionCallbacks_) cb(width, height);
    }

    math::Vec2 Presenter::getFramebufferSize() const {
        // Set the initial frambuffer size
        const auto swapchainExtent = swapchainManager_->getSwapChain().getExtent();
        math::Vec2 framebufferSize = {};

        if (desc_.frameBufferRelativeSize == PresenterFrameBufferSize::SwapchainRelative) {
            framebufferSize = {static_cast<float>(swapchainExtent.width) * desc_.frameBufferSize.X,
                static_cast<float>(swapchainExtent.height) * desc_.frameBufferSize.Y};
        } else if (desc_.frameBufferRelativeSize == PresenterFrameBufferSize::Absolute) {
            framebufferSize = desc_.frameBufferSize;
        }

        return framebufferSize;
    }
}  // namespace hammock::renderer
