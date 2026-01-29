#include <memory>
#include <compare>
#include <vulkan/vulkan.hpp>

#include "runner.hpp"
#include "hammock_core.hpp"
#include "hammock_renderer.hpp"


hammock::engine::Runner::Runner(RunnerMode mode, renderer::GraphicsContext &context) : context_(context) {
    // Create window
    window_ = std::make_unique<Window>(
        "Hammock",
        context.getInstance(),
        1920u,
        1080u
    );

    std::unique_ptr<BasePresentationStrategy> strategy;
    // Decide headless mode
    if (mode == RunnerMode::Headless) {
        // TODO
    } else {
        // Attach surface
        context_.attachSurface(window_->getSurface());

        // Select strategy
        strategy = std::make_unique<SurfacePresentationStrategy>(
            context_.getDevice(),
            *window_, // Window implements BaseSurfaceProvider
            mode == RunnerMode::Editor
                ? SurfacePresentationStrategy::Mode::Editor
                : SurfacePresentationStrategy::Mode::Runtime
        );

        // Ui only in surface mode
        ui_ = std::make_unique<Ui>(window_->getWindowPtr(), &context_, core::SwapChain::MAX_FRAMES_IN_FLIGHT);
    }

    // Register resize callback
    strategy->onResolutionChanged([this](uint32_t width, uint32_t height) {
        // Handle resolution changes (update camera aspect ratio, etc.)
        // TODO
    });

    // Initialize the presentation engine
    presentationEngine_ = std::make_unique<PresentationEngine>(std::move(strategy));

    // Initialize renderer
    auto renderStrategy = std::make_unique<renderer::DeferredRenderingStrategy>(context_, core::SwapChain::MAX_FRAMES_IN_FLIGHT);
    renderer_ = std::make_unique<renderer::Renderer>(std::move(renderStrategy));
}

hammock::engine::Runner::~Runner() {
    // Wait for GPU to finish before destroying resources
    context_.getDevice().waitIdle();
}

void hammock::engine::Runner::launch() {
    loop();
}

void hammock::engine::Runner::loop() {
    while (!window_->shouldClose()) {
        window_->pollEvents();
        handleInput();

        float deltaTime = 0.016f; // TODO: actual timing
        update(deltaTime);

        if (auto frameCtx = presentationEngine_->beginFrame()) {
            renderer_->drawFrame(
               frameCtx->renderTarget,
               presentationEngine_->getFrameIndex(),
               frameCtx->renderFinished
           );

            // Render UI (editor only)
            if (auto* surfaceStrategy = presentationEngine_->getStrategyAs<SurfacePresentationStrategy>()) {
                surfaceStrategy->submitUI(*frameCtx,
                    [this](core::ResourceHandle target, std::uint32_t frameIndex, core::Semaphore & wait, core::Semaphore & signal) {
                        ui_->renderFrame(target, frameIndex, wait, signal);
                    }
                );
            }

            // Present
            presentationEngine_->endFrame(*frameCtx);
        }
    }

    // Cleanup
    context_.getDevice().waitIdle();
}

void hammock::engine::Runner::handleInput() {
    // TODO
}

void hammock::engine::Runner::update(float deltaTime) {
    // TODO
}


