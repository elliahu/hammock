#include "window.hpp"

#include <VulkanSurfer/VulkanSurfer.h>

#include <compare>
#include <string>
#include <vulkan/vulkan.hpp>


hammock::app::Window::Window(const std::string& title, core::Instance& instance, const uint32_t width,
    const uint32_t height, const uint32_t x, const uint32_t y)
    : instance_(instance) {
    window_ = Surfer::Window::createWindow(
        "Hammock engine", width, height, static_cast<int32_t>(x), static_cast<int32_t>(y));

    // Resize
    window_->registerResizeCallback([this](uint32_t width, uint32_t height) {
        resized_ = true;
    });

    // Create surface
    VkSurfaceKHR cSurface;
    window_->createSurface(instance.getInstance(), &cSurface);
    surface_ = vk::SurfaceKHR(cSurface);
}

hammock::app::Window::~Window() {
    instance_.getInstance().destroySurfaceKHR(surface_, nullptr);
    Surfer::Window::destroyWindow(window_);
}

bool hammock::app::Window::shouldClose() const { return window_->shouldClose(); }

void hammock::app::Window::pollEvents() const { window_->pollEvents(); }

vk::Extent2D hammock::app::Window::getExtent() const {
    uint32_t w, h;
    window_->getWindowSize(w, h);
    return vk::Extent2D(w, h);
}
