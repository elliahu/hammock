module;
#include <vector>
#include <string>
#include <cassert>
#include <stdexcept>
#include "vulkan/vulkan.hpp"

export module hammock.core.image;

import hammock.core.base_resource;
import hammock.core.device;
import hammock.core.utilities;
import hammock.core.buffer;
import hammock.core.memory_allocator;



namespace hammock::core {
    /**
     * Describes general image.
     */
    export struct ImageDesc {
        uint32_t width, height, channels = 4, depth = 1, layers = 1, mips = 1;
        vk::Image image{};
        vk::ImageView view{};
        vk::Format format;
        vk::ImageUsageFlags usage;
        vk::ImageType imageType;
        vk::ImageViewType imageViewType;
        vk::ImageAspectFlags aspectFlags;
        vk::ClearValue clearValue{};
        CommandQueueFamily currentQueueFamily = CommandQueueFamily::Ignored;
        std::vector<CommandQueueFamily> queueFamilies{};
        vk::SharingMode sharingMode = vk::SharingMode::eExclusive;
        vk::MemoryPropertyFlagBits memoryFlags = vk::MemoryPropertyFlagBits::eDeviceLocal;
        vk::ImageTiling tiling = vk::ImageTiling::eOptimal;
    };


    export class Image : public BaseResource {
    protected:
        // Format and usage
        vk::Format m_format;
        vk::ImageUsageFlags m_usage;
        vk::ImageType m_type;
        vk::ImageViewType m_viewType;

        // Image dimensions
        uint32_t m_width, m_height, m_channels, m_depth = 1, m_layers = 1, m_mips = 1;

        // Vulkan handles
        vk::Image m_image{};
        vk::ImageView m_view{};
        vk::ImageLayout m_layout;
        allocator::Allocation m_allocation = nullptr;

        // Attachment
        vk::ClearValue m_clearValue = {};

        CommandQueueFamily m_queueFamily;
        std::vector<uint32_t> m_queueFamilyIndices{};
        vk::SharingMode m_sharingMode;

        vk::ImageTiling m_tiling;
        vk::MemoryPropertyFlags m_memoryFlags;

        vk::ImageAspectFlags m_aspectFlags;

        vk::Sampler m_sampler{};

    public:
        Image(Device &device, uint64_t id, const std::string &name, const ImageDesc &desc)
            : BaseResource(device, id, name) {
            // Fromat and usage
            m_format = desc.format;
            m_usage = desc.usage;
            m_type = desc.imageType;
            m_viewType = desc.imageViewType;
            m_aspectFlags = desc.aspectFlags;

            // Image dimensions
            m_width = desc.width;
            m_height = desc.height;
            m_channels = desc.channels;
            m_depth = desc.depth;
            m_layers = desc.layers;
            m_mips = desc.mips;

            m_queueFamily = desc.currentQueueFamily;

            m_image = desc.image;
            m_view = desc.view;

            // attachment
            m_clearValue = desc.clearValue;

            // queue family indices
            for (auto &family: desc.queueFamilies) {
                if (family == CommandQueueFamily::Graphics)
                    m_queueFamilyIndices.push_back(device.getGraphicsQueueFamilyIndex());
                if (family == CommandQueueFamily::Compute)
                    m_queueFamilyIndices.push_back(device.getComputeQueueFamilyIndex());
                if (family == CommandQueueFamily::Transfer)
                    m_queueFamilyIndices.push_back(device.getTransferQueueFamilyIndex());
            }

            m_sharingMode = desc.sharingMode;

            m_tiling = desc.tiling;

            m_memoryFlags = desc.memoryFlags;

            // Check for support
            vk::FormatProperties formatProperties = device.getPhysicalDevice().getFormatProperties(m_format);

            assert(formatProperties.optimalTilingFeatures & vk::FormatFeatureFlagBits::eTransferDst);

            if (m_type == vk::ImageType::e3D) {
                uint32_t maxImageDimension3D(device.getPhysicalDeviceProperties().limits.maxImageDimension3D);
                assert(m_width <= maxImageDimension3D && m_height <= maxImageDimension3D &&
                    m_depth <= maxImageDimension3D);
            }
        }

        ~Image() override {
            if (isResident()) {
                release();
            }
        }

        [[nodiscard]] vk::ImageLayout getLayout() const { return m_layout; }
        [[nodiscard]] vk::Image getImage() const { return m_image; }
        [[nodiscard]] vk::ImageView getView() const { return m_view; }
        [[nodiscard]] vk::Format getFormat() const { return m_format; }
        [[nodiscard]] uint32_t getMipLevel() const { return m_mips; }
        [[nodiscard]] uint32_t getLayerLevel() const { return m_layers; }
        [[nodiscard]] CommandQueueFamily getQueueFamily() const { return m_queueFamily; }
        [[nodiscard]] vk::Extent3D getExtent() const { return {m_width, m_height, m_depth}; }

