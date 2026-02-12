#include "vulkan_context.hpp"


hammock::core::VulkanContext::VulkanContext() { instance = std::make_unique<core::Instance>(); }

void hammock::core::VulkanContext::initialize(core::BaseSurfaceProvider& surfaceProvider) {
    device = std::make_unique<core::Device>(*instance, surfaceProvider.getSurface());
    resourceManager = std::make_unique<core::ResourceManager>(*device);
    descriptorPool = std::make_unique<core::DescriptorPool>(*device,
        10000,
        vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
        std::vector<vk::DescriptorPoolSize>{
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
}
