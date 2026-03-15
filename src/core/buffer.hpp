#pragma once
#include <stdexcept>
#include <vector>
#include <string>
#include <compare>
#include <vulkan/vulkan.hpp>


#include "base_resource.hpp"
#include "device.hpp"
#include "resource_manager.hpp"
#include "utilities.hpp"
#include "memory_allocator.hpp"

namespace hammock::core {

    enum class BufferType {
        DeviceOnly,
        HostVisible, // Staging buffer
    };

    class VulkanBufferAllocationFlags{
    public:
        constexpr VulkanBufferAllocationFlags(BufferType usage) : type(usage) {
        }

        explicit constexpr operator allocator::AllocationCreateFlags() const {
            allocator::AllocationCreateFlags flags{};

            if (type == BufferType::HostVisible) {
                flags |= allocator::AllocationCreateFlagsBits::VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
            }

            return flags;
        }

    private:
        BufferType type;
    };


    enum class BufferUsage : uint32_t {
        Invalid = 0,
        TransferSrc = 1 << 0,
        TransferDst = 1 << 1,
        UniformBuffer = 1 << 2,
        StorageBuffer = 1 << 3,
        IndexBuffer = 1 << 4,
        VertexBuffer = 1 << 5,
        IndirectBuffer = 1 << 6,
    };

    constexpr BufferUsage operator|(BufferUsage a, BufferUsage b) {
        return static_cast<BufferUsage>(
            static_cast<uint32_t>(a) | static_cast<uint32_t>(b)
        );
    }

    constexpr BufferUsage operator&(BufferUsage a, BufferUsage b) {
        return static_cast<BufferUsage>(
            static_cast<uint32_t>(a) & static_cast<uint32_t>(b)
        );
    }

    class VulkanBufferUsage {
    public:
        constexpr VulkanBufferUsage(BufferUsage usage) : usage(usage) {
        }

        explicit constexpr operator vk::BufferUsageFlags() const {
            vk::BufferUsageFlags flags{};

            if ((usage & BufferUsage::TransferSrc) != BufferUsage::Invalid)
                flags |= vk::BufferUsageFlagBits::eTransferSrc;

            if ((usage & BufferUsage::TransferDst) != BufferUsage::Invalid)
                flags |= vk::BufferUsageFlagBits::eTransferDst;

            if ((usage & BufferUsage::UniformBuffer) != BufferUsage::Invalid)
                flags |= vk::BufferUsageFlagBits::eUniformBuffer;

            if ((usage & BufferUsage::StorageBuffer) != BufferUsage::Invalid)
                flags |= vk::BufferUsageFlagBits::eStorageBuffer;

            if ((usage & BufferUsage::IndexBuffer) != BufferUsage::Invalid)
                flags |= vk::BufferUsageFlagBits::eIndexBuffer;

            if ((usage & BufferUsage::VertexBuffer) != BufferUsage::Invalid)
                flags |= vk::BufferUsageFlagBits::eVertexBuffer;

            if ((usage & BufferUsage::IndirectBuffer) != BufferUsage::Invalid)
                flags |= vk::BufferUsageFlagBits::eIndirectBuffer;

            return flags;
        }

    private:
        BufferUsage usage;
    };


    /// @struct BufferDesc
    /// @brief Describes general buffer
    struct BufferDesc {
        BufferType type = BufferType::HostVisible;
        BufferUsage usage = BufferUsage::Invalid;
        std::uint64_t instanceSize = 0;
        uint32_t instanceCount = 0;
        struct {
            std::uint64_t minOffsetAlignment = 0;
            CommandQueueFamily currentQueueFamily = CommandQueueFamily::Ignored;
            std::vector<CommandQueueFamily> queueFamilies{CommandQueueFamily::Graphics, CommandQueueFamily::Compute, CommandQueueFamily::Transfer};
            vk::SharingMode sharingMode = vk::SharingMode::eConcurrent;
        } advanced{};

    };

    /// @class Buffer
    /// @brief Class representing a data buffer
    class Buffer : public BaseResource {
    protected:
        vk::Buffer buffer_ = nullptr;
        void *mapped_ = nullptr;
        allocator::Allocation allocation_ = nullptr;
        vk::DeviceSize alignmentSize_;
        vk::DeviceSize bufferSize_;
        uint32_t instanceCount_;
        vk::DeviceSize instanceSize_;
        vk::BufferUsageFlags usageFlags_;
        allocator::AllocationCreateFlags memoryPropertyFlags_;
        std::vector<uint32_t> queueFamilyIndices_{};
        CommandQueueFamily queueFamily_;
        vk::SharingMode sharingMode_;


        /// @brief Returns the minimum instance size required to be compatible with devices minOffsetAlignment
        /// @param instanceSize The size of an instance
        /// @param minOffsetAlignment The minimum required alignment, in bytes, for the offset member
        /// @returns VkResult of the buffer mapping call
        vk::DeviceSize getAlignment(const vk::DeviceSize instanceSize, const vk::DeviceSize minOffsetAlignment);

