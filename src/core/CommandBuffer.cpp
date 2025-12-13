module;
#include <vulkan/vulkan.h>
#include <stdexcept>

module hammock.core.command_buffer;



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

auto hammock::core::CommandBuffer::submit(VkQueue queue) -> void{
    if (!inProgress) {
        throw std::runtime_error("Cannot submit commandbuffer that has not started yet (forgot to call begin()?)");
    }

    VkCommandBufferSubmitInfo cmdBufInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
        .commandBuffer =commandBuffer,
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

    if(vkEndCommandBuffer(commandBuffer) != VK_SUCCESS){
        throw std::runtime_error("Failed to end command buffer");
    }

    if(vkQueueSubmit2(queue, 1, &submitInfo, VK_NULL_HANDLE) != VK_SUCCESS){
        throw std::runtime_error("Failed to submit command buffer");
    }
}

auto hammock::core::CommandBuffer::waitOnSemaphore(VkSemaphore semaphore, VkPipelineStageFlagBits2 stageFlagBits) -> void {
    waitSemaphoreSubmitInfos.push_back({
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
        .semaphore = semaphore,
        .stageMask = stageFlagBits,
    });
}

auto hammock::core::CommandBuffer::signalSemaphore(VkSemaphore semaphore, VkPipelineStageFlagBits2 stageFlagBits) -> void {
    signalSemaphoreSubmitInfos.push_back({
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
        .semaphore = semaphore,
        .stageMask = stageFlagBits,
    });
}