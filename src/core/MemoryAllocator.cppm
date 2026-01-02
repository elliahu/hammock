module;

#define VMA_STATIC_VULKAN_FUNCTIONS 0
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 1
#include "vk_mem_alloc/vk_mem_alloc.h"

#include <stdexcept>
#include <vulkan/vulkan.hpp>

export module hammock.core.memory_allocator;


export namespace hammock::core::allocator {
    using Allocator = ::VmaAllocator;
    using Allocation = ::VmaAllocation;
    using AllocationCreateInfo = ::VmaAllocationCreateInfo;
    using AllocationCreateFlags = ::VmaAllocationCreateFlags;
    using AllocationCreateFlagsBits = ::VmaAllocationCreateFlagBits;
    using MemoryUsage = ::VmaMemoryUsage;
    using AllocationInfo = ::VmaAllocationInfo;
    using VulkanFunctions = ::VmaVulkanFunctions;
    using AllocatorCreateInfo = ::VmaAllocatorCreateInfo;
    using AllocatorCreateFlags = ::VmaAllocatorCreateFlags;
    using AllocatorCreateFlagBits = ::VmaAllocatorCreateFlagBits;
    using VulkanFunctions = ::VmaVulkanFunctions;

    /// Create memory allocator
    void createAllocator(AllocatorCreateInfo * allocatorInfo, Allocator * allocator) {
        if (auto result = vmaCreateAllocator(allocatorInfo, allocator); result != VK_SUCCESS) {
            throw std::runtime_error("Failed to create allocator, code: " + result);
        }
    }

    /// Destroy allocator
    void destroyAllocator(Allocator allocator) {
        vmaDestroyAllocator(allocator);
    }

    /// Create Vulkan buffer handle
    void createBuffer(Allocator allocator, vk::BufferCreateInfo bufferCreateInfo,
                      AllocationCreateInfo *allocationCreateInfo, vk::Buffer &buffer, Allocation *allocation,
                      AllocationInfo *allocInfo = nullptr) {

        auto bufferInfo = static_cast<VkBufferCreateInfo>(bufferCreateInfo);
        VkBuffer rawBuffer = VK_NULL_HANDLE;

        if (auto result = vmaCreateBuffer(allocator, &bufferInfo, allocationCreateInfo, &rawBuffer, allocation,
                                          allocInfo); result != VK_SUCCESS) {
            throw std::runtime_error("failed to create buffer, code: " + result);
        }

        buffer = vk::Buffer(rawBuffer);
    }

    /// Destroy Vulkan buffer handle
    void destroyBuffer(Allocator allocator, vk::Buffer &buffer, Allocation allocation) {
        vmaDestroyBuffer(allocator, buffer, allocation);
    }

    /// Map memory
    void mapMemory(Allocator allocator, Allocation allocation, void **pData) {
        if (auto result = vmaMapMemory(allocator, allocation, pData); result != VK_SUCCESS) {
            throw std::runtime_error("failed to map memory, code: " + result);
        }
    }

    /// Unmap memory
    void unmapMemory(Allocator allocator, Allocation allocation) {
        vmaUnmapMemory(allocator, allocation);
    }

    /// Flush memory
    void flushAllocation(Allocator allocator, Allocation allocation, vk::DeviceSize offset, vk::DeviceSize size) {
        if (auto result = vmaFlushAllocation(allocator, allocation, offset, size); result != VK_SUCCESS) {
            throw std::runtime_error("failed to flush memory, code: " + result);
        }
    }

    /// Invalidate allocation to make it visible to the host
    void invalidateAllocation(Allocator allocator, Allocation allocation, vk::DeviceSize offset, vk::DeviceSize size) {
        if (auto result = vmaInvalidateAllocation(allocator, allocation, offset, size); result != VK_SUCCESS) {
            throw std::runtime_error("failed to invalidate memory, code: " + result);
        }
    }

    /// Create vulkan image
    void createImage(Allocator allocator, vk::ImageCreateInfo imageCreateInfo,
                     AllocationCreateInfo *allocationCreateInfo, vk::Image &image, Allocation *allocation,
                     AllocationInfo *allocInfo) {
        VkImageCreateInfo imageInfo = static_cast<VkImageCreateInfo>(imageCreateInfo);
        VkImage rawImage = VK_NULL_HANDLE;
        if (auto result = vmaCreateImage(allocator, &imageInfo, allocationCreateInfo, &rawImage, allocation, allocInfo); result != VK_SUCCESS) {
            throw std::runtime_error("failed to create image, code: " + result);
        }

        image = vk::Image(rawImage);
    }

    /// Destroy vulkan image
    void destroyImage(Allocator allocator, vk::Image image, Allocation allocation) {
        vmaDestroyImage(allocator, image, allocation);
    }
}
