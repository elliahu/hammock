#include <stdexcept>
#include <algorithm>
#include <vulkan/vulkan.hpp>

#include "command_buffer.hpp"

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

auto hammock::core::CommandBuffer::addWaitSemaphore(ResourceRef<Semaphore> semaphore,
                                                   vk::PipelineStageFlagBits2 stageFlagBits) -> void {
    waitSemaphoreSubmitInfos_.push_back(vk::SemaphoreSubmitInfo{
        .semaphore = semaphore->getVulkanSemaphore(),
        .stageMask = stageFlagBits,
    });
}

auto hammock::core::CommandBuffer::addSignalSemaphore(ResourceRef<Semaphore> semaphore) -> void {
    signalSemaphoreSubmitInfos_.push_back(vk::SemaphoreSubmitInfo{
        .semaphore = semaphore->getVulkanSemaphore(),
    });
}
auto hammock::core::CommandBuffer::beginRendering(vk::Rect2D renderArea,
    std::span<vk::RenderingAttachmentInfo> colorAttachments,
    std::optional<vk::RenderingAttachmentInfo> depthAttachment,
    std::optional<vk::RenderingAttachmentInfo> stencilAttachment, uint32_t layerCount) -> void {
    vk::RenderingInfo renderInfo{
        .renderArea = renderArea,
        .layerCount = layerCount,
        .colorAttachmentCount = static_cast<uint32_t>(colorAttachments.size()),
        .pColorAttachments = colorAttachments.data(),
        .pDepthAttachment = depthAttachment ? &*depthAttachment : nullptr,
        .pStencilAttachment = stencilAttachment ? &*stencilAttachment : nullptr,
    };

    commandBuffer_.beginRendering(&renderInfo);
}
auto hammock::core::CommandBuffer::beginRendering(vk::Rect2D renderArea,
    std::span<ResourceRef<Image>> colorAttachments, std::span<vk::ImageLayout> colorAttachmentLayouts,
    std::optional<ResourceRef<Image>> depthAttachment, std::optional<vk::ImageLayout> depthAttachmentLayout,
    std::optional<ResourceRef<Image>> stencilAttachment, std::optional<vk::ImageLayout> stencilAttachmentLayout,
    uint32_t layerCount) -> void {

    std::vector<vk::RenderingAttachmentInfo> colors(colorAttachments.size());

    for (size_t i = 0; i < colorAttachments.size(); ++i) {
        auto info = colorAttachments[i]->getRenderingAttachmentInfo();
        info.imageLayout = colorAttachmentLayouts[i];
        colors[i] = info;
    }

    std::optional<vk::RenderingAttachmentInfo> depthInfo;
    std::optional<vk::RenderingAttachmentInfo> stencilInfo;

    if (depthAttachment) depthInfo = depthAttachment->get().getRenderingAttachmentInfo();

    if (stencilAttachment) stencilInfo = stencilAttachment->get().getRenderingAttachmentInfo();

    return beginRendering(renderArea, colors, depthInfo, stencilInfo, layerCount);
}

auto hammock::core::CommandBuffer::endRendering() -> void { commandBuffer_.endRendering(); }

auto hammock::core::CommandBuffer::copyImageToBuffer(ResourceRef<Image> src, ResourceRef<Buffer> dst,
    vk::Extent3D extent, uint32_t mipLevel, uint32_t baseArrayLayer, uint32_t layerCount,
    vk::Offset3D offset) -> void{
    vk::BufferImageCopy region = {};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;
    region.imageSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
    region.imageSubresource.mipLevel = mipLevel;
    region.imageSubresource.baseArrayLayer = baseArrayLayer;
    region.imageSubresource.layerCount = layerCount;
    region.imageOffset = offset;
    region.imageExtent = extent;

    commandBuffer_.copyImageToBuffer(
        src->getImage(), vk::ImageLayout::eTransferSrcOptimal, dst->getBuffer(), {region});
}

auto hammock::core::CommandBuffer::copyBufferToBuffer(ResourceRef<Buffer> src, ResourceRef<Buffer> dst,
    vk::DeviceSize srcOffset, vk::DeviceSize dstOffset, vk::DeviceSize size) -> void {
    vk::BufferCopy copyRegion{};
    copyRegion.srcOffset = srcOffset;
    copyRegion.dstOffset = dstOffset;
    copyRegion.size = size;

    commandBuffer_.copyBuffer(src->getBuffer(), dst->getBuffer(), 1, &copyRegion);
}

