#include "buffer.hpp"

#include <vulkan/vulkan.hpp>

#include "base_resource.hpp"
#include "vulkan/vulkan.hpp"

vk::DeviceSize hammock::core::Buffer::getAlignment(
    const vk::DeviceSize instanceSize, const vk::DeviceSize minOffsetAlignment) {
    if (minOffsetAlignment > 0) {
        return (instanceSize + minOffsetAlignment - 1) & ~(minOffsetAlignment - 1);
    }
    return instanceSize;
}

hammock::core::Buffer::Buffer(Device& device, const BufferDesc& desc) : BaseResource(device) {
    alignmentSize_ = getAlignment(desc.instanceSize, desc.advanced.minOffsetAlignment);
    bufferSize_ = alignmentSize_ * desc.instanceCount;
    instanceCount_ = desc.instanceCount;
    instanceSize_ = desc.instanceSize;
    usageFlags_ = static_cast<vk::BufferUsageFlags>(VulkanBufferUsage{desc.usage});
    memoryPropertyFlags_ =
        static_cast<allocator::AllocationCreateFlags>(VulkanBufferAllocationFlags{desc.type});

    // queue family indices
    for (auto& family : desc.advanced.queueFamilies) {
        if (family == CommandQueueFamily::Graphics)
            queueFamilyIndices_.push_back(device.getGraphicsQueueFamilyIndex());
        if (family == CommandQueueFamily::Compute)
            queueFamilyIndices_.push_back(device.getComputeQueueFamilyIndex());
        if (family == CommandQueueFamily::Transfer)
            queueFamilyIndices_.push_back(device.getTransferQueueFamilyIndex());
    }

    queueFamily_ = desc.advanced.currentQueueFamily;
    sharingMode_ = desc.advanced.sharingMode;

    create();
}

hammock::core::Buffer::~Buffer() {
    if (resident) {
        release();
    }
}

void hammock::core::Buffer::create() {
    vk::BufferCreateInfo bufferInfo{};
    bufferInfo.size = bufferSize_;
    bufferInfo.usage =
        usageFlags_ | vk::BufferUsageFlagBits::eShaderDeviceAddress;  // So that we can reference buffer via
                                                                      // pointers in shaders
    bufferInfo.sharingMode = sharingMode_;
    bufferInfo.queueFamilyIndexCount = queueFamilyIndices_.size();
    bufferInfo.pQueueFamilyIndices = queueFamilyIndices_.data();

    allocator::AllocationCreateInfo allocInfo = {};
    allocInfo.usage = allocator::MemoryUsage::VMA_MEMORY_USAGE_AUTO;
    allocInfo.flags = memoryPropertyFlags_;

    try {
        allocator::createBuffer(device.allocator(), bufferInfo, &allocInfo, buffer_, &allocation_, nullptr);
    } catch (std::runtime_error& err) {
        throw;
    }
    resident = true;
}

void hammock::core::Buffer::release() {
    unmap();
    allocator::destroyBuffer(device.allocator(), buffer_, allocation_);
    resident = false;
}

vk::MemoryPropertyFlags hammock::core::Buffer::getMemoryPropertyFlags() const {
    return vk::MemoryPropertyFlags(memoryPropertyFlags_);
}

vk::Result hammock::core::Buffer::map(vk::DeviceSize size, vk::DeviceSize offset) {
    if (!mapped_) {
        try {
            allocator::mapMemory(device.allocator(), allocation_, &mapped_);
        } catch (std::runtime_error& err) {
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

void hammock::core::Buffer::writeToBuffer(
    const void* data, vk::DeviceSize size, vk::DeviceSize offset) const {
    if (!mapped_) {
        throw std::runtime_error("Cannot copy to unmapped buffer");
    }

    if (size == vk::WholeSize) {
        memcpy(mapped_, data, bufferSize_);
    } else {
        char* memOffset = static_cast<char*>(mapped_);
        memOffset += offset;
        memcpy(memOffset, data, size);
    }
}

vk::Result hammock::core::Buffer::flush(vk::DeviceSize size, vk::DeviceSize offset) const {
    try {
        allocator::flushAllocation(device.allocator(), allocation_, offset, size);
        return vk::Result::eSuccess;
    } catch (std::runtime_error& err) {
        throw;
    }
}

vk::DescriptorBufferInfo hammock::core::Buffer::descriptorInfo(
    vk::DeviceSize size, vk::DeviceSize offset) const {
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
    } catch (std::runtime_error& err) {
        throw;
    }
}

void hammock::core::Buffer::writeToIndex(const void* data, int index) const {
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


vk::DeviceAddress hammock::core::Buffer::queryDeviceAddress() {
    vk::BufferDeviceAddressInfo deviceAddressInfo{.buffer = buffer_};

    return device.device().getBufferAddress(&deviceAddressInfo);
}