        [[nodiscard]] vk::RenderingAttachmentInfo getRenderingAttachmentInfo() const {
            return {
                .imageView = m_view,
                .imageLayout = m_layout,
                .clearValue = m_clearValue
            };
        }

        [[nodiscard]] vk::DescriptorImageInfo getDescriptorImageInfo(vk::Sampler sampler) const {
            return {
                .sampler = sampler,
                .imageView = m_view,
                .imageLayout = m_layout,
            };
        }

        [[nodiscard]] vk::DescriptorImageInfo getDescriptorImageInfo() const {
            return {
                .sampler = m_sampler,
                .imageView = m_view,
                .imageLayout = m_layout,
            };
        }

        vk::ImageAspectFlags getAspectMask() const {
            switch (m_format) {
                // Depth-only formats
                case vk::Format::eD16Unorm:
                case vk::Format::eD32Sfloat:
                    return vk::ImageAspectFlagBits::eDepth;

                    // Depth-stencil formats
                case vk::Format::eD16UnormS8Uint:
                case vk::Format::eD24UnormS8Uint:
                case vk::Format::eD32SfloatS8Uint:
                    return vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil;

                    // Everything else is treated as color
                default:
                    return vk::ImageAspectFlagBits::eColor;
            }
        }

        [[nodiscard]] vk::Sampler createAndGetSampler() const {
            vk::Sampler sampler = nullptr;
            vk::SamplerCreateInfo samplerInfo{};
            samplerInfo.magFilter = vk::Filter::eLinear;
            samplerInfo.minFilter = vk::Filter::eLinear;
            samplerInfo.mipmapMode = vk::SamplerMipmapMode::eLinear;
            samplerInfo.addressModeU = vk::SamplerAddressMode::eRepeat;
            samplerInfo.addressModeV = vk::SamplerAddressMode::eRepeat;
            samplerInfo.addressModeW = vk::SamplerAddressMode::eRepeat;
            samplerInfo.borderColor = vk::BorderColor::eIntOpaqueWhite;
            samplerInfo.minLod = 0.0f;
            samplerInfo.maxLod = static_cast<float>(m_mips);

            if(device.device().createSampler(&samplerInfo, nullptr, &sampler) != vk::Result::eSuccess) {
                throw std::runtime_error("failed to create sampler!");
            };

            return sampler;
        }

        void createSampler() { m_sampler = createAndGetSampler(); }

        [[nodiscard]] vk::Sampler getSampler() const { return m_sampler; }

        /**
         * Transitions to new layout. Transition is recorder to separate command buffer that is submitted
         * after at the end of the call. Might cause sync hazard. Do not call in frame.
         * @param newLayout New layout
         */
        void queueImageLayoutTransition(vk::ImageLayout newLayout) {
            if (newLayout == m_layout) {
                return;
            }
            device.queueImageLayoutTransition(
                m_image, m_layout, newLayout, m_layers, 0, m_mips, 0, getAspectMask());
            m_layout = newLayout;
        }

        vk::ImageSubresourceRange getSubresourceRange(
            uint32_t baseMipLevel = 0,
            uint32_t baseArrayLayer = 0) const {
            return {
                .aspectMask = getAspectMask(),
                .baseMipLevel = baseMipLevel,
                .levelCount = m_mips,
                .baseArrayLayer = baseArrayLayer,
                .layerCount = m_layers
            };
        }

        /**
         * Applies a pipeline barrier to the image. New layout is tracked internally. Layout tracking is not
         * thread safe, so do not call this from multiple threads.
         */
        void recordPipelineBarrier(
            vk::CommandBuffer cmd,
            vk::PipelineStageFlags2 srcStageMask,
            vk::AccessFlags2 srcAccessMask,
            vk::PipelineStageFlags2 dstStageMask,
            vk::AccessFlags2 dstAccessMask,
            vk::ImageLayout oldLayout,
            vk::ImageLayout newLayout,
            uint32_t srcQueueFamilyIndex,
            uint32_t dstQueueFamilyIndex) {
            // Subresource range
            vk::ImageSubresourceRange subresourceRange = getSubresourceRange();

            vk::ImageMemoryBarrier2 imageBarrier = {
                .srcStageMask = srcStageMask,
                .srcAccessMask = srcAccessMask,
                .dstStageMask = dstStageMask,
                .dstAccessMask = dstAccessMask,
                .oldLayout = oldLayout,
                .newLayout = newLayout, // Optional: layout transition
                .srcQueueFamilyIndex = srcQueueFamilyIndex,
                .dstQueueFamilyIndex = dstQueueFamilyIndex,
                .image = m_image,
                .subresourceRange = subresourceRange,
            };

            vk::DependencyInfo depInfo = {
                .imageMemoryBarrierCount = 1,
                .pImageMemoryBarriers = &imageBarrier,
            };

            cmd.pipelineBarrier2(&depInfo);

            m_layout = newLayout;
        }