    public:
        Buffer(Device &device, const BufferDesc &desc);

        ~Buffer() override;

        /// @brief Creates the GPU resource and uploads it to memory
        void create() override;

        /// @brief Releases the GPU resource
        void release() override;

        /// @brief Get Vulkan buffer handle
        [[nodiscard]] vk::Buffer* getBufferPtr() { return &buffer_; }

        /// @brief Get Vulkan buffer handle
        [[nodiscard]] vk::Buffer getBuffer() const { return buffer_; }

        /// @brief Get mapped memory pointer
        [[nodiscard]] void *getMappedMemory() const { return mapped_; }

        /// @brief Get instance count
        [[nodiscard]] uint32_t getInstanceCount() const { return instanceCount_; }

        /// @brief Get instance size
        [[nodiscard]] vk::DeviceSize getInstanceSize() const { return instanceSize_; }

        /// @brief Get alignment size
        [[nodiscard]] vk::DeviceSize getAlignmentSize() const { return alignmentSize_; }

        /// @brief Get usage flags
        [[nodiscard]] vk::BufferUsageFlags getUsageFlags() const { return usageFlags_; }

        /// @brief  Get memory property flags
        [[nodiscard]] vk::MemoryPropertyFlags getMemoryPropertyFlags() const;

        /// @brief Get total buffer size
        [[nodiscard]] vk::DeviceSize getBufferSize() const { return bufferSize_; }

        /// @brief Get queue family of the buffer
        [[nodiscard]] CommandQueueFamily getQueueFamily() const { return queueFamily_; }


        /// @brief Map a memory range of this buffer. If successful, mapped points to the specified buffer range.
        /// @param size (Optional) Size of the memory range to map. Pass VK_WHOLE_SIZE to map the complete buffer range.
        /// @param offset (Optional) Byte offset from beginning
        /// @return VkResult of the buffer mapping call
        vk::Result map(vk::DeviceSize size = vk::WholeSize, vk::DeviceSize offset = 0);


        /// @brief Unmap a mapped memory range
        /// @note Does not return a result as vkUnmapMemory can't fail
        void unmap();

        /// @brief Copies the specified data to the mapped buffer. Default value writes whole buffer range
        /// @param data Pointer to the data to copy
        /// @param size (Optional) Size of the data to copy. Pass VK_WHOLE_SIZE to flush the complete buffer range.
        /// @param offset (Optional) Byte offset from beginning of mapped region
        void writeToBuffer(const void *data, vk::DeviceSize size = vk::WholeSize, vk::DeviceSize offset = 0) const;

        /**
         * Flush a memory range of the buffer to make it visible to the device
         *
         * @note Only required for non-coherent memory
         *
         * @param size (Optional) Size of the memory range to flush. Pass VK_WHOLE_SIZE to flush the
         * complete buffer range.
         * @param offset (Optional) Byte offset from beginning
         *
         * @return VkResult of the flush call
         */
        vk::Result flush(vk::DeviceSize size = vk::WholeSize, vk::DeviceSize offset = 0) const;

        /**
         * Create a buffer info descriptor
         *
         * @param size (Optional) Size of the memory range of the descriptor
         * @param offset (Optional) Byte offset from beginning
         *
         * @return VkDescriptorBufferInfo of specified offset and range
         */
        vk::DescriptorBufferInfo descriptorInfo(vk::DeviceSize size = vk::WholeSize, vk::DeviceSize offset = 0) const;

        /**
         * Invalidate a memory range of the buffer to make it visible to the host
         *
         * @note Only required for non-coherent memory
         *
         * @param size (Optional) Size of the memory range to invalidate. Pass VK_WHOLE_SIZE to invalidate
         * the complete buffer range.
         * @param offset (Optional) Byte offset from beginning
         *
         * @return VkResult of the invalidate call
         */
        vk::Result invalidate(vk::DeviceSize size = vk::WholeSize, vk::DeviceSize offset = 0) const;

        /**
        * Copies "instanceSize" bytes of data to the mapped buffer at an offset of index * alignmentSize
        *
        * @param data Pointer to the data to copy
        * @param index Used in offset calculation
        *
        */
        void writeToIndex(const void *data, int index) const;

        /**
        *  Flush the memory range at index * alignmentSize of the buffer to make it visible to the device
        *
        * @param index Used in offset calculation
        *
        */
        vk::Result flushIndex(int index) const;

        /**
        * Create a buffer info descriptor
        *
        * @param index Specifies the region given by index * alignmentSize
        *
        * @return VkDescriptorBufferInfo for instance at index
         */
        vk::DescriptorBufferInfo descriptorInfoForIndex(int index) const;

        /**
        * Invalidate a memory range of the buffer to make it visible to the host
        *
        * @note Only required for non-coherent memory
        *
        * @param index Specifies the region to invalidate: index * alignmentSize
        *
        * @return VkResult of the invalidate call
        */
        vk::Result invalidateIndex(int index) const;

        /**
         * Retrieves device address of the buffer
         */
        vk::DeviceAddress queryDeviceAddress();
    };
}
