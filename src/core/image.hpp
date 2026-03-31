#pragma once
#include <array>
#include <cassert>
#include <cstdint>
#include <stdexcept>
#include <vector>

#include "base_resource.hpp"
#include "device.hpp"
#include "memory_allocator.hpp"
#include "vulkan/vulkan.hpp"

namespace hammock::core {
    /// @enum ImageFormat
    /// Describes the memory layout of the image
    enum class ImageFormat {
        Undefined,
        R8G8B8A8Uint,
        R8Uint,
        R16G16B16A16Sfloat,
        R32G32B32A32Sfloat,
    };

    /// @class VulkanImageFormat
    /// Used to cast ImageFormat to vk::ImageFormat
    class VulkanImageFormat {
       public:
        constexpr VulkanImageFormat(ImageFormat fmt) : format(fmt) {}
        explicit constexpr operator vk::Format() const {
            switch (format) {
                case ImageFormat::R8G8B8A8Uint:
                    return vk::Format::eR8G8B8A8Unorm;
                case ImageFormat::R8Uint:
                    return vk::Format::eR8Unorm;
                case ImageFormat::R16G16B16A16Sfloat:
                    return vk::Format::eR16G16B16A16Sfloat;
                case ImageFormat::R32G32B32A32Sfloat:
                    return vk::Format::eR32G32B32A32Sfloat;
                case ImageFormat::Undefined:
                default:
                    throw std::runtime_error("unsupported format");
            }
            throw std::runtime_error("unsupported format");
        }

       private:
        ImageFormat format;
    };

    /// @enum ImageUsage
    enum class ImageUsage : uint32_t {
        Invalid = 0,
        TransferSrc = 1 << 0,
        TransferDst = 1 << 1,
        Sampled = 1 << 2,
        Storage = 1 << 3,
        ColorAttachment = 1 << 4,
        DepthStencil = 1 << 5,
    };

    constexpr ImageUsage operator|(ImageUsage a, ImageUsage b) {
        return static_cast<ImageUsage>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
    }

    constexpr ImageUsage operator&(ImageUsage a, ImageUsage b) {
        return static_cast<ImageUsage>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
    }

    /// @class VulkanImageUsage
    /// Used to cast ImageUsage to vk::ImageUsage
    class VulkanImageUsage {
       public:
        constexpr VulkanImageUsage(ImageUsage usage) : usage(usage) {}

        explicit constexpr operator vk::ImageUsageFlags() const {
            vk::ImageUsageFlags flags{};

            if ((usage & ImageUsage::TransferSrc) != ImageUsage::Invalid)
                flags |= vk::ImageUsageFlagBits::eTransferSrc;

            if ((usage & ImageUsage::TransferDst) != ImageUsage::Invalid)
                flags |= vk::ImageUsageFlagBits::eTransferDst;

            if ((usage & ImageUsage::Sampled) != ImageUsage::Invalid)
                flags |= vk::ImageUsageFlagBits::eSampled;

            if ((usage & ImageUsage::Storage) != ImageUsage::Invalid)
                flags |= vk::ImageUsageFlagBits::eStorage;

            if ((usage & ImageUsage::ColorAttachment) != ImageUsage::Invalid)
                flags |= vk::ImageUsageFlagBits::eColorAttachment;

            if ((usage & ImageUsage::DepthStencil) != ImageUsage::Invalid)
                flags |= vk::ImageUsageFlagBits::eDepthStencilAttachment;

            return flags;
        }

       private:
        ImageUsage usage;
    };

    /// @enum ImageType
    enum class ImageType {
        Type2D,
        Type2DArray,
        Type3D,
        TypeCube,
        TypeCubeArray,
    };

    /// @class VulkanImageType
    /// Used to cast ImageType to vk::ImageType
    class VulkanImageType {
       public:
        constexpr VulkanImageType(ImageType type) : type_(type) {}

        explicit constexpr operator vk::ImageType() const {
            switch (type_) {
                case ImageType::Type2D:
                case ImageType::Type2DArray:
                    return vk::ImageType::e2D;
                case ImageType::Type3D:
                    return vk::ImageType::e3D;
                case ImageType::TypeCube:
                case ImageType::TypeCubeArray:
                    return vk::ImageType::e2D;
                default:
                    throw std::runtime_error("unsupported type");
            }
            throw std::runtime_error("unsupported type");
        }

        explicit constexpr operator vk::ImageViewType() const {
            switch (type_) {
                case ImageType::Type2D:
                    return vk::ImageViewType::e2D;
                case ImageType::Type2DArray:
                    return vk::ImageViewType::e2DArray;
                case ImageType::Type3D:
                    return vk::ImageViewType::e3D;
                case ImageType::TypeCube:
                    return vk::ImageViewType::eCube;
                case ImageType::TypeCubeArray:
                    return vk::ImageViewType::eCubeArray;
                default:
                    throw std::runtime_error("unsupported type");
            }
            throw std::runtime_error("unsupported type");
        }

       private:
        ImageType type_;
    };

    /// @struct ImageDesc
    /// @brief Describes a general image
    struct ImageDesc {
        std::uint32_t width = 0;
        std::uint32_t height = 0;
        std::uint32_t channels = 4;
        std::uint32_t depth = 1;
        std::uint32_t layers = 1;
        std::uint32_t mips = 1;
        ImageFormat format;
        ImageUsage usage;
        ImageType type;
        std::array<float, 4> colorClearValue{0.f, 0.f, 0.f, 1.f};
        std::array<float, 2> depthStencilClearValue{1.f, 1.f};
        struct {
            CommandQueueFamily currentQueueFamily = CommandQueueFamily::Ignored;
            std::vector<CommandQueueFamily> queueFamilies{
                CommandQueueFamily::Graphics, CommandQueueFamily::Compute, CommandQueueFamily::Transfer};
            vk::SharingMode sharingMode = vk::SharingMode::eConcurrent;
            vk::MemoryPropertyFlagBits memoryFlags = vk::MemoryPropertyFlagBits::eDeviceLocal;
            vk::ImageTiling tiling = vk::ImageTiling::eOptimal;
        } advanced{};
    };

