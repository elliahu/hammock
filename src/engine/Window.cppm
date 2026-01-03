module;
#include <VulkanSurfer/VulkanSurfer.h>
#include <cstdint>
#include <string>
#include <compare>
#include <vulkan/vulkan.hpp>


export module hammock.engine.window;

import hammock.core;
import hammock.core.base_surface_provider;


namespace hammock::engine {
    /// @class Window
    /// @brief Class represents OS window and handles user I/O
    export class Window final : public core::BaseSurfaceProvider {
    public:
        Window(const std::string &title, core::Instance &instance, const std::uint32_t width,
               const std::uint32_t height, const std::uint32_t x = 100u,
               const std::uint32_t y = 100u);

        ~Window() override;

        [[nodiscard]] vk::SurfaceKHR getSurface() const override { return surface_; }

        bool shouldClose() const;

        void pollEvents() const;

        [[nodiscard]] vk::Extent2D getExtent() const override;;

        bool wasResized() const override { return resized_; }

        void resetResized() override { resized_ = false; }

        Surfer::Window * getWindowPtr() const  {return window_;}

    private:
        bool resized_ = false;
        core::Instance &instance_;
        Surfer::Window *window_ = nullptr;
        vk::SurfaceKHR surface_ = nullptr;
    };
}
