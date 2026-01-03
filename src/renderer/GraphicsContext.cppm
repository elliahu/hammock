module;

#include <optional>
#include <stdexcept>
#include <compare>
#include <vulkan/vulkan.hpp>

export module hammock.renderer.graphics_context;

import hammock.core.instance;
import hammock.core.device;
import hammock.core.resource_manager;
import hammock.core.descriptor;


namespace hammock::renderer {
    export enum class GraphicsContextMode {
        Headless,
        Windowed
    };

    export struct GraphicsContextDesc {
        GraphicsContextMode mode;
    };

    export class GraphicsContext {
    public:
        explicit GraphicsContext(const GraphicsContextDesc &desc);

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

        GraphicsContextDesc desc_;

        core::Instance instance_{};
        std::optional<core::Device> device_;

        vk::SurfaceKHR surface_ = nullptr;
        bool hasPresent_ = false;
    };
}
