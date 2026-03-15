#include "render_backend.hpp"

#include <thread>

#include "command_buffer.hpp"
#include "render_proxy.hpp"
#include "utilities.hpp"

hammock::renderer::RenderBackend::RenderBackend(
    RenderProxy& proxy, core::SurfaceProviderIface& surfaceProvider, std::unique_ptr<RendererIface>&& renderer, std::unique_ptr<PresenterIface>&& presenter)
    : proxy_(proxy), surfaceProvider_(surfaceProvider), renderer_(std::move(renderer)), presenter_(std::move(presenter)) {
}

void hammock::renderer::RenderBackend::start() {
    running_.store(true);

    thread_ = std::thread([this]() {
        core::Logger::debug("%s", "Render thread created");

        // Signal logic thread that backend is ready
        proxy_.getBtfCommandQueue().push(RenderCommand{.type = RenderCommandType::Ready});

        // Enter the loop
        while (running_.load(std::memory_order_acquire)) {
            // First process commands from the logic thread
            RenderCommand cmd;
            while (proxy_.getFtbCommandQueue().pop(cmd)) {
                if (cmd.type == RenderCommandType::Stop) {
                    core::Logger::debug("%s", "Stop command received, exiting backend thread");
                    stop();
                    break;  // Break out of the render loop
                }
                if (cmd.type == RenderCommandType::Resize) {
                    renderer_->handleResize(cmd);
                }
            }

            // Consume render packet
            // Skip rendering if no packet is added
            RenderSnapshot renderSnap;
            if (!proxy_.getRenderQueue().pop(renderSnap)) {
                continue;
            }

            // Begin frame
            if(auto frameCtx = presenter_->beginFrame()) {

                // Draw frame
                renderer_->drawFrame(frameCtx->renderTarget, renderSnap, frameCtx->renderFinished);

                // Present
                presenter_->endFrame(*frameCtx);
            }
        }

        // Finish the rendering before stopping
        renderer_->waitIdle();
    });
}

void hammock::renderer::RenderBackend::stop() { running_.store(false, std::memory_order_release); }
bool hammock::renderer::RenderBackend::isRunning() const { return running_.load(std::memory_order_relaxed); }
void hammock::renderer::RenderBackend::join() { thread_.join(); }
