module;

#include <memory>
#include <array>
#include <vulkan/vulkan.hpp>

export module hammock.engine.editor;

import :window;
import :framebuffer;
import hammock.renderer.renderer;
import hammock.renderer.graphics_context;
import hammock.engine.ui;
import hammock.core.swapchain_manager;
import hammock.core.command_buffer;
import hammock.core.swapchain;
import hammock.core.device;

namespace hammock::engine {
    /// @enum LaunchMode
    /// @brief Describes how the engine is launched
    export enum class LaunchMode {
        Editor,
        Runtime,
    };

    /// @class Editor
    /// @brief Represents engines editor
    export class Editor final {
    public:
        Editor(
            const LaunchMode mode,
            renderer::GraphicsContext &ctx)
            : mode(mode), context(ctx) {
            // Create a window
            window = std::make_unique<Window>("Hammock engine", context.getInstance(), 1920u, 1080u);

            // Attach the window surface to the graphics context
            context.attachSurface(window->getSurface());

            // Create renderer
            auto strategy = std::make_unique<renderer::DeferredRenderingStrategy>(context);
            renderer = std::make_unique<renderer::Renderer>(std::move(strategy));

            // Init frame manager
            core::SwapChainManager::initialize(*window, context.getDevice());
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
                auto semaphore = std::make_unique<core::Semaphore>(context.getDevice());
                frameFinishedSemaphores.push_back(std::move(semaphore));

                // Command buffers
                auto commandBuffer = std::make_unique<core::CommandBuffer>(
                    context.getDevice(), core::CommandQueueFamily::Graphics);
                presentCommandBuffers.push_back(std::move(commandBuffer));
            });
        }

        ~Editor() {
            // Destroy frame manager once not needed
            core::SwapChainManager::dispose();
        }

        /// @brief Launch the editor window, this will also start the rendering
        /// This function is the entrypoint into the engine rendering
        void launch() {
            loop();
        }

    private:
        void loop() {
            while (!window->shouldClose()) {
                // Poll for i/o events
                window->pollEvents();

                // Get SwapChain manager instance
                auto &swapChainManager = core::SwapChainManager::getInstance();

                // Begin frame (also resets SwapChain fences)
                if (swapChainManager.beginFrame()) {
                    // Swap images
                    framebuffer->swapImages();

                    // Get current framebuffer image
                    core::ResourceHandle target = framebuffer->getFrontbufferImage();

                    // Get current signal semaphore
                    core::Semaphore &frameFinished = *frameFinishedSemaphores[swapChainManager.getFrameIndex()];

                    // Get current wait semaphore
                    core::Semaphore &frameBufferReady = framebuffer->getFramebufferReadySemaphore();

                    // Draw the frame
                    renderer->drawFrame(target, frameBufferReady, frameFinished);

                    // Present the image
                    present();

                    // End the frame
                    swapChainManager.endFrame();
                }
            }
            // Wait for all pending operation before exiting
            context.getDevice().waitIdle();
        }

        void present() {
            auto &swapChainManager = core::SwapChainManager::getInstance();
            auto &swapChain = swapChainManager.getSwapChain();

            // Get current frame's synchronization objects
            uint32_t frameIndex = swapChainManager.getFrameIndex();
            uint32_t imageIndex = swapChainManager.getSwapChainImageIndex();
            auto &syncObjects = swapChain.getSyncObjects(frameIndex);

            // Get user semaphores
            core::Semaphore &frameFinishedSemaphore = *frameFinishedSemaphores[frameIndex];
            auto &commandBuffer = *presentCommandBuffers[frameIndex];

            // Wait for frame to finish rendering
            commandBuffer.waitOnSemaphore(frameFinishedSemaphore, vk::PipelineStageFlagBits2::eTopOfPipe);

            // Wait for available swap chain image
            commandBuffer.waitOnSemaphore(*syncObjects.imageAvailable,
                                          vk::PipelineStageFlagBits2::eColorAttachmentOutput);

            // Signal swap chain that rendering is finished
            commandBuffer.signalSemaphore(*syncObjects.renderFinished);

            // Begin present command buffer
            commandBuffer.begin();

            auto target = core::ResourceManager::getInstance().getResource<core::Image>(
                framebuffer->getFrontbufferImage());

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
            blit.srcOffsets[0] = {0, 0, 0};
            blit.srcOffsets[1] = {1920, 1080, 1};

            blit.dstSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
            blit.dstSubresource.mipLevel = 0;
            blit.dstSubresource.baseArrayLayer = 0;
            blit.dstSubresource.layerCount = 1;
            blit.dstOffsets[0] = {0, 0, 0};
            blit.dstOffsets[1] = {1920, 1080, 1};

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

        void recreateFramebuffer() {
            // Destroy the old framebuffer
            framebuffer.reset();
            // Create new one
            auto &sc = core::SwapChainManager::getInstance();
            auto format = sc.getSwapChain().getSwapChainImageFormat();
            auto extent = sc.getSwapChain().getSwapChainExtent();
            framebuffer = std::make_unique<Framebuffer>(context.getDevice(), core::SwapChain::MAX_FRAMES_IN_FLIGHT,
                                                        math::Vec2{
                                                            static_cast<float>(extent.width),
                                                            static_cast<float>(extent.height)
                                                        }, format);
        }

        LaunchMode mode;
        renderer::GraphicsContext &context;
        std::unique_ptr<renderer::Renderer> renderer;
        std::unique_ptr<Window> window;
        std::unique_ptr<Framebuffer> framebuffer;
        std::vector<std::unique_ptr<core::Semaphore> > frameFinishedSemaphores;
        std::vector<std::unique_ptr<core::CommandBuffer> > presentCommandBuffers;

        math::Vec2 extent;
    };
}
