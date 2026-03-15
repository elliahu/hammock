#pragma once
#include <VulkanSurfer/VulkanSurfer.h>
#include <cstdint>
#include <string>
#include <compare>
#include <vulkan/vulkan.hpp>


#include "core/surface_provider_iface.hpp"
#include "core/instance.hpp"


namespace hammock::app {
    /// @class Window
    /// @brief Class represents OS window and handles user I/O
    class Window final : public core::SurfaceProviderIface {
    public:
        Window(const std::string &title,const std::uint32_t width,
               const std::uint32_t height, const std::uint32_t x = 100u,
               const std::uint32_t y = 100u);

        ~Window() override;

        void createVulkanSurface(core::Instance& instance) override;
        void destroyVulkanSurface(core::Instance& instance) override;

        [[nodiscard]] vk::SurfaceKHR getSurface() const override { return surface_; }

        bool shouldClose() const override;

        void pollEvents() const override;

        [[nodiscard]] vk::Extent2D getExtent() const override;;

        bool wasResized() const override { return resized_; }

        void resetResized() override { resized_ = false; }

        Surfer::Window * getWindowPtr() const  {return window_;}

    private:
        bool resized_ = false;
        Surfer::Window *window_ = nullptr;
        vk::SurfaceKHR surface_ = nullptr;
    };
}
