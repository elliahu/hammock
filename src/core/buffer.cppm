module;
#include <stdexcept>
#include <vector>
#include <string>
#include <compare>
#include <vulkan/vulkan.hpp>

export module hammock_core:buffer;


import :base_resource;
import :device;
import :utilities;
import :memory_allocator;

namespace hammock::core {
    /// @struct BufferDesc
    /// @brief Describes general buffer
    export struct BufferDesc {
        vk::DeviceSize instanceSize;
        uint32_t instanceCount;
        vk::BufferUsageFlags usageFlags;
        allocator::AllocationCreateFlags allocationFlags;
        vk::DeviceSize minOffsetAlignment;
        CommandQueueFamily currentQueueFamily = CommandQueueFamily::Ignored;
        std::vector<CommandQueueFamily> queueFamilies{};
        vk::SharingMode sharingMode = vk::SharingMode::eExclusive;
    };

    /// @class Buffer
    /// @brief Class representing a data buffer
    export class Buffer : public BaseResource {
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
        Buffer(Device &device, uint64_t id, const BufferDesc &desc);

        ~Buffer() override;

        /// @brief Creates the GPU resource and uploads it to memory
        void create() override;

        /// @brief Releases the GPU resource
        void release() override;

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
         * Copies data from source buffer into this buffer
         * @param src Source buffer
         * @param size Size of the copy region
         */
        void queuCopyFromBuffer(Buffer buffer, vk::DeviceSize srcOffset = 0, vk::DeviceSize dstOffset = 0,
                                vk::DeviceSize size = vk::WholeSize) const;


        /**
         * Copies data from image into this buffer
         * @param commandBuffer Command buffer
         * @param src Source image
         * @param extent Extent of the image
         * @param mipLevel mip level
         * @param baseArrayLayer  base array layer
         * @param layerCount layer count
         * @param offset offset
         */
        void copyFromImage(vk::CommandBuffer commandBuffer, vk::Image src, vk::Extent3D extent, uint32_t mipLevel = 0,
                           uint32_t baseArrayLayer = 0, uint32_t layerCount = 1, vk::Offset3D offset = {0, 0, 0}) const;
    };

    export template<>
    struct ResourceTypeTraits<Buffer> {
        static constexpr ResourceType type = ResourceType::Buffer;
    };
}
