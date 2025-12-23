module;
#include <stdexcept>
#include <vector>
#include <string>

export module hammock.core.buffer;


import hammock.core.base_resource;
import hammock.core.device;
import hammock.core.utilities;
import hammock.core.memory_allocator;
import vulkan_hpp;



namespace hammock::core {
    /**
    * Describes general buffer
    */
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

    export class Buffer : public BaseResource {
    protected:
        void *m_mapped = nullptr;
        allocator::Allocation m_allocation = nullptr;

        vk::DeviceSize m_alignmentSize;
        vk::DeviceSize m_bufferSize;
        uint32_t m_instanceCount;
        vk::DeviceSize m_instanceSize;
        vk::BufferUsageFlags m_usageFlags;
        allocator::AllocationCreateFlags m_memoryPropertyFlags;

        std::vector<uint32_t> m_queueFamilyIndices{};

        CommandQueueFamily m_queueFamily;
        vk::SharingMode m_sharingMode;


        /**
             * Returns the minimum instance size required to be compatible with devices minOffsetAlignment
             *
             * @param instanceSize The size of an instance
             * @param minOffsetAlignment The minimum required alignment, in bytes, for the offset member (eg
             * minUniformBufferOffsetAlignment)
             *
             * @return VkResult of the buffer mapping call
             */
        vk::DeviceSize getAlignment(const vk::DeviceSize instanceSize, const vk::DeviceSize minOffsetAlignment) {
            if (minOffsetAlignment > 0) {
                return (instanceSize + minOffsetAlignment - 1) & ~(minOffsetAlignment - 1);
            }
            return instanceSize;
        }

    public:
        vk::Buffer m_buffer = nullptr;

        Buffer(Device &device, uint64_t id, const std::string &name, const BufferDesc &desc) : BaseResource(
            device, id, name) {
            m_alignmentSize = getAlignment(desc.instanceSize, desc.minOffsetAlignment);
            m_bufferSize = m_alignmentSize * desc.instanceCount;
            m_instanceCount = desc.instanceCount;
            m_instanceSize = desc.instanceSize;
            m_usageFlags = desc.usageFlags;
            m_memoryPropertyFlags = desc.allocationFlags;

            // queue family indices
            for (auto &family: desc.queueFamilies) {
                if (family == CommandQueueFamily::Graphics) m_queueFamilyIndices.push_back(
                    device.getGraphicsQueueFamilyIndex());
                if (family == CommandQueueFamily::Compute) m_queueFamilyIndices.push_back(
                    device.getComputeQueueFamilyIndex());
                if (family == CommandQueueFamily::Transfer) m_queueFamilyIndices.push_back(
                    device.getTransferQueueFamilyIndex());
            }

            m_queueFamily = desc.currentQueueFamily;
            m_sharingMode = desc.sharingMode;
        }

        ~Buffer() override {
            if (isResident()) {
                release();
            }
        }

        /**
         * Creates the actual resource and loads it into memory
         */
        void create() override {
            Logger::log(LOG_LEVEL_DEBUG, "Creating buffer %s of size %d", getName().c_str(), m_bufferSize);
            vk::BufferCreateInfo bufferInfo{};
            bufferInfo.size = m_bufferSize;
            bufferInfo.usage = m_usageFlags;
            bufferInfo.sharingMode = m_sharingMode;
            bufferInfo.queueFamilyIndexCount = m_queueFamilyIndices.size();
            bufferInfo.pQueueFamilyIndices = m_queueFamilyIndices.data();

            allocator::AllocationCreateInfo allocInfo = {};
            allocInfo.usage = allocator::MemoryUsage::VMA_MEMORY_USAGE_AUTO;
            allocInfo.flags = m_memoryPropertyFlags;

            try {
                allocator::createBuffer(device.allocator(), bufferInfo, &allocInfo, m_buffer, &m_allocation, nullptr);
            }
            catch( std::runtime_error &err){
                throw;
            }
            resident = true;
        }

        /**
         * Frees the resource
         */
        void release() override {
            unmap();
            allocator::destroyBuffer(device.allocator(), m_buffer, m_allocation);
            resident = false;
            Logger::log(LOG_LEVEL_DEBUG, "Buffer %s of size %d released", getName().c_str(), m_bufferSize);
        }

        [[nodiscard]] vk::Buffer getBuffer() const { return m_buffer; }
        [[nodiscard]] void *getMappedMemory() const { return m_mapped; }
        [[nodiscard]] uint32_t getInstanceCount() const { return m_instanceCount; }
        [[nodiscard]] vk::DeviceSize getInstanceSize() const { return m_instanceSize; }
        [[nodiscard]] vk::DeviceSize getAlignmentSize() const { return m_alignmentSize; }
        [[nodiscard]] vk::BufferUsageFlags getUsageFlags() const { return m_usageFlags; }
        [[nodiscard]] vk::MemoryPropertyFlags getMemoryPropertyFlags() const { return vk::MemoryPropertyFlags(m_memoryPropertyFlags); }
        [[nodiscard]] vk::DeviceSize getBufferSize() const { return m_bufferSize; }
        [[nodiscard]] CommandQueueFamily getQueueFamily() const { return m_queueFamily; }