auto hammock::core::CommandBuffer::copyBufferToImage(ResourceRef<Buffer> src, ResourceRef<Image> dst,
    vk::DeviceSize srcOffset, vk::Offset3D dstOffset, uint32_t mipLevel)-> void  {
    vk::BufferImageCopy region{};
    region.bufferOffset = srcOffset;
    region.bufferRowLength = 0;    // tightly packed
    region.bufferImageHeight = 0;  // tightly packed

    region.imageSubresource.aspectMask = dst->getAspectMask();
    region.imageSubresource.mipLevel = mipLevel;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = dst->getLayerLevel();

    region.imageOffset = dstOffset;
    region.imageExtent = dst->getExtent();

    commandBuffer_.copyBufferToImage(
        src->getBuffer(), dst->getImage(), vk::ImageLayout::eTransferDstOptimal, 1, &region);
}

auto hammock::core::CommandBuffer::copyImageToImage(
    ResourceRef<Image> src, ResourceRef<Image> dst, vk::Offset3D srcOffset, vk::Offset3D dstOffset)-> void {
    vk::ImageCopy region{};
    region.srcSubresource.aspectMask = src->getAspectMask();
    region.srcSubresource.mipLevel = 0;
    region.srcSubresource.baseArrayLayer = 0;
    region.srcSubresource.layerCount = src->getLayerLevel();
    region.srcOffset = srcOffset;

    region.dstSubresource.aspectMask = dst->getAspectMask();
    region.dstSubresource.mipLevel = 0;
    region.dstSubresource.baseArrayLayer = 0;
    region.dstSubresource.layerCount = dst->getLayerLevel();
    region.dstOffset = dstOffset;
    region.extent = dst->getExtent();

    commandBuffer_.copyImage(src->getImage(),
        vk::ImageLayout::eTransferSrcOptimal,
        dst->getImage(),
        vk::ImageLayout::eTransferDstOptimal,
        1,
        &region);
}

auto hammock::core::CommandBuffer::imagePipelineBarrier(ResourceRef<Image> image,
    vk::PipelineStageFlags2 srcStageMask, vk::AccessFlags2 srcAccessMask,
    vk::PipelineStageFlags2 dstStageMask, vk::AccessFlags2 dstAccessMask, vk::ImageLayout oldLayout,
    vk::ImageLayout newLayout) -> void {
    // Subresource range
    vk::ImageSubresourceRange subresourceRange = image->getSubresourceRange();

    vk::ImageMemoryBarrier2 imageBarrier = {
        .srcStageMask = srcStageMask,
        .srcAccessMask = srcAccessMask,
        .dstStageMask = dstStageMask,
        .dstAccessMask = dstAccessMask,
        .oldLayout = oldLayout,
        .newLayout = newLayout,  // Optional: layout transition
        .srcQueueFamilyIndex = vk::QueueFamilyIgnored,
        .dstQueueFamilyIndex = vk::QueueFamilyIgnored,
        .image = image->getImage(),
        .subresourceRange = subresourceRange,
    };

    vk::DependencyInfo depInfo = {
        .imageMemoryBarrierCount = 1,
        .pImageMemoryBarriers = &imageBarrier,
    };

    commandBuffer_.pipelineBarrier2(&depInfo);
}

auto hammock::core::CommandBuffer::bindPipeline(ResourceRef<Pipeline> pipeline) -> void {
    if (pipeline->getType() == PipelineType::Compute) {
        commandBuffer_.bindPipeline(vk::PipelineBindPoint::eCompute, pipeline->getPipeline());
    } else {
        commandBuffer_.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline->getPipeline());
    }
}

auto hammock::core::CommandBuffer::bindVertexBuffers(
    std::span<ResourceRef<Buffer>> buffers, std::span<vk::DeviceSize> offsets) -> void {
    std::vector<vk::Buffer> buffs(buffers.size());
    std::transform(
        buffers.begin(), buffers.end(), buffs.begin(), [](const auto& b) { return b->getBuffer(); });
    commandBuffer_.bindVertexBuffers(0, buffers.size(), buffs.data(), offsets.data());
}

auto hammock::core::CommandBuffer::bindDescriptorSets(
    vk::PipelineBindPoint bindPoint, ResourceRef<Pipeline> pipeline, std::span<DescriptorSet> sets) -> void{
    commandBuffer_.bindDescriptorSets(bindPoint, pipeline->getPipelineLayout(), 0, sets, nullptr);
}

auto hammock::core::CommandBuffer::setViewport(vk::Viewport viewport) -> void {
    commandBuffer_.setViewport(0, 1, &viewport);
}

auto hammock::core::CommandBuffer::setScissor(vk::Rect2D scissor) -> void {
    commandBuffer_.setScissor(0, 1, &scissor);
}

auto hammock::core::CommandBuffer::pushConstants(ResourceRef<Pipeline> pipeline, vk::ShaderStageFlags stages,
    uint32_t offset, uint32_t size, const void* data) -> void {
    commandBuffer_.pushConstants(pipeline->getPipelineLayout(), stages, offset, size, data);
}
