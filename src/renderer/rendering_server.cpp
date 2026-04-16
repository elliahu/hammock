#include "rendering_server.hpp"

#include <thread>

#include "utilities.hpp"

hammock::renderer::RenderQueue& hammock::renderer::RenderingServerProxy::getRenderQueue() { return renderQueue_; }
hammock::renderer::CommandQueue& hammock::renderer::RenderingServerProxy::getFtbCommandQueue() { return requestQueue; }
hammock::renderer::CommandQueue& hammock::renderer::RenderingServerProxy::getBtfCommandQueue() {
    return responseQueue;
}

hammock::renderer::RenderingServer::RenderingServer(RenderingServerProxy& proxy,
    core::SurfaceProviderIface& surfaceProvider, std::unique_ptr<RendererIface>&& renderer,
    std::unique_ptr<PresenterIface>&& presenter)
    : proxy_(proxy),
      surfaceProvider_(surfaceProvider),
      renderer_(std::move(renderer)),
      presenter_(std::move(presenter)) {}

void hammock::renderer::RenderingServer::start() {
    running_.store(true);

    thread_ = std::thread([this]() {
        core::Logger::debug("%s", "Render thread created");

        // Signal logic thread that backend is ready
        proxy_.getBtfCommandQueue().push(RenderCommand{.type = RenderCommandType::Ready});

        // Enter the loop
        while (running_.load(std::memory_order_acquire)) {
            // measure frame start
            auto frameStart = std::chrono::high_resolution_clock::now();
            // First process commands from the logic thread
            RenderCommand cmd;
            while (proxy_.getFtbCommandQueue().pop(cmd)) {
                if (cmd.type == RenderCommandType::Stop) {
                    core::Logger::debug("%s", "Stop command received, exiting backend thread");
                    stop();
                    break;  // Break out of the render loop
                }
                if (cmd.type == RenderCommandType::Resize) {
                    renderer_->handleResize(cmd.width, cmd.height);
                    // Resize handled
                    surfaceProvider_.resetResized();
                }
            }

            // Consume render packet
            RenderSnapshot renderSnap;
            if (!proxy_.getRenderQueue().pop(renderSnap)) {
                // No packet, skip frame, this indicates the logic thread is still working on a new frame
                // Might be due to heavy logic thread workload
                continue;
            }

            auto commandProcessingEnd = std::chrono::high_resolution_clock::now();

            // Begin frame
            if (auto frameCtx = presenter_->beginFrame()) {
                // Draw frame
                renderer_->drawFrame(frameCtx->renderTarget, renderSnap, frameCtx->renderFinished);

                // Present
                presenter_->endFrame(*frameCtx);
            }

            auto frameEnd = std::chrono::high_resolution_clock::now();
            std::chrono::duration<float, std::milli> commandProcessingTime =
                commandProcessingEnd - frameStart;
            std::chrono::duration<float, std::milli> frameTime = frameEnd - frameStart;
            std::chrono::duration<float, std::milli> renderTime = frameEnd - commandProcessingEnd;
            float lastFrameMs = frameTime.count();
            float commandProcessingTimeMs = commandProcessingTime.count();
            float renderTimeMs = renderTime.count();
            proxy_.getBtfCommandQueue().push(RenderCommand{.type = RenderCommandType::Frametime,
                .frametime = lastFrameMs,
                .rendertime = renderTimeMs,
                .cmdtime = commandProcessingTimeMs});
        }

        // Finish the rendering before stopping
        renderer_->waitIdle();
    });
}

void hammock::renderer::RenderingServer::stop() { running_.store(false, std::memory_order_release); }

bool hammock::renderer::RenderingServer::isRunning() const { return running_.load(std::memory_order_relaxed); }

void hammock::renderer::RenderingServer::join() { thread_.join(); }
