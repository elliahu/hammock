#include <optional>
#include <stdexcept>
#include <compare>
#include <vulkan/vulkan.hpp>

#include "graphics_context.hpp"
#include "hammock_core.hpp"

hammock::renderer::GraphicsContext::~GraphicsContext() {
    core::ResourceManager::dispose();
    core::DescriptorPool::dispose();
}

void hammock::renderer::GraphicsContext::attachSurface(vk::SurfaceKHR surface) {
    if (device_.has_value()) {
        throw std::runtime_error("cannot attach surface, already in headless mode");
    }

    this->surface_ = surface;

    device_.emplace(instance_, surface);
    hasPresent_ = true;
    initManagers();
}

void hammock::renderer::GraphicsContext::createHeadlessDevice() {
    if (device_.has_value()) {
        throw std::runtime_error("cannot create headless device, surface already attached");
    }

    device_.emplace(instance_, nullptr);
    hasPresent_ = false;
    initManagers();
}

bool hammock::renderer::GraphicsContext::supportsPresent() const {
    return device_.has_value() && hasPresent_;
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
