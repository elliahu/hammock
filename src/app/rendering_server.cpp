#include "rendering_server.hpp"

#include <thread>

#include "utilities.hpp"

hammock::app::InfoCommandQueue& hammock::app::RenderingServerProxy::getResponseQueue() {
    return responseQueue;
}

hammock::app::RenderingServer::RenderingServer(RenderingServerProxy& proxy,
    core::SurfaceProviderIface& surfaceProvider, std::unique_ptr<renderer::RendererIface>&& renderer,
    std::unique_ptr<PresenterIface>&& presenter)
    : proxy_(proxy),
      surfaceProvider_(surfaceProvider),
      renderer_(std::move(renderer)),
      presenter_(std::move(presenter)) {}

void hammock::app::RenderingServer::start() {
    running_.store(true);

    thread_ = std::thread([this]() {
        core::Logger::debug("%s", "Render thread created");

        // Signal logic thread that backend is ready
        proxy_.getResponseQueue().push(
            RenderingServerInfoCommand{ .type = RenderingServerInfoCommandType::Ready });

        // Enter the loop
        while (running_.load(std::memory_order_acquire)) {
            // measure frame start
            auto frameStart = std::chrono::high_resolution_clock::now();

            // Consume draw command
            RenderingServerDrawCommand drawCmd = proxy_.getDrawQueue().pop();
            // core::Logger::debug("Rendering Server:Collected frame %d", drawCmd.frameNumber);

            // Check for poison pill
            if (drawCmd.snapHeader == nullptr) {
                core::Logger::debug("%s", "Poison pill received, exiting main render loop");
                break;
            }
            const void* snapData = reinterpret_cast<const void*>(drawCmd.snapHeader);

            // Handle resolution changed
            if (drawCmd.snapHeader->view.width != latestSize_.width ||
                drawCmd.snapHeader->view.height != latestSize_.width) {
                latestSize_ = { .width = drawCmd.snapHeader->view.width,
                    .height = drawCmd.snapHeader->view.height };
                renderer_->handleResize(latestSize_.width, latestSize_.height);
                surfaceProvider_.resetResized();
            }

            auto commandProcessingEnd = std::chrono::high_resolution_clock::now();

            // Begin frame
            if (auto frameCtx = presenter_->beginFrame()) {
                // Draw frame
                renderer_->drawFrame(
                    frameCtx->renderTarget, drawCmd.snapHeader, snapData, frameCtx->renderFinished);

                // Present
                presenter_->endFrame(*frameCtx);
            }

            // Signal dispatcher
            proxy_.setLastCompletedFrame(drawCmd.frameNumber);

            auto frameEnd = std::chrono::high_resolution_clock::now();
            std::chrono::duration<float, std::milli> commandProcessingTime =
                commandProcessingEnd - frameStart;
            std::chrono::duration<float, std::milli> frameTime = frameEnd - frameStart;
            std::chrono::duration<float, std::milli> renderTime = frameEnd - commandProcessingEnd;
            float lastFrameMs = frameTime.count();
            float commandProcessingTimeMs = commandProcessingTime.count();
            float renderTimeMs = renderTime.count();
            proxy_.getResponseQueue().push(
                RenderingServerInfoCommand{ .type = RenderingServerInfoCommandType::Frametime,
                    .frametime = lastFrameMs,
                    .rendertime = renderTimeMs,
                    .cmdtime = commandProcessingTimeMs });
        }

        // Finish the rendering before stopping
        renderer_->waitIdle();
    });
}

void hammock::app::RenderingServer::stop() { running_.store(false, std::memory_order_release); }

bool hammock::app::RenderingServer::isRunning() const { return running_.load(std::memory_order_relaxed); }

void hammock::app::RenderingServer::join() { thread_.join(); }
