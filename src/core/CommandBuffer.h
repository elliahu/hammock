#pragma once
#include <vulkan/vulkan.h>

#include <exception>
#include <memory>
#include <string>
#include <vector>

#include "Device.h"

namespace hammock::core {
    class CommandBuffer {
       public:
        CommandBuffer(VkCommandBuffer cmd) : commandBuffer(cmd) {}

        template <CommandQueueFamily Queue>
        static auto create(Device& device) -> std::unique_ptr<CommandBuffer> {
            try {
                auto commandBuffers = device.createVulkanCommandBuffers<Queue>(1);

                if (commandBuffers.empty()) {
                    throw std::runtime_error("Failed to create Vulkan command buffer.");
                }

                return std::make_unique<CommandBuffer>(commandBuffers[0]);
            } catch (std::exception e) {
                throw;
            }
        }

        auto waitOnSemaphore(VkSemaphore semaphore, VkPipelineStageFlagBits2 stageFlagBits) -> void;

        auto signalSemaphore(VkSemaphore semaphore, VkPipelineStageFlagBits2 stageFlagBits) -> void;

        auto begin() -> void;

        auto submit(VkQueue queue) -> void;

        auto getCommandBuffer() -> VkCommandBuffer { return commandBuffer; }

       private:
        VkCommandBuffer commandBuffer;
        std::vector<VkSemaphoreSubmitInfo> waitSemaphoreSubmitInfos;
        std::vector<VkSemaphoreSubmitInfo> signalSemaphoreSubmitInfos;
        bool inProgress{false};
    };
}  // namespace Hammock