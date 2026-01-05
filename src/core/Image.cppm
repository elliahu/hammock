module;
#include <cstdint>
#include <vector>
#include <string>
#include <cassert>
#include <stdexcept>
#include <compare>
#include "vulkan/vulkan.hpp"

export module hammock_core.image;

import hammock_core.base_resource;
import hammock_core.device;
import hammock_core.utilities;
import hammock_core.buffer;
import hammock_core.memory_allocator;



namespace hammock::core {

    /// @struct ImageDesc
    /// @brief Describes a general image
    export struct ImageDesc {
        std::uint32_t width, height, channels = 4, depth = 1, layers = 1, mips = 1;
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


    /// @class Image
    /// @brief General Vulkan image resource
    export class Image : public BaseResource {
    protected:
        // Format and usage
        vk::Format format_;
        vk::ImageUsageFlags usage_;
        vk::ImageType type_;
        vk::ImageViewType viewType_;

        // Image dimensions
        std::uint32_t width_, height_, channels_, depth_ = 1, layers_ = 1, mips_ = 1;

        // Vulkan handles
        vk::Image image_{};
        vk::ImageView view_{};
        vk::ImageLayout layout_{};
        allocator::Allocation allocation_ = nullptr;

        // Attachment
        vk::ClearValue clearValue_ = {};

        CommandQueueFamily queueFamily_;
        std::vector<std::uint32_t> queueFamilyIndices_{};
        vk::SharingMode sharingMode_;

        vk::ImageTiling tiling_;
        vk::MemoryPropertyFlags memoryFlags_;

        vk::ImageAspectFlags aspectFlags_;
        vk::Sampler sampler_{};

    public:
        Image(Device &device, std::uint64_t id, const std::string &name, const ImageDesc &desc);

        ~Image() override;

        /// @brief Get image layout
        [[nodiscard]] vk::ImageLayout getLayout() const { return layout_; }

        /// @brief Get vulkan image handle
        [[nodiscard]] vk::Image getImage() const { return image_; }

        /// @brief Get vulkan image view handle
        [[nodiscard]] vk::ImageView getView() const { return view_; }

        /// @brief Get image format
        [[nodiscard]] vk::Format getFormat() const { return format_; }

        /// @brief Get number of mip levels
        [[nodiscard]] std::uint32_t getMipLevel() const { return mips_; }

        /// @brief Get number of array layers
        [[nodiscard]] std::uint32_t getLayerLevel() const { return layers_; }

        /// @brief Get queue family
        [[nodiscard]] CommandQueueFamily getQueueFamily() const { return queueFamily_; }

        /// @brief Get the image extent
        [[nodiscard]] vk::Extent3D getExtent() const { return {width_, height_, depth_}; }

        /// @brief Ge the rendering attachment info when using this image as a render target
        [[nodiscard]] vk::RenderingAttachmentInfo getRenderingAttachmentInfo() const;

        /// @brief Get the descriptor image info when using this image in a descriptor set with external sampler
        [[nodiscard]] vk::DescriptorImageInfo getDescriptorImageInfo(vk::Sampler sampler) const;

        /// @brief Get the descriptor image info when using this image in a descriptor set
        [[nodiscard]] vk::DescriptorImageInfo getDescriptorImageInfo() const;

        /// @brief Get image aspect mask based on its format
        vk::ImageAspectFlags getAspectMask() const;

        /// @brief This creates a sampler and returns it.
        /// @note This does not asign the sampler to the image. To create a sampler for this image, call createSampler()
        [[nodiscard]] vk::Sampler createAndGetSampler() const;

        /// @brief Creates a sampler and retains it
        void createSampler() { sampler_ = createAndGetSampler(); }

        /// @brief Get vulkan sampler handle for this image (if not set may be nullptr)
        [[nodiscard]] vk::Sampler getSampler() const { return sampler_; }

        /**
         * Transitions to new layout. Transition is recorder to separate command buffer that is submitted
         * after at the end of the call. Might cause sync hazard. Do not call in frame.
         * @param newLayout New layout
         */
        void queueImageLayoutTransition(vk::ImageLayout newLayout);

        vk::ImageSubresourceRange getSubresourceRange(
            std::uint32_t baseMipLevel = 0,
            std::uint32_t baseArrayLayer = 0) const;

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
            std::uint32_t srcQueueFamilyIndex,
            std::uint32_t dstQueueFamilyIndex);


        /**
         * Copy data from buffer into this image
         * @param buffer Buffer to copy from
         */
        void queueCopyFromBuffer(Buffer buffer) const;

        void queueCopyFromImage(Image image) const;

        /**
         * Creates the resource on device. This is called when ever this resource is requested and is not
         * resident
         */
        void create() override;

        /**
         * Clears the resource from device memory. This is called if the resource is being destroyed ot space
         * is needed in device memory.
         */
        void release() override;

        /**
         * Generates mip map chain for this image
         */
        void generateMips();
    };

    export template<>
    struct ResourceTypeTraits<Image> {
        static constexpr ResourceType type = ResourceType::Image;
    };

    /// @struct SamplerDesc
    /// @brief Describes a vulkan sampler
    export struct SamplerDesc {
        vk::Filter magFilter = vk::Filter::eLinear;
        vk::Filter minFilter = vk::Filter::eLinear;
        vk::SamplerAddressMode addressModeU = vk::SamplerAddressMode::eRepeat;
        vk::SamplerAddressMode addressModeV = vk::SamplerAddressMode::eRepeat;
        vk::SamplerAddressMode addressModeW = vk::SamplerAddressMode::eRepeat;
        vk::Bool32 anisotropyEnable = true;
        vk::BorderColor borderColor = vk::BorderColor::eIntOpaqueBlack;
        vk::SamplerMipmapMode mipmapMode = vk::SamplerMipmapMode::eLinear;
        std::uint32_t mips = 1;
        float mipLodBias = 0.0f;
    };

    /// @class Sampler
    /// @brief Wrapper around vulkan sampler handle
    export class Sampler : public BaseResource {
        vk::Sampler sampler_ = nullptr;
        vk::Filter magFilter_;
        vk::Filter minFilter_;
        vk::SamplerAddressMode addressModeU_;
        vk::SamplerAddressMode addressModeV_;
        vk::SamplerAddressMode addressModeW_;
        vk::Bool32 anisotropyEnable_;
        float maxAnisotropy_;
        vk::BorderColor borderColor_;
        vk::SamplerMipmapMode mipmapMode_;
        std::uint32_t mips_;
        float mipLodBias_;

    public:
        Sampler(Device &device, std::uint64_t id, const std::string &name, const SamplerDesc &desc);

        ~Sampler() override;

        void create() override;

        void release() override;

        /// @brief Get the vulkan sampler handle
        [[nodiscard]] VkSampler getSampler() const { return sampler_; }
    };

    export template<>
    struct ResourceTypeTraits<Sampler> {
        static constexpr ResourceType type = ResourceType::Sampler;
    };
} // namespace hammock::core
