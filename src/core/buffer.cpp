module;
#include <compare>
#include <vulkan/vulkan.hpp>

module hammock_core;

vk::DeviceSize hammock::core::Buffer::getAlignment(const vk::DeviceSize instanceSize,
                                                   const vk::DeviceSize minOffsetAlignment) {
    if (minOffsetAlignment > 0) {
        return (instanceSize + minOffsetAlignment - 1) & ~(minOffsetAlignment - 1);
    }
    return instanceSize;
}

hammock::core::Buffer::Buffer(Device &device, uint64_t id,
                              const BufferDesc &desc) : BaseResource(
    device, id) {
    alignmentSize_ = getAlignment(desc.instanceSize, desc.minOffsetAlignment);
    bufferSize_ = alignmentSize_ * desc.instanceCount;
    instanceCount_ = desc.instanceCount;
    instanceSize_ = desc.instanceSize;
    usageFlags_ = desc.usageFlags;
    memoryPropertyFlags_ = desc.allocationFlags;

    // queue family indices
    for (auto &family: desc.queueFamilies) {
        if (family == CommandQueueFamily::Graphics)
            queueFamilyIndices_.push_back(
                device.getGraphicsQueueFamilyIndex());
        if (family == CommandQueueFamily::Compute)
            queueFamilyIndices_.push_back(
                device.getComputeQueueFamilyIndex());
        if (family == CommandQueueFamily::Transfer)
            queueFamilyIndices_.push_back(
                device.getTransferQueueFamilyIndex());
    }

    queueFamily_ = desc.currentQueueFamily;
    sharingMode_ = desc.sharingMode;
}

hammock::core::Buffer::~Buffer() {
    if (isResident()) {
        Buffer::release();
    }
}

void hammock::core::Buffer::create() {
    Logger::log(LOG_LEVEL_DEBUG, "Creating buffer of size %d", bufferSize_);
    vk::BufferCreateInfo bufferInfo{};
    bufferInfo.size = bufferSize_;
    bufferInfo.usage = usageFlags_;
    bufferInfo.sharingMode = sharingMode_;
    bufferInfo.queueFamilyIndexCount = queueFamilyIndices_.size();
    bufferInfo.pQueueFamilyIndices = queueFamilyIndices_.data();

    allocator::AllocationCreateInfo allocInfo = {};
    allocInfo.usage = allocator::MemoryUsage::VMA_MEMORY_USAGE_AUTO;
    allocInfo.flags = memoryPropertyFlags_;

    try {
        allocator::createBuffer(device.allocator(), bufferInfo, &allocInfo, buffer_, &allocation_, nullptr);
    } catch (std::runtime_error &err) {
        throw;
    }
    resident = true;
}

void hammock::core::Buffer::release() {
    unmap();
    allocator::destroyBuffer(device.allocator(), buffer_, allocation_);
    resident = false;
    Logger::log(LOG_LEVEL_DEBUG, "Releasing buffer of size %d" , bufferSize_);
}

vk::MemoryPropertyFlags hammock::core::Buffer::getMemoryPropertyFlags() const {
    return vk::MemoryPropertyFlags(memoryPropertyFlags_);
}

vk::Result hammock::core::Buffer::map(vk::DeviceSize size, vk::DeviceSize offset) {
    if (!mapped_) {
        try {
            allocator::mapMemory(device.allocator(), allocation_, &mapped_);
        } catch (std::runtime_error &err) {
            throw;
        }
    }
    return vk::Result::eSuccess;
}

void hammock::core::Buffer::unmap() {
    if (mapped_) {
        allocator::unmapMemory(device.allocator(), allocation_);
        mapped_ = nullptr;
    }
}

void hammock::core::Buffer::writeToBuffer(const void *data, vk::DeviceSize size, vk::DeviceSize offset) const {
    if (!mapped_) {
        throw std::runtime_error("Cannot copy to unmapped buffer");
    }

    if (size == vk::WholeSize) {
        memcpy(mapped_, data, bufferSize_);
    } else {
        char *memOffset = static_cast<char *>(mapped_);
        memOffset += offset;
        memcpy(memOffset, data, size);
    }
}

vk::Result hammock::core::Buffer::flush(vk::DeviceSize size, vk::DeviceSize offset) const {
    try {
        allocator::flushAllocation(device.allocator(), allocation_, offset, size);
        return vk::Result::eSuccess;
    } catch (std::runtime_error &err) {
        throw;
    }
}

vk::DescriptorBufferInfo hammock::core::Buffer::descriptorInfo(vk::DeviceSize size, vk::DeviceSize offset) const {
    return vk::DescriptorBufferInfo{
        buffer_,
        offset,
        size,
    };
}

vk::Result hammock::core::Buffer::invalidate(vk::DeviceSize size, vk::DeviceSize offset) const {
    try {
        allocator::invalidateAllocation(device.allocator(), allocation_, offset, size);
        return vk::Result::eSuccess;
    } catch (std::runtime_error &err) {
        throw;
    }
}

void hammock::core::Buffer::writeToIndex(const void *data, int index) const {
    writeToBuffer(data, instanceSize_, index * alignmentSize_);
}

vk::Result hammock::core::Buffer::flushIndex(int index) const {
    return flush(alignmentSize_, index * alignmentSize_);
}

vk::DescriptorBufferInfo hammock::core::Buffer::descriptorInfoForIndex(int index) const {
    return descriptorInfo(alignmentSize_, index * alignmentSize_);
}

vk::Result hammock::core::Buffer::invalidateIndex(int index) const {
    return invalidate(alignmentSize_, index * alignmentSize_);
}

void hammock::core::Buffer::queuCopyFromBuffer(Buffer buffer, vk::DeviceSize srcOffset, vk::DeviceSize dstOffset,
    vk::DeviceSize size) const {
    vk::BufferCopy copyRegion{};
    copyRegion.srcOffset = srcOffset;
    copyRegion.dstOffset = dstOffset;
    copyRegion.size = size;

    auto cmd = device.beginSingleTimeCommands();
    cmd.copyBuffer(buffer.getBuffer(), buffer_, 1, &copyRegion);
    device.endSingleTimeCommands(cmd);
}

void hammock::core::Buffer::copyFromImage(vk::CommandBuffer commandBuffer, vk::Image src, vk::Extent3D extent,
    uint32_t mipLevel, uint32_t baseArrayLayer, uint32_t layerCount, vk::Offset3D offset) const {
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

    commandBuffer.copyImageToBuffer(src, vk::ImageLayout::eTransferSrcOptimal, buffer_, 1, &region);
}
