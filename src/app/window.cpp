#include "window.hpp"

#include <VulkanSurfer/VulkanSurfer.h>

#include <compare>
#include <string>
#include <vulkan/vulkan.hpp>

hammock::app::Window::Window(const std::string& title, const uint32_t width, const uint32_t height,
    const uint32_t x, const uint32_t y) {
    window_ = Surfer::Window::createWindow(
        "Hammock engine", width, height, static_cast<int32_t>(x), static_cast<int32_t>(y));

    // Resize
    window_->registerResizeCallback([this](uint32_t width, uint32_t height) { resized_ = true; });
}

hammock::app::Window::~Window() { Surfer::Window::destroyWindow(window_); }

bool hammock::app::Window::shouldClose() const { return window_->shouldClose(); }

void hammock::app::Window::pollEvents() const { window_->pollEvents(); }

vk::Extent2D hammock::app::Window::getExtent() const {
    uint32_t w, h;
    window_->getWindowSize(w, h);
    return vk::Extent2D(w, h);
}

void hammock::app::Window::createVulkanSurface(core::Instance& instance) {
    // Create surface
    VkSurfaceKHR cSurface;
    window_->createSurface(instance.getInstance(), &cSurface);
    surface_ = vk::SurfaceKHR(cSurface);
}

void hammock::app::Window::destroyVulkanSurface(core::Instance& instance) {
    instance.getInstance().destroySurfaceKHR(surface_, nullptr);
}
