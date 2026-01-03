module;
#include <string>
#include <compare>
#include <vulkan/vulkan.hpp>
#include <VulkanSurfer/VulkanSurfer.h>

module hammock.engine.window;

hammock::engine::Window::Window(const std::string &title, core::Instance &instance, const uint32_t width,
    const uint32_t height, const uint32_t x, const uint32_t y): instance_(instance) {

    window_ = Surfer::Window::createWindow("Hammock engine", width, height, static_cast<int32_t>(x),
                                          static_cast<int32_t>(y));

    // Create surface
    VkSurfaceKHR cSurface;
    window_->createSurface(instance.getInstance(), &cSurface);
    surface_ = vk::SurfaceKHR(cSurface);
}

hammock::engine::Window::~Window() {
    instance_.getInstance().destroySurfaceKHR(surface_, nullptr);
    Surfer::Window::destroyWindow(window_);
}

bool hammock::engine::Window::shouldClose() const {
    return window_->shouldClose();
}

void hammock::engine::Window::pollEvents() const { window_->pollEvents(); }

vk::Extent2D hammock::engine::Window::getExtent() const {
    uint32_t w, h;
    window_->getWindowSize(w, h);
    return vk::Extent2D(w, h);
}
