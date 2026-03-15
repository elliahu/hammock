#include "render_backend.hpp"

#include <thread>

#include "command_buffer.hpp"
#include "render_proxy.hpp"
#include "utilities.hpp"

hammock::renderer::RenderBackend::RenderBackend(
    RenderProxy& proxy, core::SurfaceProviderIface& surfaceProvider)
    : proxy_(proxy), surfaceProvider_(surfaceProvider) {
    // Initialize renderer
    renderer_ = std::make_unique<renderer::Renderer>([&](core::Instance& instance) {
        surfaceProvider.createVulkanSurface(instance);
        return surfaceProvider.getSurface();
    }, [&](core::Instance& instance, vk::SurfaceKHR surface) {
        surfaceProvider.destroyVulkanSurface(instance);
    });

    // Initialize presentation engine
    // We are using the tooling mode (with ui) even if no ui is present
    // That is because if the ui is not present, empty draw command is submitted
    // Might rework this later
    auto presentStrategy = std::make_unique<SurfacePresentationStrategy>(
        renderer_->getDevice(), surfaceProvider, SurfacePresentationStrategy::Mode::Game);
    presentationEngine_ = std::make_unique<PresentationEngine>(std::move(presentStrategy));
}

void hammock::renderer::RenderBackend::start() {
    running_.store(true);

    thread_ = std::thread([this]() {
        core::Logger::debug("%s", "Render thread created");

        proxy_.getBtfCommandQueue().push(RenderCommand{.type = RenderCommandType::Ready});

        // Enter the loop
        while (running_.load(std::memory_order_acquire)) {
            // First process commands from the logic thread
            RenderCommand cmd;
            while (proxy_.getFtbCommandQueue().pop(cmd)) {
                if (cmd.type == RenderCommandType::Stop) {
                    core::Logger::debug("%s", "Stop command received, exiting backend thread");
                    stop();
                    break;  // Exit thread
                }
                if (cmd.type == RenderCommandType::Resize) {
                    // TODO
                }
            }

            // Consume render packet
            // Skip rendering if no packet is added
            RenderSnapshot renderSnap;
            if (!proxy_.getRenderQueue().pop(renderSnap)) {
                continue;
            }

            // Begin frame
            if (auto frameCtx = presentationEngine_->beginFrame()) {
                // Draw frame
                renderer_->drawFrame(frameCtx->renderTarget, renderSnap, frameCtx->renderFinished);

                // This here is a technical debt from one of the previous iterations of hammock
                // Not time to redo this now, maybe some day
                // The whole presentation engine is a mess
                if (auto* surfaceStrategy =
                        presentationEngine_->getStrategyAs<SurfacePresentationStrategy>()) {
                    // Render UI (editor only)
                    surfaceStrategy->submitUI(
                        *frameCtx, [this](core::CommandBuffer& cmd, std::uint32_t frameIndex) {
                            // This is left empty intentionally
                        });
                }

                // Present
                presentationEngine_->endFrame(*frameCtx);
            }
        }

        renderer_->getDevice().waitIdle();
    });
}

void hammock::renderer::RenderBackend::stop() { running_.store(false, std::memory_order_release); }
bool hammock::renderer::RenderBackend::isRunning() const { return running_.load(std::memory_order_relaxed); }
void hammock::renderer::RenderBackend::join() { thread_.join(); }
