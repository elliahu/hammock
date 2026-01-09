module;

#include <optional>
#include <stdexcept>
#include <compare>
#include <vulkan/vulkan.hpp>

export module hammock_renderer:graphics_context;

import hammock_core;

namespace hammock::renderer {


    /// @class GraphicsContext
    /// @brief Manages Vulkan instance, device, and surface
    export class GraphicsContext {
    public:
        GraphicsContext() = default;

        ~GraphicsContext();

        // Windowed extension

        void attachSurface(vk::SurfaceKHR surface);

        // Headless extension
        void createHeadlessDevice();

        bool supportsPresent() const;

        core::Instance &getInstance() { return instance_; }

        core::Device &getDevice() { return *device_; }

        vk::SurfaceKHR getSurface() const { return surface_; }

    private
    :
        void initManagers();

        core::Instance instance_{};
        std::optional<core::Device> device_;

        vk::SurfaceKHR surface_ = nullptr;
        bool hasPresent_ = false;
    };
}
