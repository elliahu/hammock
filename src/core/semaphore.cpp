#include "semaphore.hpp"

#include <cstdint>
#include <stdexcept>
#include <vulkan/vulkan.hpp>

#include "vulkan/vulkan.hpp"

namespace hammock::core {
    Semaphore::Semaphore(Device& device) : device_(device) {
        vk::SemaphoreCreateInfo semaphoreCreateInfo = {};
        if (auto result = device.device().createSemaphore(&semaphoreCreateInfo, nullptr, &semaphore_);
            result != vk::Result::eSuccess) {
            throw std::runtime_error("failed to create semaphore");
        }
    }

    Semaphore::Semaphore(Device& device, uint64_t initialValue) : device_(device) {
        vk::SemaphoreTypeCreateInfo timelineInfo{
            .semaphoreType = vk::SemaphoreType::eTimeline,
            .initialValue = initialValue,
        };

        vk::SemaphoreCreateInfo semaphoreCreateInfo = {.pNext = &timelineInfo};
        if (auto result = device.device().createSemaphore(&semaphoreCreateInfo, nullptr, &semaphore_);
            result != vk::Result::eSuccess) {
            throw std::runtime_error("failed to create semaphore");
        }
    }

    Semaphore::~Semaphore() { device_.device().destroySemaphore(semaphore_); }

    vk::Semaphore Semaphore::getVulkanSemaphore() const { return semaphore_; }
}  // namespace hammock::core
