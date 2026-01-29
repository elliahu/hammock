#include <stdexcept>
#include <compare>
#include <vulkan/vulkan.hpp>

#include "semaphore.hpp"

namespace hammock::core {
    Semaphore::Semaphore(Device &device) : device_(device) {
        vk::SemaphoreCreateInfo semaphoreCreateInfo = {};
        if (auto result = device.device().createSemaphore(&semaphoreCreateInfo, nullptr, &semaphore_);
            result != vk::Result::eSuccess) {
            throw std::runtime_error("failed to create semaphore");
            }
    }

    Semaphore::~Semaphore() {
        device_.device().destroySemaphore(semaphore_);
    }

    vk::Semaphore Semaphore::getVulkanSemaphore() const {
        return semaphore_;
    }
}