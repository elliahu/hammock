#include <memory>
#include <compare>
#include <vulkan/vulkan.hpp>

#include "application.hpp"

hammock::engine::Application::Application(RunnerMode mode) {
    // Create graphics context
    context_ = std::make_unique<renderer::GraphicsContext>();

    // Create editor
    runner_ = std::make_unique<Runner>(mode, *context_);
}

void hammock::engine::Application::launch() const {
    runner_->launch();
}