        /**
        * Map a memory range of this buffer. If successful, mapped points to the specified buffer range.
        *
        * @param size (Optional) Size of the memory range to map. Pass VK_WHOLE_SIZE to map the complete
        * buffer range.
        * @param offset (Optional) Byte offset from beginning
        *
        * @return VkResult of the buffer mapping call
        */
        vk::Result map(vk::DeviceSize size = vk::WholeSize, vk::DeviceSize offset = 0) {
            if (!m_mapped) {
                try {
                    allocator::mapMemory(device.allocator(), m_allocation, &m_mapped);
                } catch (std::runtime_error &err) {
                    throw;
                }
            }
            return vk::Result::eSuccess;
        }

        /**
        * Unmap a mapped memory range
        *
        * @note Does not return a result as vkUnmapMemory can't fail
        */
        void unmap() {
            if (m_mapped) {
                allocator::unmapMemory(device.allocator(), m_allocation);
                m_mapped = nullptr;
            }
        }

        /**
        * Copies the specified data to the mapped buffer. Default value writes whole buffer range
        *
        * @param data Pointer to the data to copy
        * @param size (Optional) Size of the data to copy. Pass VK_WHOLE_SIZE to flush the complete buffer
        * range.
        * @param offset (Optional) Byte offset from beginning of mapped region
        *
        */
        void writeToBuffer(const void *data, vk::DeviceSize size = vk::WholeSize, vk::DeviceSize offset = 0) const {
            if(!m_mapped) {
                throw std::runtime_error("Cannot copy to unmapped buffer");
            }

            if (size == vk::WholeSize) {
                memcpy(m_mapped, data, m_bufferSize);
            } else {
                char *memOffset = static_cast<char *>(m_mapped);
                memOffset += offset;
                memcpy(memOffset, data, size);
            }
        }

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
        vk::Result flush(vk::DeviceSize size = vk::WholeSize, vk::DeviceSize offset = 0) const {
            try {
                allocator::flushAllocation(device.allocator(), m_allocation, offset, size);
                return vk::Result::eSuccess;
            }catch (std::runtime_error &err) {
                throw;
            }
        }

        /**
         * Create a buffer info descriptor
         *
         * @param size (Optional) Size of the memory range of the descriptor
         * @param offset (Optional) Byte offset from beginning
         *
         * @return VkDescriptorBufferInfo of specified offset and range
         */
        vk::DescriptorBufferInfo descriptorInfo(vk::DeviceSize size = vk::WholeSize, vk::DeviceSize offset = 0) const {
            return vk::DescriptorBufferInfo{
                m_buffer,
                offset,
                size,
            };
        }

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
        vk::Result invalidate(vk::DeviceSize size = vk::WholeSize, vk::DeviceSize offset = 0) const {
            try {
                allocator::invalidateAllocation(device.allocator(), m_allocation, offset, size);
                return vk::Result::eSuccess;
            } catch (std::runtime_error &err) {
                throw;
            }
        }

        /**
        * Copies "instanceSize" bytes of data to the mapped buffer at an offset of index * alignmentSize
        *
        * @param data Pointer to the data to copy
        * @param index Used in offset calculation
        *
        */
        void writeToIndex(const void *data, int index) const {
            writeToBuffer(data, m_instanceSize, index * m_alignmentSize);
        }

        /**
        *  Flush the memory range at index * alignmentSize of the buffer to make it visible to the device
        *
        * @param index Used in offset calculation
        *
        */
        vk::Result flushIndex(int index) const {
            return flush(m_alignmentSize, index * m_alignmentSize);
        }

        /**
        * Create a buffer info descriptor
        *
        * @param index Specifies the region given by index * alignmentSize
        *
        * @return VkDescriptorBufferInfo for instance at index
         */
        vk::DescriptorBufferInfo descriptorInfoForIndex(int index) const {
            return descriptorInfo(m_alignmentSize, index * m_alignmentSize);
        }

        /**
        * Invalidate a memory range of the buffer to make it visible to the host
        *
        * @note Only required for non-coherent memory
        *
        * @param index Specifies the region to invalidate: index * alignmentSize
        *
        * @return VkResult of the invalidate call
        */
        vk::Result invalidateIndex(int index) const {
            return invalidate(m_alignmentSize, index * m_alignmentSize);
        }

        /**
         * Copies data from source buffer into this buffer
         * @param src Source buffer
         * @param size Size of the copy region
         */
        void queuCopyFromBuffer(Buffer buffer,vk::DeviceSize srcOffset = 0, vk::DeviceSize dstOffset = 0,  vk::DeviceSize size = vk::WholeSize) const {
            vk::BufferCopy copyRegion{};
            copyRegion.srcOffset = srcOffset;
            copyRegion.dstOffset = dstOffset;
            copyRegion.size = size;

            auto cmd = device.beginSingleTimeCommands();
            cmd.copyBuffer(buffer.getBuffer(), m_buffer, 1, &copyRegion);
            device.endSingleTimeCommands(cmd);
        }


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
                           uint32_t baseArrayLayer = 0, uint32_t layerCount = 1, vk::Offset3D offset = {0, 0, 0}) {
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

            commandBuffer.copyImageToBuffer(src, vk::ImageLayout::eTransferSrcOptimal, m_buffer, 1, &region);
        }
    };

    export template<>
    struct ResourceTypeTraits<Buffer> {
        static constexpr ResourceType type = ResourceType::Buffer;
    };
}
