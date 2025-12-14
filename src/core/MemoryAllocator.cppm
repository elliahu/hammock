module;

#define VMA_STATIC_VULKAN_FUNCTIONS 0
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 1
#include "vk_mem_alloc/vk_mem_alloc.h"

#include <stdexcept>

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
    void createBuffer(Allocator allocator, VkBufferCreateInfo *bufferCreateInfo,
                      AllocationCreateInfo *allocationCreateInfo, VkBuffer *buffer, Allocation *allocation,
                      AllocationInfo *allocInfo) {
        if (auto result = vmaCreateBuffer(allocator, bufferCreateInfo, allocationCreateInfo, buffer, allocation,
                                          allocInfo); result != VK_SUCCESS) {
            throw std::runtime_error("failed to create buffer, code: " + result);
        }
    }

    /// Destroy Vulkan buffer handle
    void destroyBuffer(Allocator allocator, VkBuffer buffer, Allocation allocation) {
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
    void flushAllocation(Allocator allocator, Allocation allocation, VkDeviceSize offset, VkDeviceSize size) {
        if (auto result = vmaFlushAllocation(allocator, allocation, offset, size); result != VK_SUCCESS) {
            throw std::runtime_error("failed to flush memory, code: " + result);
        }
    }

    /// Invalidate allocation to make it visible to the host
    void invalidateAllocation(Allocator allocator, Allocation allocation, VkDeviceSize offset, VkDeviceSize size) {
        if (auto result = vmaInvalidateAllocation(allocator, allocation, offset, size); result != VK_SUCCESS) {
            throw std::runtime_error("failed to invalidate memory, code: " + result);
        }
    }

    /// Create vulkan image
    void createImage(Allocator allocator, VkImageCreateInfo *imageCreateInfo,
                     AllocationCreateInfo *allocationCreateInfo, VkImage *image, Allocation *allocation,
                     AllocationInfo *allocInfo) {
        if (auto result = vmaCreateImage(allocator, imageCreateInfo, allocationCreateInfo, image, allocation, allocInfo); result != VK_SUCCESS) {
            throw std::runtime_error("failed to create image, code: " + result);
        }
    }

    /// Destroy vulkan image
    void destroyImage(Allocator allocator, VkImage image, Allocation allocation) {
        vmaDestroyImage(allocator, image, allocation);
    }
}