        /**
         * Copy data from buffer into this image
         * @param buffer Buffer to copy from
         */
        void queueCopyFromBuffer(Buffer buffer) {
            vk::BufferImageCopy region{};
            region.bufferOffset = 0;
            region.bufferRowLength = 0;     // tightly packed
            region.bufferImageHeight = 0;  // tightly packed

            region.imageSubresource.aspectMask = getAspectMask();
            region.imageSubresource.mipLevel = 0;
            region.imageSubresource.baseArrayLayer = 0;
            region.imageSubresource.layerCount = m_layers;

            region.imageOffset = { 0, 0, 0 };
            region.imageExtent = { m_width, m_height, m_depth };

            auto cmd = device.beginSingleTimeCommands();
            cmd.copyBufferToImage(
                buffer.getBuffer(),
                m_image,
                vk::ImageLayout::eTransferDstOptimal,
                1,
                &region
            );
            device.endSingleTimeCommands(cmd);
        }

        void queueCopyFromImage(Image image) {
            vk::ImageCopy region{};
            region.srcSubresource.aspectMask = image.getAspectMask();
            region.srcSubresource.mipLevel = 0;
            region.srcSubresource.baseArrayLayer = 0;
            region.srcSubresource.layerCount = image.getLayerLevel();

            region.srcOffset = { 0, 0, 0 };

            region.dstSubresource.aspectMask = getAspectMask();
            region.dstSubresource.mipLevel = 0;
            region.dstSubresource.baseArrayLayer = 0;
            region.dstSubresource.layerCount = m_layers;

            region.dstOffset = { 0, 0, 0 };
            region.extent = { m_width, m_height, m_depth };

            auto cmd = device.beginSingleTimeCommands();
            cmd.copyImage(
                image.getImage(),
                vk::ImageLayout::eTransferSrcOptimal,
                m_image,
                vk::ImageLayout::eTransferDstOptimal,
                1,
                &region
            );
            device.endSingleTimeCommands(cmd);
        }

        /**
         * Creates the resource on device. This is called when ever this resource is requested and is not
         * resident
         */
        void create() override {
            Logger::log(LOG_LEVEL_DEBUG, "Creating image %s", getName().c_str());
            // Create the image
            vk::ImageCreateInfo imageCreateInfo{};
            imageCreateInfo.imageType = m_type;
            imageCreateInfo.format = m_format;
            imageCreateInfo.mipLevels = m_mips;
            imageCreateInfo.arrayLayers = m_layers;
            imageCreateInfo.samples = vk::SampleCountFlagBits::e1;
            imageCreateInfo.tiling = m_tiling;
            imageCreateInfo.sharingMode = m_sharingMode;
            imageCreateInfo.queueFamilyIndexCount = m_queueFamilyIndices.size();
            imageCreateInfo.pQueueFamilyIndices = m_queueFamilyIndices.data();
            imageCreateInfo.extent.width = m_width;
            imageCreateInfo.extent.height = m_height;
            imageCreateInfo.extent.depth = m_depth;
            imageCreateInfo.initialLayout = vk::ImageLayout::eUndefined;
            imageCreateInfo.usage = m_usage;

            allocator::AllocationCreateInfo allocInfo = {};
            allocInfo.usage = allocator::MemoryUsage::VMA_MEMORY_USAGE_AUTO;
            allocInfo.requiredFlags = static_cast<std::uint32_t>(m_memoryFlags); // FIXME

            try {
                allocator::createImage(device.allocator(), imageCreateInfo, &allocInfo, m_image, &m_allocation,
                                       nullptr);
            } catch (std::runtime_error &err) {
                throw;
            }

            // create image view
            vk::ImageViewCreateInfo viewInfo{};
            viewInfo.image = m_image;
            viewInfo.viewType = m_viewType;
            viewInfo.format = m_format;
            viewInfo.subresourceRange.aspectMask = getAspectMask();
            viewInfo.subresourceRange.baseMipLevel = 0;
            viewInfo.subresourceRange.levelCount = m_mips;
            viewInfo.subresourceRange.baseArrayLayer = 0;
            viewInfo.subresourceRange.layerCount = m_layers;

            if (device.device().createImageView(&viewInfo, nullptr, &m_view) != vk::Result::eSuccess) {
                throw std::runtime_error("failed to create image view");
            }

            resident = true;
        }

