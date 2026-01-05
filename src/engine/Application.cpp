module;

#include <memory>
#include <compare>
#include <vulkan/vulkan.hpp>

module hammock_engine;

hammock::engine::Application::Application(RunnerMode mode) {
    // Create graphics context
    context_ = std::make_unique<renderer::GraphicsContext>(renderer::GraphicsContextDesc{
        .mode = mode == RunnerMode::Headless
                    ? renderer::GraphicsContextMode::Headless
                    : renderer::GraphicsContextMode::Windowed,
    });

    // Create editor
    runner_ = std::make_unique<Runner>(mode, *context_);
}

void hammock::engine::Application::launch() const {
    runner_->launch();
}
