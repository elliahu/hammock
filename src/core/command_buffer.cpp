#include "command_buffer.hpp"

#include <algorithm>
#include <cstdint>
#include <stdexcept>
#include <vulkan/vulkan.hpp>

#include "image.hpp"
#include "vulkan/vulkan.hpp"

hammock::core::CommandPool::CommandPool(Device& device, CommandQueueFamily family)
    : device_(device), family_(family) {
    vk::CommandPoolCreateInfo poolInfo = {
        .flags =
            vk::CommandPoolCreateFlagBits::eTransient | vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
        .queueFamilyIndex = device_.getQueueFamilyIndex(family),
    };

    if (device.device().createCommandPool(&poolInfo, nullptr, &pool_) != vk::Result::eSuccess) {
        throw std::runtime_error("failed to create graphics command pool!");
    }
}

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
auto hammock::core::CommandBuffer::begin(CommandBufferInheritanceInfo&& inheritance) -> void {
    if (inProgress_) {
        throw std::runtime_error("Cannot begin commandbuffer that is already in progress");
    }

    vk::CommandBufferInheritanceRenderingInfo inheritanceRenderingInfo{
        .colorAttachmentCount = static_cast<uint32_t>(inheritance.colorAttachmentFormats.size()),
        .pColorAttachmentFormats = inheritance.colorAttachmentFormats.data(),
        .depthAttachmentFormat = inheritance.depthAttachmentFormat.value_or(vk::Format::eUndefined),
        .stencilAttachmentFormat = inheritance.stencilAttachmentFormat.value_or(vk::Format::eUndefined)};

    vk::CommandBufferInheritanceInfo inheritanceInfo{.pNext = &inheritanceRenderingInfo};

    vk::CommandBufferBeginInfo beginInfo{
        .flags = vk::CommandBufferUsageFlagBits::eRenderPassContinue, .pInheritanceInfo = &inheritanceInfo};

    if (commandBuffer_.begin(&beginInfo) != vk::Result::eSuccess) {
        throw std::runtime_error("Failed to begin recording a secondary command buffer");
    }

    inProgress_ = true;
};

auto hammock::core::CommandBuffer::end() -> void {
    if (!inProgress_) {
        throw std::runtime_error(
            "Cannot end command buffer that has not started yet (forgot to call begin()?)");
    }

    commandBuffer_.end();

    inProgress_ = false;
}

