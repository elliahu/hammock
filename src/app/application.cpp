#include <memory>
#include <compare>
#include <vulkan/vulkan.hpp>

#include "application.hpp"

hammock::app::Application::Application(hammock::app::ExecutionMode mode) {
    // Create editor
    runner_ = std::make_unique<Runner>();
}

void hammock::app::Application::launch() const {
    runner_->launch();
}
