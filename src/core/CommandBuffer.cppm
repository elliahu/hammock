module;
#include <memory>
#include <stdexcept>
#include <vector>
#include <vulkan/vulkan.hpp>

export module hammock.core.command_buffer;

import hammock.core.device;
import hammock.core.semaphore;


namespace hammock::core {
    export class CommandBuffer {
    public:
        CommandBuffer(Device &device, CommandQueueFamily queueFamily) : device(device), queueFamily(queueFamily) {
            try {
                vk::CommandBufferAllocateInfo allocInfo{};
                allocInfo.level = vk::CommandBufferLevel::ePrimary;
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

                if (device.device().allocateCommandBuffers(&allocInfo, &commandBuffer) != vk::Result::eSuccess) {
                    throw std::runtime_error("failed to allocate command buffer");
                }
            } catch (std::exception &e) {
                throw;
            }
        }

        ~CommandBuffer() {
            if (commandBuffer) {
                if (queueFamily == CommandQueueFamily::Graphics) {
                    device.device().freeCommandBuffers( device.getGraphicsCommandPool(), 1, &commandBuffer);
                } else if (queueFamily == CommandQueueFamily::Compute) {
                    device.device().freeCommandBuffers(device.getComputeCommandPool(), 1, &commandBuffer);
                } else if (queueFamily == CommandQueueFamily::Transfer) {
                    device.device().freeCommandBuffers(device.getTransferCommandPool(), 1, &commandBuffer);
                }
            }
        }

        auto waitOnSemaphore(Semaphore& semaphore, vk::PipelineStageFlagBits2 stageFlagBits) -> void;

        auto signalSemaphore(Semaphore& semaphore) -> void;

        /// @brief begin command buffer recording
        auto begin() -> void;

        /// @brief Call this function when you only need to END the command buffer
        /// To both end and submit call `submit()`
        auto end() -> void;

        /// @brief This function ENDS and SUBMITS the command buffer
        auto submit(vk::Fence fence = vk::Fence{}) -> void;

        auto getCommandBuffer() const -> vk::CommandBuffer { return commandBuffer; }

    private:
        Device &device;
        CommandQueueFamily queueFamily;
        vk::CommandBuffer commandBuffer;
        std::vector<vk::SemaphoreSubmitInfo> waitSemaphoreSubmitInfos;
        std::vector<vk::SemaphoreSubmitInfo> signalSemaphoreSubmitInfos;
        bool inProgress{false};
    };
} // namespace Hammock
