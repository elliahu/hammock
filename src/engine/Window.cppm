module;

#include <vulkan/vulkan.h>
#include <cstdint>
#include <string>


export module hammock.engine.editor:window;

import hammock.core.instance;
import hammock.core.base_surface_provider;

namespace hammock::engine {
    /// @class Window
    /// @brief Class represents OS window and handles user I/O
    export class Window final : public core::BaseSurfaceProvider {
    public:
        Window(const std::string &title, core::Instance &instance, const std::uint32_t width,
               const std::uint32_t height, const std::uint32_t x = 100u,
               const std::uint32_t y = 100u) : instance(instance) {
            // Create window
            window = rgfw::createWindow(
                title.c_str(),
                static_cast<std::int32_t>(x),
                static_cast<std::int32_t>(y),
                static_cast<std::int32_t>(width),
                static_cast<std::int32_t>(height),
                (1 << (6))
            );

            rgfw::setWindowUserPtr(window, this);

            // Create surface
            rgfw::createVulkanSurface(window, instance.getInstance(), &surface);
        }

        ~Window() override {
            vkDestroySurfaceKHR(instance.getInstance(), surface, nullptr);
            rgfw::closeWindow(window);
        }

        [[nodiscard]] VkSurfaceKHR getSurface() const override { return surface; }

        bool shouldClose() {
            rgfw::checkEvent(window, &event);
            return event.type == 14; // QUIT
        }

        void pollEvents() const { rgfw::pollEvents(); }

        [[nodiscard]] VkExtent2D getExtent() const override {
            std::int32_t w, h;
            rgfw::getWindowSize(window, &w, &h);
            return VkExtent2D(static_cast<std::uint32_t>(w), static_cast<std::uint32_t>(h));
        };

        bool wasResized() const override {return resized;}

        void resetResized() override {resized = false;}

    private:
        bool resized = false;
        core::Instance &instance;
        rgfw::RGFW_window * window = nullptr;
        VkSurfaceKHR surface = VK_NULL_HANDLE;
        rgfw::RGFW_event event;
    };
}
