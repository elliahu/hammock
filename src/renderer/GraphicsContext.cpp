module;

#include <optional>
#include <stdexcept>
#include <compare>
#include <vulkan/vulkan.hpp>

module hammock_renderer.graphics_context;

import hammock_core.instance;
import hammock_core.device;
import hammock_core.resource_manager;
import hammock_core.descriptor;


hammock::renderer::GraphicsContext::GraphicsContext(const GraphicsContextDesc &desc): desc_(desc) {
}

hammock::renderer::GraphicsContext::~GraphicsContext() {
    core::ResourceManager::dispose();
    core::DescriptorPool::dispose();
}

void hammock::renderer::GraphicsContext::attachSurface(vk::SurfaceKHR surface) {
    if (desc_.mode != GraphicsContextMode::Windowed) {
        throw std::runtime_error("Cannot attach surface when not in Windowed context mode");
    }

    this->surface_ = surface;

    device_.emplace(instance_, surface);
    hasPresent_ = true;
    initManagers();
}

void hammock::renderer::GraphicsContext::createHeadlessDevice() {
    if (desc_.mode != GraphicsContextMode::Headless) {
        throw std::runtime_error("Cannot create headless device when not in Headless context mode");
    }

    device_.emplace(instance_, nullptr);
    hasPresent_ = false;
    initManagers();
}

bool hammock::renderer::GraphicsContext::supportsPresent() const {
    return desc_.mode == GraphicsContextMode::Windowed;
}

void hammock::renderer::GraphicsContext::initManagers() {
    core::ResourceManager::initialize(*device_);
    // @formatter:off
    core::DescriptorPool::initialize(*device_, 10000, vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet, {
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
