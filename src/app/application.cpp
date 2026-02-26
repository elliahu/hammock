#include <memory>
#include <compare>
#include <vulkan/vulkan.hpp>
#include "vulkan_context.hpp"

#include "application.hpp"

hammock::app::Application::Application(hammock::app::ExecutionMode mode) {
    // Create graphics context
    context_ = std::make_unique<core::VulkanContext>();

    // Create editor
    runner_ = std::make_unique<Runner>(mode, *context_);
}

void hammock::app::Application::launch() const {
    runner_->launch();
}
