module;
#include <stdexcept>
#include <compare>
#include <vulkan/vulkan.hpp>

module hammock.core.command_buffer;

import hammock.core.semaphore;

auto hammock::core::CommandBuffer::begin() -> void {
    if (inProgress_) {
        throw std::runtime_error("Cannot begin commandbuffer that is already in progress");
    }

    vk::CommandBufferBeginInfo beginInfo{};

    if (commandBuffer_.begin(&beginInfo) != vk::Result::eSuccess) {
        throw std::runtime_error("Failed to begin recording a command buffer");
    }

    inProgress_ = true;
}

auto hammock::core::CommandBuffer::end() -> void {
    if (!inProgress_) {
        throw std::runtime_error("Cannot end command buffer that has not started yet (forgot to call begin()?)");
    }

    commandBuffer_.end();

    inProgress_ = false;
}

auto hammock::core::CommandBuffer::submit(vk::Fence fence) -> void {
    if (!inProgress_) {
        throw std::runtime_error("Cannot submit command buffer that has not started yet (forgot to call begin()?)");
    }

    vk::CommandBufferSubmitInfo cmdBufInfo = {
        .commandBuffer = commandBuffer_,
    };

    vk::SubmitInfo2 submitInfo = {
        .waitSemaphoreInfoCount = static_cast<uint32_t>(waitSemaphoreSubmitInfos_.size()),
        .pWaitSemaphoreInfos = waitSemaphoreSubmitInfos_.data(),
        .commandBufferInfoCount = 1,
        .pCommandBufferInfos = &cmdBufInfo,
        .signalSemaphoreInfoCount = static_cast<uint32_t>(signalSemaphoreSubmitInfos_.size()),
        .pSignalSemaphoreInfos = signalSemaphoreSubmitInfos_.data(),
    };

    commandBuffer_.end();

    // find queue
    vk::Queue queue;
    switch (queueFamily_) {
        case CommandQueueFamily::Graphics:
            queue = device_.graphicsQueue();
            break;
        case CommandQueueFamily::Compute:
            queue = device_.getComputeQueue();
            break;
        case CommandQueueFamily::Transfer:
            queue = device_.getTransferQueue();
            break;
        default:
            throw std::runtime_error("Unknown queue family");
    }

    if (queue.submit2(1, &submitInfo, fence) != vk::Result::eSuccess) {
        throw std::runtime_error("Failed to submit command buffer");
    }

    waitSemaphoreSubmitInfos_.clear();
    signalSemaphoreSubmitInfos_.clear();

    inProgress_ = false;
}

hammock::core::CommandBuffer::CommandBuffer(Device &device, CommandQueueFamily queueFamily): device_(device), queueFamily_(queueFamily) {
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

        if (device.device().allocateCommandBuffers(&allocInfo, &commandBuffer_) != vk::Result::eSuccess) {
            throw std::runtime_error("failed to allocate command buffer");
        }
    } catch (std::exception &e) {
        throw;
    }
}

hammock::core::CommandBuffer::~CommandBuffer() {
    if (commandBuffer_) {
        if (queueFamily_ == CommandQueueFamily::Graphics) {
            device_.device().freeCommandBuffers( device_.getGraphicsCommandPool(), 1, &commandBuffer_);
        } else if (queueFamily_ == CommandQueueFamily::Compute) {
            device_.device().freeCommandBuffers(device_.getComputeCommandPool(), 1, &commandBuffer_);
        } else if (queueFamily_ == CommandQueueFamily::Transfer) {
            device_.device().freeCommandBuffers(device_.getTransferCommandPool(), 1, &commandBuffer_);
        }
    }
}

auto hammock::core::CommandBuffer::waitOnSemaphore(Semaphore &semaphore,
                                                   vk::PipelineStageFlagBits2 stageFlagBits) -> void {
    waitSemaphoreSubmitInfos_.push_back(vk::SemaphoreSubmitInfo{
        .semaphore = semaphore.getVulkanSemaphore(),
        .stageMask = stageFlagBits,
    });
}

auto hammock::core::CommandBuffer::signalSemaphore(Semaphore &semaphore) -> void {
    signalSemaphoreSubmitInfos_.push_back(vk::SemaphoreSubmitInfo{
        .semaphore = semaphore.getVulkanSemaphore(),
    });
}
