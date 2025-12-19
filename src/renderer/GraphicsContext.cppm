module;

#include <optional>
#include <vulkan/vulkan.h>
#include <stdexcept>

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
        explicit GraphicsContext(const GraphicsContextDesc &desc)
            : desc(desc) {
        }

        ~GraphicsContext() {
            core::ResourceManager::dispose();
            core::DescriptorPool::dispose();
        }

        // Windowed extension

        void attachSurface(VkSurfaceKHR surface) {
            if (desc.mode != GraphicsContextMode::Windowed) {
                throw std::runtime_error("Cannot attach surface when not in Windowed context mode");
            }

            this->surface = surface;

            device.emplace(instance, surface);
            hasPresent = true;
            initManagers();
        }

        // Headless extension
        void createHeadlessDevice() {
            if (desc.mode != GraphicsContextMode::Headless) {
                throw std::runtime_error("Cannot create headless device when not in Headless context mode");
            }

            device.emplace(instance, VK_NULL_HANDLE);
            hasPresent = false;
            initManagers();
        }

        bool supportsPresent() const {
            return desc.mode == GraphicsContextMode::Windowed;
        }

        core::Instance &getInstance() {
            return instance;
        }

        core::Device &getDevice() {
            return *device;
        }

        VkSurfaceKHR getSurface() const {
            return surface;
        }

    private
    :
        void initManagers() {
            core::ResourceManager::initialize(*device);
            // @formatter:off
            core::DescriptorPool::initialize(*device, 10000, VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT, {
                {VK_DESCRIPTOR_TYPE_SAMPLER, 1000},
                {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000},
                {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000},
                {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000},
                {VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000},
                {VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000},
                {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000},
                {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000},
                {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000},
                {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000},
                {VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000},
            });
            // @formatter:on
        }

        GraphicsContextDesc desc;

        core::Instance instance{};
        std::optional<core::Device> device;

        VkSurfaceKHR surface = VK_NULL_HANDLE;
        bool hasPresent = false;
    };
}
