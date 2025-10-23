#pragma once
#include <vulkan/vulkan.h>

#include <expected>
#include <string>
#include <vector>

namespace Hammock {
    class CommandBuffer {
       public:
        CommandBuffer(VkCommandBuffer cmd) : commandBuffer(cmd) {}

        // Not copyable or movable
        CommandBuffer(const CommandBuffer&) = delete;
        CommandBuffer& operator=(const CommandBuffer&) = delete;
        CommandBuffer(CommandBuffer&&) = delete;
        CommandBuffer& operator=(CommandBuffer&&) = delete;

        auto waitOnSemaphore(VkSemaphore semaphore, VkPipelineStageFlagBits2 stageFlagBits) -> void;

        auto signalSemaphore(VkSemaphore semaphore, VkPipelineStageFlagBits2 stageFlagBits) -> void;

        auto begin() -> std::expected<void, std::string>;

        auto submit(VkQueue queue) -> std::expected<void, std::string>;

       private:
        VkCommandBuffer commandBuffer;
        std::vector<VkSemaphoreSubmitInfo> waitSemaphoreSubmitInfos;
        std::vector<VkSemaphoreSubmitInfo> signalSemaphoreSubmitInfos;
        bool inProgress{false};
    };
}  // namespace Hammock