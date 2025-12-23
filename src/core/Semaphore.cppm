module;

#include <stdexcept>

export module hammock.core.semaphore;

import hammock.core.device;
import vulkan_hpp;

namespace hammock::core {
    export class Semaphore final {
    public:
        explicit Semaphore(Device &device) : device(device) {
            vk::SemaphoreCreateInfo semaphoreCreateInfo = {};
            if (auto result = device.device().createSemaphore(&semaphoreCreateInfo, nullptr, &semaphore);
                result != vk::Result::eSuccess) {
                throw std::runtime_error("failed to create semaphore");
            }
        }

        ~Semaphore() {
            device.device().destroySemaphore(semaphore);
        }

        [[nodiscard]] vk::Semaphore getVulkanSemaphore() const {
            return semaphore;
        }

    private:
        Device &device;
        vk::Semaphore semaphore;
    };
}
