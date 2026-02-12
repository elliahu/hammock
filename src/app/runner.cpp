#include "runner.hpp"

#include <compare>
#include <memory>
#include <vulkan/vulkan.hpp>

#include "core/vulkan_context.hpp"

hammock::app::Runner::Runner(RunnerMode mode, core::VulkanContext& context) : vulkanContext_(context) {
    // Create window
    window_ = std::make_unique<Window>("Hammock", *context.instance, 1920u, 1080u);

    std::unique_ptr<BasePresentationStrategy> strategy;

    // Attach surface
    vulkanContext_.initialize(*window_);

    // Select strategy
    strategy = std::make_unique<SurfacePresentationStrategy>(vulkanContext_,
        *window_,  // Window implements BaseSurfaceProvider
        mode == RunnerMode::Tooling ? SurfacePresentationStrategy::Mode::Tooling
                                   : SurfacePresentationStrategy::Mode::Game);

    // Ui only in surface mode
    ui_ = std::make_unique<Ui>(window_->getWindowPtr(), vulkanContext_);

    // Register resize callback
    strategy->onResolutionChanged([this](uint32_t width, uint32_t height) {
        // Handle resolution changes (update camera aspect ratio, etc.)
        // TODO
    });

    // Initialize the presentation engine
    presentationEngine_ = std::make_unique<PresentationEngine>(std::move(strategy));

    // Initialize renderer
    renderer_ = std::make_unique<renderer::Renderer>(vulkanContext_);
}

hammock::app::Runner::~Runner() {
    // Wait for GPU to finish before destroying resources
    vulkanContext_.device->waitIdle();
}

void hammock::app::Runner::launch() { loop(); }

void hammock::app::Runner::loop() {
    while (!window_->shouldClose()) {
        window_->pollEvents();
        handleInput();

        float deltaTime = 0.016f;  // TODO: actual timing
        update(deltaTime);

        if (auto frameCtx = presentationEngine_->beginFrame()) {
            renderer_->drawFrame(
                frameCtx->renderTarget, presentationEngine_->getFrameIndex(), frameCtx->renderFinished);

            // Render UI (editor only)
            if (auto* surfaceStrategy = presentationEngine_->getStrategyAs<SurfacePresentationStrategy>()) {
                surfaceStrategy->submitUI(*frameCtx,
                    [this](core::ResourceHandle target,
                        std::uint32_t frameIndex,
                        core::Semaphore& wait,
                        core::Semaphore& signal) { ui_->renderFrame(target, frameIndex, wait, signal); });
            }

            // Present
            presentationEngine_->endFrame(*frameCtx);
        }
    }
}

void hammock::app::Runner::handleInput() {
    // TODO
}

void hammock::app::Runner::update(float deltaTime) {
    // TODO
}
