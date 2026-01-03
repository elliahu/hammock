module;
#include <stdexcept>
#include <vulkan/vulkan.hpp>

module hammock.core.command_buffer;

import hammock.core.semaphore;

auto hammock::core::CommandBuffer::begin() -> void {
    if (inProgress) {
        throw std::runtime_error("Cannot begin commandbuffer that is already in progress");
    }

    vk::CommandBufferBeginInfo beginInfo{};

    if (commandBuffer.begin(&beginInfo) != vk::Result::eSuccess) {
        throw std::runtime_error("Failed to begin recording a command buffer");
    }

    inProgress = true;
}

auto hammock::core::CommandBuffer::end() -> void {
    if (!inProgress) {
        throw std::runtime_error("Cannot end command buffer that has not started yet (forgot to call begin()?)");
    }

    commandBuffer.end();

    inProgress = false;
}

auto hammock::core::CommandBuffer::submit(vk::Fence fence) -> void {
    if (!inProgress) {
        throw std::runtime_error("Cannot submit command buffer that has not started yet (forgot to call begin()?)");
    }

    vk::CommandBufferSubmitInfo cmdBufInfo = {
        .commandBuffer = commandBuffer,
    };

    vk::SubmitInfo2 submitInfo = {
        .waitSemaphoreInfoCount = static_cast<uint32_t>(waitSemaphoreSubmitInfos.size()),
        .pWaitSemaphoreInfos = waitSemaphoreSubmitInfos.data(),
        .commandBufferInfoCount = 1,
        .pCommandBufferInfos = &cmdBufInfo,
        .signalSemaphoreInfoCount = static_cast<uint32_t>(signalSemaphoreSubmitInfos.size()),
        .pSignalSemaphoreInfos = signalSemaphoreSubmitInfos.data(),
    };

    commandBuffer.end();

    // find queue
    vk::Queue queue;
    switch (queueFamily) {
        case CommandQueueFamily::Graphics:
            queue = device.graphicsQueue();
            break;
        case CommandQueueFamily::Compute:
            queue = device.getComputeQueue();
            break;
        case CommandQueueFamily::Transfer:
            queue = device.getTransferQueue();
            break;
        default:
            throw std::runtime_error("Unknown queue family");
    }

    if (queue.submit2(1, &submitInfo, fence) != vk::Result::eSuccess) {
        throw std::runtime_error("Failed to submit command buffer");
    }

    waitSemaphoreSubmitInfos.clear();
    signalSemaphoreSubmitInfos.clear();

    inProgress = false;
}

auto hammock::core::CommandBuffer::waitOnSemaphore(Semaphore &semaphore,
                                                   vk::PipelineStageFlagBits2 stageFlagBits) -> void {
    waitSemaphoreSubmitInfos.push_back(vk::SemaphoreSubmitInfo{
        .semaphore = semaphore.getVulkanSemaphore(),
        .stageMask = stageFlagBits,
    });
}

auto hammock::core::CommandBuffer::signalSemaphore(Semaphore &semaphore) -> void {
    signalSemaphoreSubmitInfos.push_back(vk::SemaphoreSubmitInfo{
        .semaphore = semaphore.getVulkanSemaphore(),
    });
}
