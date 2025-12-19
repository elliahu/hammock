module;
#include <vulkan/vulkan.h>
#include <memory>
#include <stdexcept>
#include <vector>

export module hammock.core.command_buffer;

import hammock.core.device;
import hammock.core.semaphore;


namespace hammock::core {
    export class CommandBuffer {
    public:
        CommandBuffer(Device &device, CommandQueueFamily queueFamily) : device(device), queueFamily(queueFamily) {
            try {
                VkCommandBufferAllocateInfo allocInfo{};
                allocInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
                allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
                allocInfo.commandBufferCount = 1;
                if (queueFamily == CommandQueueFamily::Graphics) {
                    allocInfo.commandPool = device.getGraphicsCommandPool();
                }
                if (queueFamily == CommandQueueFamily::Compute) {
                    allocInfo.commandPool = device.getComputeCommandPool();
                }
                if (queueFamily == CommandQueueFamily::Transfer) {
                    allocInfo.commandPool = device.getTransferCommandPool();
                }

                if (vkAllocateCommandBuffers(device.device(), &allocInfo, &commandBuffer) != VkResult::VK_SUCCESS) {
                    throw std::runtime_error("failed to allocate command buffer");
                }
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

        auto waitOnSemaphore(Semaphore& semaphore, VkPipelineStageFlagBits2 stageFlagBits) -> void;

        auto signalSemaphore(Semaphore& semaphore) -> void;

        /// @brief begin command buffer recording
        auto begin() -> void;

        /// @brief Call this function when you only need to END the command buffer
        /// To both end and submit call `submit()`
        auto end() -> void;

        /// @brief This function ENDS and SUBMITS the command buffer
        auto submit(VkFence fence = VK_NULL_HANDLE) -> void;

        auto getCommandBuffer() const -> VkCommandBuffer { return commandBuffer; }

    private:
        Device &device;
        CommandQueueFamily queueFamily;
        VkCommandBuffer commandBuffer;
        std::vector<VkSemaphoreSubmitInfo> waitSemaphoreSubmitInfos;
        std::vector<VkSemaphoreSubmitInfo> signalSemaphoreSubmitInfos;
        bool inProgress{false};
    };
} // namespace Hammock
