module;

#include <memory>
#include <array>
#include <vulkan/vulkan_core.h>

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
                if (swapChainManager.beginFrame()) { // FIXME hangs on a start of the third frame
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
            core::Semaphore &frameFinishedSemaphore = *frameFinishedSemaphores[swapChainManager.getFrameIndex()];
            auto &commandBuffer = *presentCommandBuffers[swapChainManager.getFrameIndex()];

            // Wait for frame to finish rendering
            commandBuffer.waitOnSemaphore(frameFinishedSemaphore, VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT);
            // Wait for available swap chain image
            commandBuffer.waitOnSemaphore(swapChain.getImageAvailableSemaphore(),
                                          VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT);
            // Signal swap chain
            commandBuffer.signalSemaphore(swapChain.getRenderFinishedSemaphore());

            // Begin present command buffer
            commandBuffer.begin();

            auto target = core::ResourceManager::getInstance().getResource<core::Image>(
                framebuffer->getFrontbufferImage());

            // target color attachment -> transfer src
            target->recordPipelineBarrier(
                commandBuffer.getCommandBuffer(),
                VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
                VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
                VK_PIPELINE_STAGE_2_TRANSFER_BIT,
                VK_ACCESS_2_TRANSFER_READ_BIT,
                VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                VK_QUEUE_FAMILY_IGNORED,
                VK_QUEUE_FAMILY_IGNORED
                );

            // swapchain undefined -> transfer dst
            swapChain.recordPipelineBarrier(
                swapChainManager.getSwapChainImageIndex(),
                commandBuffer.getCommandBuffer(),
                VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                VK_ACCESS_2_NONE,
                VK_PIPELINE_STAGE_TRANSFER_BIT,
                VK_ACCESS_TRANSFER_WRITE_BIT,
                VK_IMAGE_LAYOUT_UNDEFINED,
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                VK_QUEUE_FAMILY_IGNORED,
                VK_QUEUE_FAMILY_IGNORED
            );

            //  Blit
            VkImageBlit blit{};
            blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            blit.srcSubresource.mipLevel = 0;
            blit.srcSubresource.baseArrayLayer = 0;
            blit.srcSubresource.layerCount = 1;
            blit.srcOffsets[0] = {0, 0, 0};
            blit.srcOffsets[1] = {1920, 1080, 1};

            blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            blit.dstSubresource.mipLevel = 0;
            blit.dstSubresource.baseArrayLayer = 0;
            blit.dstSubresource.layerCount = 1;
            blit.dstOffsets[0] = {0, 0, 0};
            blit.dstOffsets[1] = {1920, 1080, 1};

            vkCmdBlitImage(
                commandBuffer.getCommandBuffer(),
                target->getImage(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                swapChain.getImage(swapChainManager.getSwapChainImageIndex()), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                1,
                &blit,
                VK_FILTER_LINEAR
            );


            // swapchain Transfer dst -> PRESENT_SRC_KHR
            swapChain.recordPipelineBarrier(
                swapChainManager.getSwapChainImageIndex(),
                commandBuffer.getCommandBuffer(),
                VK_PIPELINE_STAGE_2_TRANSFER_BIT,
                VK_ACCESS_2_TRANSFER_WRITE_BIT,
                VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT,
                VK_ACCESS_2_NONE,
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
                VK_QUEUE_FAMILY_IGNORED,
                VK_QUEUE_FAMILY_IGNORED
            );

            // End the command buffer
            commandBuffer.submit(swapChain.getInFlightFence());

            // Present
            swapChainManager.present();
        }

        void recreateFramebuffer() {
            // Destroy the old framebuffer
            framebuffer.reset();
            // Create new one
            auto &sc = core::SwapChainManager::getInstance();
            auto format = sc.getSwapChain().getSwapChainImageFormat();
            framebuffer = std::make_unique<Framebuffer>(context.getDevice(), core::SwapChain::MAX_FRAMES_IN_FLIGHT,
                                                        math::Vec2{1920.f, 1080.f}, format);
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
