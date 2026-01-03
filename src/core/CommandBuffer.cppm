module;
#include <memory>
#include <stdexcept>
#include <vector>
#include <compare>
#include <vulkan/vulkan.hpp>

export module hammock.core.command_buffer;

import hammock.core.device;
import hammock.core.semaphore;


namespace hammock::core {
    /// @class CommandBuffer
    /// Wrapper class around vulkan command buffer
    export class CommandBuffer {
    public:
        CommandBuffer(Device &device, CommandQueueFamily queueFamily);

        ~CommandBuffer();

        /// @brief Set a semaphore that needs to be signaled before the command buffer starts
        /// Execution will wait on all wait semaphores in their corresponding stages
        auto waitOnSemaphore(Semaphore& semaphore, vk::PipelineStageFlagBits2 stageFlagBits) -> void;

        /// @brief Set a semaphore that will be signaled after the command buffer finishes
        auto signalSemaphore(Semaphore& semaphore) -> void;

        /// @brief begin command buffer recording
        auto begin() -> void;

        /// @brief Call this function when you only need to END the command buffer
        /// @note To both end and submit call `submit()`
        auto end() -> void;

        /// @brief This function ENDS and SUBMITS the command buffer
        auto submit(vk::Fence fence = vk::Fence{}) -> void;

        /// @brief Get vulkan command buffer handle
        auto getCommandBuffer() const -> vk::CommandBuffer { return commandBuffer_; }

    private:
        Device &device_;
        CommandQueueFamily queueFamily_;
        vk::CommandBuffer commandBuffer_;
        std::vector<vk::SemaphoreSubmitInfo> waitSemaphoreSubmitInfos_;
        std::vector<vk::SemaphoreSubmitInfo> signalSemaphoreSubmitInfos_;
        bool inProgress_{false};
    };
} // namespace Hammock
