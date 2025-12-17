module;

#include <optional>
#include <vulkan/vulkan.h>
#include <stdexcept>

export module hammock.renderer.graphics_context;

import hammock.core.instance;
import hammock.core.device;

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
        explicit GraphicsContext(const GraphicsContextDesc &desc)
            : desc(desc) {
        }

        // Windowed extension
        void attachSurface(VkSurfaceKHR surface) {
            if (desc.mode != GraphicsContextMode::Windowed) {
                throw std::runtime_error("Cannot attach surface when not in Windowed context mode");
            }

            this->surface = surface;

            device.emplace(instance, surface);
            hasPresent = true;
        }

        // Headless extension
        void createHeadlessDevice() {
            if (desc.mode != GraphicsContextMode::Headless) {
                throw std::runtime_error("Cannot create headless device when not in Headless context mode");
            }

            device.emplace(instance, VK_NULL_HANDLE);
            hasPresent = false;
        }

        bool supportsPresent() const { return desc.mode == GraphicsContextMode::Windowed; }

        core::Instance &getInstance() { return instance; }
        core::Device &getDevice() { return *device; }
        VkSurfaceKHR getSurface() const { return surface; }

    private:
        GraphicsContextDesc desc;

        core::Instance instance{};
        std::optional<core::Device> device;

        VkSurfaceKHR surface = VK_NULL_HANDLE;
        bool hasPresent = false;
    };
}
