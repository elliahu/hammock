module;
#include <VulkanSurfer/VulkanSurfer.h>
#include <cstdint>
#include <string>
#include <vulkan/vulkan.hpp>


export module hammock.engine.editor:window;

import hammock.core;
import hammock.core.base_surface_provider;


namespace hammock::engine {
    /// @class Window
    /// @brief Class represents OS window and handles user I/O
    export class Window final : public core::BaseSurfaceProvider {
    public:
        Window(const std::string &title, core::Instance &instance, const std::uint32_t width,
               const std::uint32_t height, const std::uint32_t x = 100u,
               const std::uint32_t y = 100u) : instance(instance) {

            window = Surfer::Window::createWindow("Hammock engine", width, height, static_cast<std::int32_t>(x),
                                                  static_cast<std::int32_t>(y));

            // Create surface
            VkSurfaceKHR cSurface;
            window->createSurface(instance.getInstance(), &cSurface);
            surface = vk::SurfaceKHR(cSurface);
        }

        ~Window() override {
            instance.getInstance().destroySurfaceKHR(surface, nullptr);
            Surfer::Window::destroyWindow(window);
        }

        [[nodiscard]] vk::SurfaceKHR getSurface() const override { return surface; }

        bool shouldClose() {
            return window->shouldClose();
        }

        void pollEvents() const { window->pollEvents(); }

        [[nodiscard]] vk::Extent2D getExtent() const override {
            std::uint32_t w, h;
            window->getWindowSize(w, h);
            return vk::Extent2D(w, h);
        };

        bool wasResized() const override { return resized; }

        void resetResized() override { resized = false; }

        Surfer::Window * getWindowPtr() const  {return window;}

    private:
        bool resized = false;
        core::Instance &instance;
        Surfer::Window *window = nullptr;
        vk::SurfaceKHR surface = nullptr;
    };
}
