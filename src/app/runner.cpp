#include "runner.hpp"

#include <memory>
#include <thread>
#include <vulkan/vulkan.hpp>

#include "application_types.hpp"
#include "core/vulkan_context.hpp"
#include "render_backend.hpp"
#include "render_frontend.hpp"
#include "render_proxy.hpp"
#include "thread_pool.hpp"
#include "ui.hpp"

hammock::app::Runner::Runner(ExecutionMode mode, core::VulkanContext& context) : vulkanContext_(context) {
    // Create window
    window_ = std::make_unique<Window>("Hammock", *context.instance, 1920u, 1080u);

    // Attach surface
    vulkanContext_.initialize(*window_);

    /// Set the threads
    renderProxy_ = std::make_unique<renderer::RenderProxy>();
    renderFrontend_ = std::make_unique<renderer::RenderFrontend>(*renderProxy_, *window_);
    renderBackend_ = std::make_unique<renderer::RenderBackend>(*renderProxy_, vulkanContext_, *window_);
}

hammock::app::Runner::~Runner() {
    // Wait for GPU to finish before destroying resources
    vulkanContext_.device->waitIdle();
}

void hammock::app::Runner::launch() {
    core::Logger::debug("Main thread %d", std::this_thread::get_id());
    // Backend must start before frontend as the frontend runs on this (main) thread
    renderBackend_->start();
    // Start the frontend
    renderFrontend_->start();

    // Join the threads
    renderBackend_->join();
}
