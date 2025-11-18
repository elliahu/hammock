#pragma once
#include <vulkan/vulkan.h>

#include <string>
#include <vector>

namespace Hammock {
    class CommandBuffer {
       public:
        CommandBuffer(VkCommandBuffer cmd) : commandBuffer(cmd) {}

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