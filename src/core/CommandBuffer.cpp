module;
#include <vulkan/vulkan.h>
#include <stdexcept>

module hammock.core.command_buffer;

import hammock.core.semaphore;

auto hammock::core::CommandBuffer::begin() -> void {
    if (inProgress) {
        throw std::runtime_error("Cannot begin commandbuffer that is already in progress");
    }

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
        throw std::runtime_error("Failed to begin recording a command buffer");
    }

    inProgress = true;
}

auto hammock::core::CommandBuffer::end() -> void {
    if (!inProgress) {
        throw std::runtime_error("Cannot end command buffer that has not started yet (forgot to call begin()?)");
    }

    if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
        throw std::runtime_error("Failed to end command buffer");
    }

    inProgress = false;
}

auto hammock::core::CommandBuffer::submit(VkFence fence) -> void {
    if (!inProgress) {
        throw std::runtime_error("Cannot submit command buffer that has not started yet (forgot to call begin()?)");
    }

    VkCommandBufferSubmitInfo cmdBufInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
        .commandBuffer = commandBuffer,
    };

    VkSubmitInfo2 submitInfo = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
        .waitSemaphoreInfoCount = static_cast<uint32_t>(waitSemaphoreSubmitInfos.size()),
        .pWaitSemaphoreInfos = waitSemaphoreSubmitInfos.data(),
        .commandBufferInfoCount = 1,
        .pCommandBufferInfos = &cmdBufInfo,
        .signalSemaphoreInfoCount = static_cast<uint32_t>(signalSemaphoreSubmitInfos.size()),
        .pSignalSemaphoreInfos = signalSemaphoreSubmitInfos.data(),
    };

    if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
        throw std::runtime_error("Failed to end command buffer");
    }

    // find queue
    VkQueue queue = VK_NULL_HANDLE;
    switch (queueFamily) {
        case CommandQueueFamily::Graphics:
            queue = device.graphicsQueue();
            break;
        case CommandQueueFamily::Compute:
            queue = device.computeQueue();
            break;
        case CommandQueueFamily::Transfer:
            queue = device.transferQueue();
            break;
        default:
            throw std::runtime_error("Unknown queue family");
    }

    if (vkQueueSubmit2(queue, 1, &submitInfo, fence) != VK_SUCCESS) {
        throw std::runtime_error("Failed to submit command buffer");
    }

    waitSemaphoreSubmitInfos.clear();
    signalSemaphoreSubmitInfos.clear();

    inProgress = false;
}

auto hammock::core::CommandBuffer::waitOnSemaphore(Semaphore &semaphore,
                                                   VkPipelineStageFlagBits2 stageFlagBits) -> void {
    waitSemaphoreSubmitInfos.push_back({
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
        .semaphore = semaphore.getVulkanSemaphore(),
        .stageMask = stageFlagBits,
    });
}

auto hammock::core::CommandBuffer::signalSemaphore(Semaphore &semaphore) -> void {
    signalSemaphoreSubmitInfos.push_back({
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
        .semaphore = semaphore.getVulkanSemaphore(),
        .stageMask = 0,
    });
}
