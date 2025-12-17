module;

#include <VulkanSurfer/VulkanSurfer.h>
#include <cstdint>
#include <string>


export module hammock.engine.editor:window;

import hammock.core.instance;

namespace hammock::engine {
    /// @class Window
    /// @brief Class represents OS window and handles user I/O
    export class Window final {
    public:
        Window(const std::string &title, core::Instance &instance, const std::uint32_t width, const std::uint32_t height, const std::uint32_t x = 100u, const std::uint32_t y = 100u) :instance(instance) {
            // Create window
            window = Surfer::Window::createWindow(title, width, height, x, y);

            // Create surface
            window->createSurface(instance.getInstance(), &surface);
        }

        ~Window() {
            vkDestroySurfaceKHR(instance.getInstance() , surface, nullptr);
            Surfer::Window::destroyWindow(window);
        }

        VkSurfaceKHR getSurface() const { return surface; }

        bool shouldClose() const { return window->shouldClose(); }
        void pollEvents() const { window->pollEvents(); }
    private:
        core::Instance &instance;
        Surfer::Window *window = nullptr;
        VkSurfaceKHR surface = VK_NULL_HANDLE;
    };
}