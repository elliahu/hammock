module;

#include <stdexcept>
#include <vulkan/vulkan.hpp>

module hammock.core.semaphore;

namespace hammock::core {
    Semaphore::Semaphore(Device &device) : device(device) {
        vk::SemaphoreCreateInfo semaphoreCreateInfo = {};
        if (auto result = device.device().createSemaphore(&semaphoreCreateInfo, nullptr, &semaphore);
            result != vk::Result::eSuccess) {
            throw std::runtime_error("failed to create semaphore");
            }
    }

    Semaphore::~Semaphore() {
        device.device().destroySemaphore(semaphore);
    }

    vk::Semaphore Semaphore::getVulkanSemaphore() const {
        return semaphore;
    }
}