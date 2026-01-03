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
        explicit GraphicsContext(const GraphicsContextDesc &desc)
            : desc(desc) {
        }

        ~GraphicsContext() {
            core::ResourceManager::dispose();
            core::DescriptorPool::dispose();
        }

        // Windowed extension

        void attachSurface(vk::SurfaceKHR surface) {
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

            device.emplace(instance, nullptr);
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

        vk::SurfaceKHR getSurface() const {
            return surface;
        }

    private
    :
        void initManagers() {
            core::ResourceManager::initialize(*device);
            // @formatter:off
            core::DescriptorPool::initialize(*device, 10000, vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet, {
                {vk::DescriptorType::eSampler, 1000},
                {vk::DescriptorType::eCombinedImageSampler, 1000},
                {vk::DescriptorType::eSampledImage, 1000},
                {vk::DescriptorType::eStorageImage, 1000},
                {vk::DescriptorType::eUniformTexelBuffer, 1000},
                {vk::DescriptorType::eStorageTexelBuffer, 1000},
                {vk::DescriptorType::eUniformBuffer, 1000},
                {vk::DescriptorType::eStorageBuffer, 1000},
                {vk::DescriptorType::eUniformBufferDynamic, 1000},
                {vk::DescriptorType::eStorageBufferDynamic, 1000},
                {vk::DescriptorType::eInputAttachment, 1000},
            });
            // @formatter:on
        }

        GraphicsContextDesc desc;

        core::Instance instance{};
        std::optional<core::Device> device;

        vk::SurfaceKHR surface = nullptr;
        bool hasPresent = false;
    };
}
