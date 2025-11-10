#pragma once
#include <vulkan/vulkan.h>

#include <expected>
#include <string>
#include <vector>

namespace Hammock {
    class CommandBuffer {
       public:
        CommandBuffer(VkCommandBuffer cmd) : commandBuffer(cmd) {}

        auto waitOnSemaphore(VkSemaphore semaphore, VkPipelineStageFlagBits2 stageFlagBits) -> void;

        auto signalSemaphore(VkSemaphore semaphore, VkPipelineStageFlagBits2 stageFlagBits) -> void;

        auto begin() -> std::expected<void, std::string>;

        auto submit(VkQueue queue) -> std::expected<void, std::string>;

        auto getCommandBuffer() -> VkCommandBuffer { return commandBuffer; }

       private:
        VkCommandBuffer commandBuffer;
        std::vector<VkSemaphoreSubmitInfo> waitSemaphoreSubmitInfos;
        std::vector<VkSemaphoreSubmitInfo> signalSemaphoreSubmitInfos;
        bool inProgress{false};
    };
}  // namespace Hammock