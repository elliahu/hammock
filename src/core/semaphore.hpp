#pragma once
#include <stdexcept>
#include <compare>
#include <vulkan/vulkan.hpp>

#include "device.hpp"

namespace hammock::core {
    /// @class Semaphore
    /// @brief Wrapper around vulkan semaphore.
    /// Used for GPU to GPU sync
    class Semaphore final {
    public:
        explicit Semaphore(Device &device);
        ~Semaphore();

        [[nodiscard]] vk::Semaphore getVulkanSemaphore() const;

    private:
        Device &device_;
        vk::Semaphore semaphore_;
    };
}