        /**
         * Clears the resource from device memory. This is called if the resource is being destroyed ot space
         * is needed in device memory.
         */
        void release() override {
            Logger::log(LOG_LEVEL_DEBUG, "Releasing image %s", getName().c_str());
            if (m_image != nullptr) {
                allocator::destroyImage(device.allocator(), m_image, m_allocation);
            }

            if (m_view != nullptr) {
                device.device().destroyImageView(m_view, nullptr);
            }

            if (m_sampler != nullptr) {
                device.device().destroySampler(m_sampler, nullptr);
            }

            resident = false;
        }

        /**
         * Generates mip map chain for this image
         */
        void generateMips() {
            vk::CommandBuffer commandBuffer = device.beginSingleTimeCommands();

            recordPipelineBarrier(
                commandBuffer,
                vk::PipelineStageFlagBits2::eTopOfPipe,
                vk::AccessFlagBits2::eNone,
                vk::PipelineStageFlagBits2::eTransfer,
                vk::AccessFlagBits2::eTransferWrite,
                vk::ImageLayout::eUndefined,
                vk::ImageLayout::eTransferDstOptimal,
                vk::QueueFamilyIgnored,
                vk::QueueFamilyIgnored
            );

            vk::ImageMemoryBarrier barrier{};
            barrier.image = m_image;
            barrier.srcQueueFamilyIndex = vk::QueueFamilyIgnored;
            barrier.dstQueueFamilyIndex = vk::QueueFamilyIgnored;
            barrier.subresourceRange.aspectMask = getAspectMask();
            barrier.subresourceRange.baseArrayLayer = 0;
            barrier.subresourceRange.layerCount = 1;
            barrier.subresourceRange.levelCount = 1;

            auto mipWidth = static_cast<int32_t>(m_width);
            auto mipHeight = static_cast<int32_t>(m_height);

            for (uint32_t i = 1; i < m_mips; i++) {
                barrier.subresourceRange.baseMipLevel = i - 1;
                barrier.oldLayout = vk::ImageLayout::eTransferDstOptimal;
                barrier.newLayout = vk::ImageLayout::eTransferSrcOptimal;
                barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
                barrier.dstAccessMask = vk::AccessFlagBits::eTransferRead;

                commandBuffer.pipelineBarrier(
                                     vk::PipelineStageFlagBits::eTransfer,
                                     vk::PipelineStageFlagBits::eTransfer,
                                     {},
                                     0,
                                     nullptr,
                                     0,
                                     nullptr,
                                     1,
                                     &barrier);

                vk::ImageBlit blit{};
                blit.srcOffsets[0] = {0, 0, 0};
                blit.srcOffsets[1] = {mipWidth, mipHeight, 1};
                blit.srcSubresource.aspectMask = getAspectMask();
                blit.srcSubresource.mipLevel = i - 1;
                blit.srcSubresource.baseArrayLayer = 0;
                blit.srcSubresource.layerCount = 1;
                blit.dstOffsets[0] = {0, 0, 0};
                blit.dstOffsets[1] = {mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1};
                blit.dstSubresource.aspectMask = getAspectMask();
                blit.dstSubresource.mipLevel = i;
                blit.dstSubresource.baseArrayLayer = 0;
                blit.dstSubresource.layerCount = 1;

                commandBuffer.blitImage(
                               m_image,
                               vk::ImageLayout::eTransferSrcOptimal,
                               m_image,
                               vk::ImageLayout::eTransferDstOptimal,
                               1,
                               &blit,
                               vk::Filter::eLinear);

                barrier.oldLayout = vk::ImageLayout::eTransferSrcOptimal;
                barrier.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
                barrier.srcAccessMask = vk::AccessFlagBits::eTransferRead;
                barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

                commandBuffer.pipelineBarrier(
                                     vk::PipelineStageFlagBits::eTransfer,
                                     vk::PipelineStageFlagBits::eFragmentShader,
                                     {},
                                     0,
                                     nullptr,
                                     0,
                                     nullptr,
                                     1,
                                     &barrier);

                if (mipWidth > 1) mipWidth /= 2;
                if (mipHeight > 1) mipHeight /= 2;
            }

            barrier.subresourceRange.baseMipLevel = m_mips - 1;
            barrier.oldLayout = vk::ImageLayout::eTransferDstOptimal;
            barrier.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
            barrier.srcAccessMask =  vk::AccessFlagBits::eTransferWrite;
            barrier.dstAccessMask =  vk::AccessFlagBits::eShaderRead;

            m_layout = vk::ImageLayout::eShaderReadOnlyOptimal;

            commandBuffer.pipelineBarrier(
                                 vk::PipelineStageFlagBits::eTransfer,
                                 vk::PipelineStageFlagBits::eFragmentShader,
                                 {},
                                 0,
                                 nullptr,
                                 0,
                                 nullptr,
                                 1,
                                 &barrier);

            device.endSingleTimeCommands(commandBuffer);
            device.waitIdle();
        }
    };