    struct Offset2D {
        int32_t x, y;
    };

    class VulkanOffset2D{
        public:
        constexpr VulkanOffset2D(Offset2D offset) : offset_(offset) {}

        explicit constexpr operator vk::Offset2D() const {
            return vk::Offset2D{offset_.x, offset_.y};
        }

        private:
      Offset2D offset_;
    };

    struct Extent2D {
        uint32_t x, y;
    };

    class VulkanExtent2D {
       public:
        constexpr VulkanExtent2D(Extent2D extent) : extent_(extent) {}

        explicit constexpr operator vk::Extent2D() const { return vk::Extent2D{extent_.x, extent_.y}; }

       private:
        Extent2D extent_;
    };

    struct Extent3D {
        uint32_t x, y, z;
    };

    class VulkanExtent3D {
       public:
        constexpr VulkanExtent3D(Extent3D extent) : extent_(extent) {}

        explicit constexpr operator vk::Extent2D() const { return vk::Extent2D{extent_.x, extent_.y}; }
        explicit constexpr operator vk::Extent3D() const {
            return vk::Extent3D{extent_.x, extent_.y, extent_.z};
        }

       private:
        Extent3D extent_;
    };

    struct Depth {
        float min, max;
    };

    struct Viewport {
        Offset2D offset;
        Extent2D extent;
        Depth depth;
    };

    class VulkanViewport {
       public:
        constexpr VulkanViewport(Viewport viewport) : viewport_(viewport) {}

        explicit constexpr operator vk::Viewport() const {
            return vk::Viewport{.x = static_cast<float>(viewport_.offset.x),
                .y = static_cast<float>(viewport_.offset.y),
                .width = static_cast<float>(viewport_.extent.x),
                .height = static_cast<float>(viewport_.extent.y),
                .minDepth = viewport_.depth.min,
                .maxDepth = viewport_.depth.max};
        }

       private:
        Viewport viewport_;
    };

    struct Rect2D {
        Offset2D offset;
        Extent2D extent;
    };

    class VulkanRect2D {
       public:
       constexpr VulkanRect2D(Rect2D rect) : rect_(rect){}
       explicit constexpr operator vk::Rect2D() const {
           return vk::Rect2D{
               .offset = static_cast<vk::Offset2D>(VulkanOffset2D(rect_.offset)),
               .extent = static_cast<vk::Extent2D>(VulkanExtent2D(rect_.extent))
           };
       }
       private:
        Rect2D rect_;
    };

    /// @class Image
    /// @brief General Vulkan image resource
    class Image : public BaseResource {
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
        allocator::Allocation allocation_ = nullptr;

        // Attachment
        vk::ClearValue clearValue_ = {};

        CommandQueueFamily queueFamily_;
        std::vector<std::uint32_t> queueFamilyIndices_{};
        vk::SharingMode sharingMode_;

        vk::ImageTiling tiling_;
        vk::MemoryPropertyFlags memoryFlags_;

        vk::Sampler sampler_{};

       public:
        Image(Device& device, const ImageDesc& desc);

        ~Image() override;

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
        [[nodiscard]] Extent3D getExtent3D() const { return {width_, height_, depth_}; }
        [[nodiscard]] Extent2D getExtent2D() const { return {width_, height_}; }

        /// @brief Ge the rendering attachment info when using this image as a render target
        [[nodiscard]] vk::RenderingAttachmentInfo getRenderingAttachmentInfo(vk::ImageLayout layout) const;

        /// @brief Get the descriptor image info when using this image in a descriptor set with external
        /// sampler
        [[nodiscard]] vk::DescriptorImageInfo getDescriptorImageInfo(vk::Sampler sampler) const;

        /// @brief Get the descriptor image info when using this image in a descriptor set
        [[nodiscard]] vk::DescriptorImageInfo getDescriptorImageInfo() const;

        /// @brief Get image aspect mask based on its format
        [[nodiscard]] vk::ImageAspectFlags getAspectMask() const;

        /// @brief Creates a viewport for this image
        [[nodiscard]] Viewport createViewport(Offset2D offset = {0,0}, Depth depth = {0.f, 1.f}){
            return Viewport{
                .offset = offset,
                .extent = getExtent2D(),
                .depth = depth
            };
        }

        /// @brief Creates a Rect2D scissor for this image
        Rect2D createScissor(Offset2D offset = {0,0}){
            return Rect2D{
                .offset = offset,
                .extent = getExtent2D()
            };
        }

        /// @brief This creates a sampler and returns it.
        /// @note This does not asign the sampler to the image. To create a sampler for this image, call
        /// createSampler()
        [[nodiscard]] vk::Sampler createAndGetSampler() const;

        /// @brief Creates a sampler and retains it
        void createSampler() { sampler_ = createAndGetSampler(); }

        /// @brief Get vulkan sampler handle for this image (if not set may be nullptr)
        [[nodiscard]] vk::Sampler getSampler() const { return sampler_; }

        vk::ImageSubresourceRange getSubresourceRange(
            std::uint32_t baseMipLevel = 0, std::uint32_t baseArrayLayer = 0) const;

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
    };

}  // namespace hammock::core
