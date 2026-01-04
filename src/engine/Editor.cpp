module;

#include <compare>
#include <vulkan/vulkan.hpp>

module hammock.engine.editor;

import hammock.core;
import hammock.renderer.deferred_rendering_strategy;

hammock::engine::Editor::Editor(const LaunchMode mode, renderer::GraphicsContext &ctx) : mode_(mode), context_(ctx) {
    // Create a window
    window_ = std::make_unique<Window>("Hammock engine", context_.getInstance(), 1920u, 1080u);

    // Attach the window surface to the graphics context
    context_.attachSurface(window_->getSurface());

    // Create renderer
    auto strategy = std::make_unique<renderer::DeferredRenderingStrategy>(context_);
    renderer_ = std::make_unique<renderer::Renderer>(std::move(strategy));

    // Init frame manager
    core::SwapChainManager::initialize(*window_, context_.getDevice());
    auto &sc = core::SwapChainManager::getInstance();

    // Create offscreen framebuffer
    recreateFramebuffer();
    sc.registerOnSwapChainRecreatedCallback([this](std::uint32_t width, std::uint32_t height) {
        this->recreateFramebuffer();
    });

    // Create semaphores that are signaled when frame is finished rendering
    // Also creat command present buffers
    core::SwapChain::forEachFrameInFlight([this](int frame) {
        // Semaphores
        auto semaphore = std::make_unique<core::Semaphore>(context_.getDevice());
        rendererFinishedSemaphores_.push_back(std::move(semaphore));

        // Command buffers
        auto commandBuffer = std::make_unique<core::CommandBuffer>(
            context_.getDevice(), core::CommandQueueFamily::Graphics);
        presentCommandBuffers_.push_back(std::move(commandBuffer));
    });

    // Only set up the ui in editor mode
    if (mode_ == LaunchMode::Editor) {
        // Create the ui
        ui_ = std::make_unique<Ui>(window_->getWindowPtr(), &context_);
        // Create the ui command buffers and sync semaphores
        core::SwapChain::forEachFrameInFlight([this](int frame) {
            auto commandBuffer = std::make_unique<core::CommandBuffer>(
                context_.getDevice(), core::CommandQueueFamily::Graphics);
            uiCommandBuffers_.push_back(std::move(commandBuffer));

            auto semaphore = std::make_unique<core::Semaphore>(context_.getDevice());
            uiFinishedSemaphores_.push_back(std::move(semaphore));
        });
    }
}

hammock::engine::Editor::~Editor() {
    // Destroy frame manager once not needed
    core::SwapChainManager::dispose();
}

void hammock::engine::Editor::launch() {
    loop();
}

void hammock::engine::Editor::loop() {
    while (!window_->shouldClose()) {
        // Poll for i/o events
        window_->pollEvents();

        // Get SwapChain manager instance
        auto &swapChainManager = core::SwapChainManager::getInstance();

        // Begin frame (also resets SwapChain fences)
        if (swapChainManager.beginFrame()) {
            // Swap images
            framebuffer_->swapImages();

            // Get current framebuffer image
            core::ResourceHandle target = framebuffer_->getFrontbufferImage();

            // Get current signal semaphore
            core::Semaphore &rendererFinished = *rendererFinishedSemaphores_[swapChainManager.getFrameIndex()];

            // Get current wait semaphore
            core::Semaphore &frameBufferReady = framebuffer_->getFramebufferReadySemaphore();

            // Draw the frame
            renderer_->drawFrame(target, frameBufferReady, rendererFinished);

            // Draw the ui
            if (mode_ == LaunchMode::Editor) {
                drawUi();
            }

            // Present the image
            present();

            // End the frame
            swapChainManager.endFrame();
        }
    }
    // Wait for all pending operation before exiting
    context_.getDevice().waitIdle();
}

void hammock::engine::Editor::drawUi() {
    auto &swapChainManager = core::SwapChainManager::getInstance();
    auto &commandBuffer = *uiCommandBuffers_[swapChainManager.getFrameIndex()];
    // Wait for renderer to finish
    commandBuffer.waitOnSemaphore(*rendererFinishedSemaphores_[swapChainManager.getFrameIndex()],
                                  vk::PipelineStageFlagBits2::eTopOfPipe);
    // Signal when ui is finished
    commandBuffer.signalSemaphore(*uiFinishedSemaphores_[swapChainManager.getFrameIndex()]);

    // Begin ui rendering
    commandBuffer.begin();
    auto extent = swapChainManager.getSwapChain().getSwapChainExtent();
    ui_->renderFrame(commandBuffer, swapChainManager.getSwapChain().getImageView(swapChainManager.getSwapChainImageIndex()), extent.width, extent.height);
    // End ui rendering
    commandBuffer.submit();
}