auto hammock::core::CommandBuffer::submit(vk::Fence fence) -> void {
    if (!inProgress_) {
        throw std::runtime_error(
            "Cannot submit command buffer that has not started yet (forgot to call begin()?)");
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
    switch (family_) {
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

hammock::core::CommandBuffer::CommandBuffer(CommandPool& pool, CommandBufferLevel level)
    : device_(pool.device_), pool_(pool), family_(pool.getFamily()), level_(level) {
    try {
        vk::CommandBufferAllocateInfo allocInfo{};
        allocInfo.level = static_cast<vk::CommandBufferLevel>(VulkanCommandBufferLevel(level));
        allocInfo.commandBufferCount = 1;
        allocInfo.commandPool = pool.getCommandPool();

        if (device_.device().allocateCommandBuffers(&allocInfo, &commandBuffer_) != vk::Result::eSuccess) {
            throw std::runtime_error("failed to allocate command buffer");
        }
    } catch (std::exception& e) {
        throw;
    }
}

hammock::core::CommandBuffer::~CommandBuffer() {
    // Release only if auto free is true and command buffer is valid
    if (commandBuffer_) {
        device_.device().freeCommandBuffers(pool_.getCommandPool(), 1, &commandBuffer_);
    }
}

auto hammock::core::CommandBuffer::addWaitSemaphore(ResourceRef<Semaphore> semaphore, PipelineStage waitStage,
    std::optional<uint64_t> waitSemaphoreValue) -> void {
    vk::SemaphoreSubmitInfo submitInfo{
        .semaphore = semaphore->getVulkanSemaphore(),
        .stageMask = static_cast<vk::PipelineStageFlags2>(VulkanPipelineStageFlags(waitStage)),
    };

    vk::TimelineSemaphoreSubmitInfo timelineInfo{};
    if (waitSemaphoreValue.has_value()) {
        timelineInfo.waitSemaphoreValueCount = 1;
        timelineInfo.pWaitSemaphoreValues = &waitSemaphoreValue.value();
        submitInfo.pNext = &timelineInfo;
    }

    waitSemaphoreSubmitInfos_.push_back(submitInfo);
}

auto hammock::core::CommandBuffer::addSignalSemaphore(
    ResourceRef<Semaphore> semaphore, std::optional<uint64_t> signalValue) -> void {
    vk::SemaphoreSubmitInfo submitInfo{
        .semaphore = semaphore->getVulkanSemaphore(),
    };

    vk::TimelineSemaphoreSubmitInfo timelineInfo{};
    if (signalValue.has_value()) {
        timelineInfo.signalSemaphoreValueCount = 1;
        timelineInfo.pSignalSemaphoreValues = &signalValue.value();
        submitInfo.pNext = &timelineInfo;
    }

    signalSemaphoreSubmitInfos_.push_back(submitInfo);
}
auto hammock::core::CommandBuffer::beginRendering(Rect2D renderArea,
    std::span<vk::RenderingAttachmentInfo> colorAttachments,
    std::optional<vk::RenderingAttachmentInfo> depthAttachment,
    std::optional<vk::RenderingAttachmentInfo> stencilAttachment, uint32_t layerCount) -> void {
    vk::RenderingInfo renderInfo{
        .renderArea = static_cast<vk::Rect2D>(VulkanRect2D(renderArea)),
        .layerCount = layerCount,
        .colorAttachmentCount = static_cast<uint32_t>(colorAttachments.size()),
        .pColorAttachments = colorAttachments.data(),
        .pDepthAttachment = depthAttachment ? &*depthAttachment : nullptr,
        .pStencilAttachment = stencilAttachment ? &*stencilAttachment : nullptr,
    };

    commandBuffer_.beginRendering(&renderInfo);
}
auto hammock::core::CommandBuffer::beginRendering(Rect2D renderArea,
    std::span<ResourceRef<Image>> colorAttachments, std::optional<ResourceRef<Image>> depthAttachment,
    std::optional<ResourceRef<Image>> stencilAttachment, uint32_t layerCount) -> void {
    std::vector<vk::RenderingAttachmentInfo> colors(colorAttachments.size());

    for (size_t i = 0; i < colorAttachments.size(); ++i) {
        auto info = colorAttachments[i]->getRenderingAttachmentInfo(vk::ImageLayout::eColorAttachmentOptimal);
        colors[i] = info;
    }

    std::optional<vk::RenderingAttachmentInfo> depthInfo;
    std::optional<vk::RenderingAttachmentInfo> stencilInfo;

    if (depthAttachment)
        depthInfo = depthAttachment->get().getRenderingAttachmentInfo(vk::ImageLayout::eDepthAttachmentOptimal);

    if (stencilAttachment)
        stencilInfo = stencilAttachment->get().getRenderingAttachmentInfo(vk::ImageLayout::eStencilAttachmentOptimal);

    return beginRendering(renderArea, colors, depthInfo, stencilInfo, layerCount);
}

auto hammock::core::CommandBuffer::endRendering() -> void { commandBuffer_.endRendering(); }

auto hammock::core::CommandBuffer::copyImageToBuffer(ResourceRef<Image> src, ResourceRef<Buffer> dst,
    vk::Extent3D extent, uint32_t mipLevel, uint32_t baseArrayLayer, uint32_t layerCount, vk::Offset3D offset)
    -> void {
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
    vk::DeviceSize srcOffset, vk::Offset3D dstOffset, uint32_t mipLevel) -> void {
    vk::BufferImageCopy region{};
    region.bufferOffset = srcOffset;
    region.bufferRowLength = 0;    // tightly packed
    region.bufferImageHeight = 0;  // tightly packed

    region.imageSubresource.aspectMask = dst->getAspectMask();
    region.imageSubresource.mipLevel = mipLevel;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = dst->getLayerLevel();

    region.imageOffset = dstOffset;
    region.imageExtent = static_cast<vk::Extent3D>(VulkanExtent3D(dst->getExtent3D()));

    commandBuffer_.copyBufferToImage(
        src->getBuffer(), dst->getImage(), vk::ImageLayout::eTransferDstOptimal, 1, &region);
}

auto hammock::core::CommandBuffer::copyImageToImage(
    ResourceRef<Image> src, ResourceRef<Image> dst, vk::Offset3D srcOffset, vk::Offset3D dstOffset) -> void {
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
    region.extent = static_cast<vk::Extent3D>(VulkanExtent3D(dst->getExtent3D()));

    commandBuffer_.copyImage(src->getImage(),
        vk::ImageLayout::eTransferSrcOptimal,
        dst->getImage(),
        vk::ImageLayout::eTransferDstOptimal,
        1,
        &region);
}
auto hammock::core::CommandBuffer::pipelineBarrier(
    std::span<ImageMemoryBarrier> images, std::span<BufferMemoryBarrier> buffers) -> void {
    std::vector<vk::ImageMemoryBarrier2> imageBarriers{};
    std::vector<vk::BufferMemoryBarrier2> bufferBarriers{};

    for (auto& image : images) {
        imageBarriers.push_back(static_cast<vk::ImageMemoryBarrier2>(VulkanImageMemoryBarrier(image)));
    }

    for (auto& buffer : buffers) {
        bufferBarriers.push_back(static_cast<vk::BufferMemoryBarrier2>(VulkanBufferMemoryBarrier(buffer)));
    }

    vk::DependencyInfo depInfo = {
        .bufferMemoryBarrierCount = static_cast<uint32_t>(bufferBarriers.size()),
        .pBufferMemoryBarriers = bufferBarriers.data(),
        .imageMemoryBarrierCount = static_cast<uint32_t>(imageBarriers.size()),
        .pImageMemoryBarriers = imageBarriers.data(),
    };

    commandBuffer_.pipelineBarrier2(&depInfo);
};

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
    vk::PipelineBindPoint bindPoint, ResourceRef<Pipeline> pipeline, std::span<DescriptorSet> sets) -> void {
    commandBuffer_.bindDescriptorSets(bindPoint, pipeline->getPipelineLayout(), 0, sets, nullptr);
}

auto hammock::core::CommandBuffer::setViewport(Viewport viewport) -> void {
    vk::Viewport vp = static_cast<vk::Viewport>(VulkanViewport(viewport));
    commandBuffer_.setViewport(0, 1, &vp);
}

auto hammock::core::CommandBuffer::setScissor(Rect2D scissor) -> void {
    vk::Rect2D rect = static_cast<vk::Rect2D>(VulkanRect2D(scissor));
    commandBuffer_.setScissor(0, 1, &rect);
}

auto hammock::core::CommandBuffer::pushConstants(ResourceRef<Pipeline> pipeline, ShaderStage stages,
    uint32_t offset, uint32_t size, const void* data) -> void {
    commandBuffer_.pushConstants(pipeline->getPipelineLayout(), static_cast<vk::ShaderStageFlags>(VulkanShaderStageFlags(stages)), offset, size, data);
}
hammock::core::CommandPool::~CommandPool() {
    if (pool_) {
        device_.device().destroyCommandPool(pool_);
    }
}
void hammock::core::CommandPool::reset() { device_.device().resetCommandPool(pool_); }
