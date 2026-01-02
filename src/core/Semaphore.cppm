module;

#include <stdexcept>
#include <vulkan/vulkan.hpp>

export module hammock.core.semaphore;

import hammock.core.device;

namespace hammock::core {
    export class Semaphore final {
    public:
        explicit Semaphore(Device &device);
        ~Semaphore();

        [[nodiscard]] vk::Semaphore getVulkanSemaphore() const;

    private:
        Device &device;
        vk::Semaphore semaphore;
    };
}