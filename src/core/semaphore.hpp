#pragma once
#include <cstdint>
#include <vulkan/vulkan.hpp>

#include "device.hpp"
#include "resource_manager.hpp"

namespace hammock::core {
    /// @class Semaphore
    /// @brief Wrapper around vulkan semaphore.
    /// Used for GPU to GPU sync
    class Semaphore final {
       public:
        /// Creates a binary semaphore
        explicit Semaphore(Device& device);
        /// Creates timeline semaphore with initial value
        Semaphore(Device& device, uint64_t initialValue);
        ~Semaphore();

        [[nodiscard]] vk::Semaphore getVulkanSemaphore() const;

       private:
        Device& device_;
        vk::Semaphore semaphore_;
    };

    struct TimelineSemaphorePoint {
        ResourceRef<Semaphore> semaphore;
        uint64_t value;
    };
}  // namespace hammock::core