void hammock::engine::Editor::present() const {
    auto &swapChainManager = core::SwapChainManager::getInstance();
    auto &swapChain = swapChainManager.getSwapChain();

    // Get current frame's synchronization objects
    uint32_t frameIndex = swapChainManager.getFrameIndex();
    uint32_t imageIndex = swapChainManager.getSwapChainImageIndex();
    auto &syncObjects = swapChain.getSyncObjects(frameIndex);
    auto &commandBuffer = *presentCommandBuffers_[frameIndex];

    if (mode_ == LaunchMode::Editor) {
        // Wait for frame to finish rendering
        core::Semaphore &frameFinishedSemaphore = *uiFinishedSemaphores_[swapChainManager.getFrameIndex()];
        commandBuffer.waitOnSemaphore(frameFinishedSemaphore, vk::PipelineStageFlagBits2::eTopOfPipe);
    } else if (mode_ == LaunchMode::Runtime) {
        core::Semaphore &frameFinishedSemaphore = *rendererFinishedSemaphores_[swapChainManager.getFrameIndex()];
        commandBuffer.waitOnSemaphore(frameFinishedSemaphore, vk::PipelineStageFlagBits2::eTopOfPipe);
    }

    // Wait for available swap chain image
    commandBuffer.waitOnSemaphore(*syncObjects.imageAvailable,
                                  vk::PipelineStageFlagBits2::eColorAttachmentOutput);

    // Signal swap chain that rendering is finished
    commandBuffer.signalSemaphore(*syncObjects.renderFinished);

    // Begin present command buffer
    commandBuffer.begin();

    auto target = core::ResourceManager::getInstance().getResource<core::Image>(
        framebuffer_->getFrontbufferImage());

    // target color attachment -> transfer src
    target->recordPipelineBarrier(
        commandBuffer.getCommandBuffer(),
        vk::PipelineStageFlagBits2::eColorAttachmentOutput,
        vk::AccessFlagBits2::eColorAttachmentWrite,
        vk::PipelineStageFlagBits2::eTransfer,
        vk::AccessFlagBits2::eTransferRead,
        vk::ImageLayout::eColorAttachmentOptimal,
        vk::ImageLayout::eTransferSrcOptimal,
        vk::QueueFamilyIgnored,
        vk::QueueFamilyIgnored
    );

    // swapchain undefined -> transfer dst
    swapChain.recordPipelineBarrier(
        imageIndex, // Now explicitly use imageIndex, not frameIndex
        commandBuffer.getCommandBuffer(),
        vk::PipelineStageFlagBits2::eTopOfPipe,
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
    blit.srcOffsets[1] = vk::Offset3D{1920, 1080, 1};

    blit.dstSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
    blit.dstSubresource.mipLevel = 0;
    blit.dstSubresource.baseArrayLayer = 0;
    blit.dstSubresource.layerCount = 1;
    blit.dstOffsets[0] = vk::Offset3D{0, 0, 0};
    blit.dstOffsets[1] = vk::Offset3D{1920, 1080, 1};

    commandBuffer.getCommandBuffer().blitImage(
        target->getImage(), vk::ImageLayout::eTransferSrcOptimal,
        swapChain.getImage(imageIndex), vk::ImageLayout::eTransferDstOptimal,
        1,
        &blit,
        vk::Filter::eLinear
    );

    // swapchain Transfer dst -> PRESENT_SRC_KHR
    swapChain.recordPipelineBarrier(
        imageIndex,
        commandBuffer.getCommandBuffer(),
        vk::PipelineStageFlagBits2::eTransfer,
        vk::AccessFlagBits2::eTransferWrite,
        vk::PipelineStageFlagBits2::eBottomOfPipe,
        vk::AccessFlagBits2::eNone,
        vk::ImageLayout::eTransferDstOptimal,
        vk::ImageLayout::ePresentSrcKHR,
        vk::QueueFamilyIgnored,
        vk::QueueFamilyIgnored
    );

    // End the command buffer and submit with fence
    commandBuffer.submit(syncObjects.inFlightFence);

    // Present
    swapChainManager.present();
}

void hammock::engine::Editor::recreateFramebuffer() {
    // Destroy the old framebuffer
    framebuffer_.reset();
    // Create new one
    auto &sc = core::SwapChainManager::getInstance();
    auto format = sc.getSwapChain().getSwapChainImageFormat();
    auto extent = sc.getSwapChain().getSwapChainExtent();
    framebuffer_ = std::make_unique<Framebuffer>(context_.getDevice(), core::SwapChain::MAX_FRAMES_IN_FLIGHT,
                                                 math::Vec2{
                                                     static_cast<float>(extent.width),
                                                     static_cast<float>(extent.height)
                                                 }, format);
}
