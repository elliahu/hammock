module;

#include <stdexcept>
#include <compare>
#include <vulkan/vulkan.hpp>

export module hammock_core.semaphore;

import hammock_core.device;

namespace hammock::core {
    /// @class Semaphore
    /// @brief Wrapper around vulkan semaphore.
    /// Used for GPU to GPU sync
    export class Semaphore final {
    public:
        explicit Semaphore(Device &device);
        ~Semaphore();

        [[nodiscard]] vk::Semaphore getVulkanSemaphore() const;

    private:
        Device &device_;
        vk::Semaphore semaphore_;
    };
}