    export template<>
    struct ResourceTypeTraits<Image> {
        static constexpr ResourceType type = ResourceType::Image;
    };

    export struct SamplerDesc {
        vk::Filter magFilter = vk::Filter::eLinear;
        vk::Filter minFilter = vk::Filter::eLinear;
        vk::SamplerAddressMode addressModeU = vk::SamplerAddressMode::eRepeat;
        vk::SamplerAddressMode addressModeV = vk::SamplerAddressMode::eRepeat;
        vk::SamplerAddressMode addressModeW = vk::SamplerAddressMode::eRepeat;
        vk::Bool32 anisotropyEnable = true;
        vk::BorderColor borderColor = vk::BorderColor::eIntOpaqueBlack;
        vk::SamplerMipmapMode mipmapMode = vk::SamplerMipmapMode::eLinear;
        uint32_t mips = 1;
        float mipLodBias = 0.0f;
    };

    export class Sampler : public BaseResource {
        vk::Sampler m_sampler = nullptr;
        vk::Filter magFilter;
        vk::Filter minFilter;
        vk::SamplerAddressMode addressModeU;
        vk::SamplerAddressMode addressModeV;
        vk::SamplerAddressMode addressModeW;
        vk::Bool32 anisotropyEnable;
        float maxAnisotropy;
        vk::BorderColor borderColor;
        vk::SamplerMipmapMode mipmapMode;
        uint32_t mips;
        float mipLodBias;

    public:
        Sampler(Device &device, uint64_t id, const std::string &name, const SamplerDesc &desc)
            : BaseResource(device, id, name) {
            magFilter = desc.magFilter;
            minFilter = desc.minFilter;
            addressModeU = desc.addressModeU;
            addressModeV = desc.addressModeV;
            addressModeW = desc.addressModeW;
            anisotropyEnable = desc.anisotropyEnable;
            borderColor = desc.borderColor;
            mipmapMode = desc.mipmapMode;
            mips = desc.mips;
            mipLodBias = desc.mipLodBias;

            // retrieve max anisotropy from physical device
            maxAnisotropy = device.getPhysicalDeviceProperties().limits.maxSamplerAnisotropy;
        }

        ~Sampler() {
            if (resident) {
                Sampler::release();
            }
        }

        void create() override {
            Logger::log(LOG_LEVEL_DEBUG, "Creating sampler %s", getName().c_str());
            vk::SamplerCreateInfo samplerInfo{};
            samplerInfo.magFilter = magFilter;
            samplerInfo.minFilter = minFilter;
            samplerInfo.addressModeU = addressModeU;
            samplerInfo.addressModeV = addressModeV;
            samplerInfo.addressModeW = addressModeW;
            samplerInfo.anisotropyEnable = anisotropyEnable;
            samplerInfo.maxAnisotropy = maxAnisotropy;
            samplerInfo.borderColor = borderColor;
            samplerInfo.unnormalizedCoordinates = false;
            samplerInfo.compareEnable = false;
            samplerInfo.compareOp = vk::CompareOp::eAlways;
            samplerInfo.mipmapMode = mipmapMode;
            samplerInfo.mipLodBias = mipLodBias;
            samplerInfo.minLod = 0.0;
            samplerInfo.maxLod = mips;

            if (device.device().createSampler(&samplerInfo, nullptr, &m_sampler) != vk::Result::eSuccess) {
                throw std::runtime_error("Failed to create sampler");
            }
            resident = true;
        }

        void release() override {
            Logger::log(LOG_LEVEL_DEBUG, "Releasing sampler %s", getName().c_str());
            if (m_sampler != VK_NULL_HANDLE) {
                vkDestroySampler(device.device(), m_sampler, nullptr);
            }
            resident = false;
        }

        [[nodiscard]] VkSampler getSampler() const { return m_sampler; }
    };

    export template<>
    struct ResourceTypeTraits<Sampler> {
        static constexpr ResourceType type = ResourceType::Sampler;
    };
} // namespace hammock::core
