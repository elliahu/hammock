module;
#include <vulkan/vulkan.h>
#include <memory>
#include <stdexcept>
#include <vector>

export module hammock.core.command_buffer;

import hammock.core.device;


namespace hammock::core {
    /// FIXME TODO make this self destructure, now this is really bad
    export class CommandBuffer {
    public:
        CommandBuffer(Device &device, CommandQueueFamily queueFamily) : device(device), queueFamily(queueFamily) {
            try {
                auto commandBuffers = device.createVulkanCommandBuffers(queueFamily, 1);

                if (commandBuffers.empty()) {
                    throw std::runtime_error("Failed to create Vulkan command buffer.");
                }

                commandBuffer = commandBuffers[0];
            } catch (std::exception &e) {
                throw;
            }
        }

        ~CommandBuffer() {
            if (commandBuffer != VK_NULL_HANDLE) {
                if (queueFamily == CommandQueueFamily::Graphics) {
                    vkFreeCommandBuffers(device.device(), device.getGraphicsCommandPool(), 1, &commandBuffer);
                } else if (queueFamily == CommandQueueFamily::Compute) {
                    vkFreeCommandBuffers(device.device(), device.getComputeCommandPool(), 1, &commandBuffer);
                } else if (queueFamily == CommandQueueFamily::Transfer) {
                    vkFreeCommandBuffers(device.device(), device.getTransferCommandPool(), 1, &commandBuffer);
                }
            }
        }

        auto waitOnSemaphore(VkSemaphore semaphore, VkPipelineStageFlagBits2 stageFlagBits) -> void;

        auto signalSemaphore(VkSemaphore semaphore, VkPipelineStageFlagBits2 stageFlagBits) -> void;

        auto begin() -> void;

        auto submit(VkQueue queue) -> void;

        auto getCommandBuffer() -> VkCommandBuffer { return commandBuffer; }

    private:
        Device &device;
        CommandQueueFamily queueFamily;
        VkCommandBuffer commandBuffer;
        std::vector<VkSemaphoreSubmitInfo> waitSemaphoreSubmitInfos;
        std::vector<VkSemaphoreSubmitInfo> signalSemaphoreSubmitInfos;
        bool inProgress{false};
    };
} // namespace Hammock
