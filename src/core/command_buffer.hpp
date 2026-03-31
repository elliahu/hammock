#pragma once
#include <cstdint>
#include <optional>
#include <stdexcept>
#include <vector>
#include <vulkan/vulkan.hpp>

#include "buffer.hpp"
#include "descriptors.hpp"
#include "device.hpp"
#include "image.hpp"
#include "pipeline.hpp"
#include "resource_manager.hpp"
#include "semaphore.hpp"
#include "vulkan/vulkan.hpp"

namespace hammock::core {

    /// @class CommandPool
    /// Used to allocate command buffers from
    class CommandPool {
        friend class CommandBuffer;

       public:
        /// Create the command pool
        /// @param device The device to create the command pool on
        /// @param family The queue family to use for the command pool
        CommandPool(Device& device, CommandQueueFamily family);

        /// Destructor automatically releases the vulkan pool
        ~CommandPool();

        /// Get the vulkan command pool handle
        vk::CommandPool getCommandPool() { return pool_; }
        /// Get family of this pool
        CommandQueueFamily getFamily() { return family_; }
        /// Reset the pool
        void reset();

       private:
        Device& device_;
        vk::CommandPool pool_;
        CommandQueueFamily family_;
    };

    /// @struct CommandBufferLevel
    /// Describes the level of the command buffer (primary or secondary)
    enum class CommandBufferLevel { Primary, Secondary };

    /// @class VulkanCommandBufferLevel
    /// This class is used to map CommandBufferLevel to vk::CommandBufferLevel
    class VulkanCommandBufferLevel {
       public:
        constexpr VulkanCommandBufferLevel(CommandBufferLevel level) : level_(level) {}
        explicit constexpr operator vk::CommandBufferLevel() const {
            if (level_ == CommandBufferLevel::Primary) return vk::CommandBufferLevel::ePrimary;
            if (level_ == CommandBufferLevel::Secondary) return vk::CommandBufferLevel::eSecondary;
            throw std::runtime_error("invalid command buffer level");
        }

       private:
        CommandBufferLevel level_;
    };

    /// @enum PipelineStage
    /// Stage of the gpu pipeline
    enum class PipelineStage : uint32_t {
        Invalid = 0,           // No wait
        AllCommands = 1 << 0,  // Conservative, kills parallelism
        AllGraphics = 1 << 1,
        AllTransfer = 1 << 2,
        VertexInput = 1 << 3,
        VertexShader = 1 << 4,
        GeometryShader = 1 << 5,
        FragmentShader = 1 << 6,
        ColorAttachmentOutput = 1 << 7,
        EarlyFragmentTest = 1 << 8,
        LateFragmentTest = 1 << 9,
        ComputeShader = 1 << 10,
        Transfer = 1 << 11,
        TopOfPipeline = 1 << 12,
        BottomOfPipeline = 1 << 13  // No wait, does not make memory visible
    };

    constexpr PipelineStage operator|(PipelineStage a, PipelineStage b) {
        return static_cast<PipelineStage>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
    }

    constexpr PipelineStage operator&(PipelineStage a, PipelineStage b) {
        return static_cast<PipelineStage>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
    }

    class VulkanPipelineStageFlags {
       public:
        VulkanPipelineStageFlags(PipelineStage stage) : stage_(stage) {}

        explicit constexpr operator vk::PipelineStageFlags2() const {
            vk::PipelineStageFlags2 flags{};

            if ((stage_ & PipelineStage::TopOfPipeline) != PipelineStage::Invalid)
                flags |= vk::PipelineStageFlagBits2::eTopOfPipe;

            if ((stage_ & PipelineStage::BottomOfPipeline) != PipelineStage::Invalid)
                flags |= vk::PipelineStageFlagBits2::eBottomOfPipe;

            if ((stage_ & PipelineStage::AllCommands) != PipelineStage::Invalid)
                flags |= vk::PipelineStageFlagBits2::eAllCommands;

            if ((stage_ & PipelineStage::AllGraphics) != PipelineStage::Invalid)
                flags |= vk::PipelineStageFlagBits2::eAllGraphics;

            if ((stage_ & PipelineStage::AllTransfer) != PipelineStage::Invalid)
                flags |= vk::PipelineStageFlagBits2::eAllTransfer;

            if ((stage_ & PipelineStage::VertexInput) != PipelineStage::Invalid)
                flags |= vk::PipelineStageFlagBits2::eVertexInput;

            if ((stage_ & PipelineStage::VertexShader) != PipelineStage::Invalid)
                flags |= vk::PipelineStageFlagBits2::eVertexShader;

            if ((stage_ & PipelineStage::GeometryShader) != PipelineStage::Invalid)
                flags |= vk::PipelineStageFlagBits2::eGeometryShader;

            if ((stage_ & PipelineStage::FragmentShader) != PipelineStage::Invalid)
                flags |= vk::PipelineStageFlagBits2::eFragmentShader;

            if ((stage_ & PipelineStage::ColorAttachmentOutput) != PipelineStage::Invalid)
                flags |= vk::PipelineStageFlagBits2::eColorAttachmentOutput;

            if ((stage_ & PipelineStage::EarlyFragmentTest) != PipelineStage::Invalid)
                flags |= vk::PipelineStageFlagBits2::eEarlyFragmentTests;

            if ((stage_ & PipelineStage::LateFragmentTest) != PipelineStage::Invalid)
                flags |= vk::PipelineStageFlagBits2::eLateFragmentTests;

            if ((stage_ & PipelineStage::ComputeShader) != PipelineStage::Invalid)
                flags |= vk::PipelineStageFlagBits2::eComputeShader;

            if ((stage_ & PipelineStage::Transfer) != PipelineStage::Invalid)
                flags |= vk::PipelineStageFlagBits2::eTransfer;

            return flags;
        }

       private:
        PipelineStage stage_;
    };

    /// @enum ResourceState
    /// Possible state of a resource
    enum class ResourceState {
        Undefined,
        ColorAttachment,
        DepthAttachment,
        ShaderRead,
        TransferSrc,
        TransferDst,
        VertexBuffer,
        IndexBuffer,
        UniformBuffer,
    };

    /// @struct MemoryBarrierInfo
    /// Info struct that describes a memory barrier stage (src or dst).
    /// You can use a convenient helper `MemoryBarrierInfo::fromState(state)` to create `MemoryBarrierInfo`
    /// from `ResourceState`
    struct MemoryBarrierInfo {
        vk::PipelineStageFlags2 stageMask;
        vk::AccessFlags2 accessMask;
        vk::ImageLayout layout;  // Only relevant for images

        static MemoryBarrierInfo fromState(ResourceState state) {
            switch (state) {
                case ResourceState::Undefined:
                    return {};

                case ResourceState::ColorAttachment:
                    return {vk::PipelineStageFlagBits2::eColorAttachmentOutput,
                        vk::AccessFlagBits2::eColorAttachmentWrite,
                        vk::ImageLayout::eColorAttachmentOptimal};

                case ResourceState::DepthAttachment:
                    return {vk::PipelineStageFlagBits2::eEarlyFragmentTests,
                        vk::AccessFlagBits2::eDepthStencilAttachmentWrite,
                        vk::ImageLayout::eDepthAttachmentOptimal};

                case ResourceState::ShaderRead:
                    return {vk::PipelineStageFlagBits2::eFragmentShader,
                        vk::AccessFlagBits2::eShaderRead,
                        vk::ImageLayout::eShaderReadOnlyOptimal};

                case ResourceState::TransferSrc:
                    return {vk::PipelineStageFlagBits2::eTransfer,
                        vk::AccessFlagBits2::eTransferRead,
                        vk::ImageLayout::eTransferSrcOptimal};

                case ResourceState::TransferDst:
                    return {vk::PipelineStageFlagBits2::eTransfer,
                        vk::AccessFlagBits2::eTransferWrite,
                        vk::ImageLayout::eTransferDstOptimal};

                case ResourceState::VertexBuffer:
                    return {vk::PipelineStageFlagBits2::eVertexShader,
                        vk::AccessFlagBits2::eVertexAttributeRead,
                        vk::ImageLayout::eShaderReadOnlyOptimal};

                case ResourceState::IndexBuffer:
                    return {vk::PipelineStageFlagBits2::eVertexShader,
                        vk::AccessFlagBits2::eIndexRead,
                        vk::ImageLayout::eShaderReadOnlyOptimal};

                case ResourceState::UniformBuffer:
                    return {vk::PipelineStageFlagBits2::eVertexShader,
                        vk::AccessFlagBits2::eUniformRead,
                        vk::ImageLayout::eShaderReadOnlyOptimal};
            }
        }
    };

    /// @struct ImageMemoryBarrier
    struct ImageMemoryBarrier {
        ResourceRef<Image> image;
        ResourceState from;
        ResourceState to;
    };

    class VulkanImageMemoryBarrier {
       public:
        VulkanImageMemoryBarrier(ImageMemoryBarrier barrier)
            : image_(barrier.image), from_(barrier.from), to_(barrier.to) {}

        explicit constexpr operator vk::ImageMemoryBarrier2() const {
            MemoryBarrierInfo fromBarrierInfo = MemoryBarrierInfo::fromState(from_);
            MemoryBarrierInfo toBarrierInfo = MemoryBarrierInfo::fromState(to_);

            return vk::ImageMemoryBarrier2{
                .srcStageMask = fromBarrierInfo.stageMask,
                .srcAccessMask = fromBarrierInfo.accessMask,
                .dstStageMask = toBarrierInfo.stageMask,
                .dstAccessMask = toBarrierInfo.accessMask,
                .oldLayout = fromBarrierInfo.layout,
                .newLayout = toBarrierInfo.layout,
                .srcQueueFamilyIndex = vk::QueueFamilyIgnored,
                .dstQueueFamilyIndex = vk::QueueFamilyIgnored,
                .image = image_->getImage(),
                .subresourceRange = image_->getSubresourceRange(),
            };
        }

       private:
        ResourceRef<Image> image_;
        ResourceState from_, to_;
    };

    /// @struct BufferMemoryBarrier
    struct BufferMemoryBarrier {
        ResourceRef<Buffer> buffer;
        ResourceState from;
        ResourceState to;
        uint64_t offset = 0;
        uint64_t size = vk::WholeSize;
    };

    class VulkanBufferMemoryBarrier {
       public:
        VulkanBufferMemoryBarrier(BufferMemoryBarrier barrier)
            : buffer_(barrier.buffer),
              from_(barrier.from),
              to_(barrier.to),
              offset_(barrier.offset),
              size_(barrier.size) {}

        explicit constexpr operator vk::BufferMemoryBarrier2() const {
            MemoryBarrierInfo fromBarrierInfo = MemoryBarrierInfo::fromState(from_);
            MemoryBarrierInfo toBarrierInfo = MemoryBarrierInfo::fromState(to_);

            return vk::BufferMemoryBarrier2{
                .srcStageMask = fromBarrierInfo.stageMask,
                .srcAccessMask = fromBarrierInfo.accessMask,
                .dstStageMask = toBarrierInfo.stageMask,
                .dstAccessMask = toBarrierInfo.accessMask,
                .srcQueueFamilyIndex = vk::QueueFamilyIgnored,
                .dstQueueFamilyIndex = vk::QueueFamilyIgnored,
                .buffer = buffer_->getBuffer(),
                .offset = offset_,
                .size = size_,
            };
        }

       private:
        ResourceRef<Buffer> buffer_;
        ResourceState from_, to_;
        uint64_t offset_, size_;
    };

    struct CommandBufferInheritanceInfo{
        std::span<vk::Format> colorAttachmentFormats;
        std::optional<vk::Format> depthAttachmentFormat;
        std::optional<vk::Format> stencilAttachmentFormat;
    };

    /// @class CommandBuffer
    /// Wrapper class around vulkan command buffer
    class CommandBuffer {
       public:
        /// Create the command buffer
        /// @param device The device to create the command buffer on
        /// @param queueFamily The queue family to use for the command buffer
        /// @param level The level of the command buffer (primary or secondary)
        CommandBuffer(CommandPool& pool, CommandBufferLevel level = CommandBufferLevel::Primary);

        ~CommandBuffer();

        /// @brief Set a semaphore that needs to be signaled before the command buffer starts
        /// Execution will wait on all wait semaphores in their corresponding stages
        auto addWaitSemaphore(ResourceRef<Semaphore> semaphore, PipelineStage waitStage,
            std::optional<uint64_t> waitValue = std::nullopt) -> void;

        /// @brief Set a semaphore that will be signaled after the command buffer finishes
        auto addSignalSemaphore(
            ResourceRef<Semaphore> semaphore, std::optional<uint64_t> signalValue = std::nullopt) -> void;

        /// @brief begin command buffer recording
        auto begin() -> void;

        /// @brief begin secondary command buffer recording with inheritance info
        auto begin(CommandBufferInheritanceInfo&& inheritance) -> void;

        /// @brief Call this function when you only need to END the command buffer
        /// @note To both end and submit call `submit()`
        auto end() -> void;

        /// @brief This function ENDS and SUBMITS the command buffer
        auto submit(vk::Fence fence = vk::Fence{}) -> void;

        /// @brief Get vulkan command buffer handle
        auto getCommandBuffer() const -> vk::CommandBuffer { return commandBuffer_; }

        /// @brief Starts dynamic rendering into provided target images
        auto beginRendering(vk::Rect2D renderArea, std::span<vk::RenderingAttachmentInfo> colorAttachments,
            std::optional<vk::RenderingAttachmentInfo> depthAttachment = std::nullopt,
            std::optional<vk::RenderingAttachmentInfo> stencilAttachment = std::nullopt,
            uint32_t layerCount = 1) -> void;

        /// @brief Starts dynamic rendering into provided target images
        auto beginRendering(vk::Rect2D renderArea, std::span<ResourceRef<Image>> colorAttachments,
            std::span<vk::ImageLayout> colorAttachmentLayouts,
            std::optional<ResourceRef<Image>> depthAttachment = std::nullopt,
            std::optional<vk::ImageLayout> depthAttachmentLayout = std::nullopt,
            std::optional<ResourceRef<Image>> stencilAttachment = std::nullopt,
            std::optional<vk::ImageLayout> stencilAttachmentLayout = std::nullopt, uint32_t layerCount = 1)
            -> void;

        /// @brief End dynamic rendering
        auto endRendering() -> void;

        /// @brief Copies contents of an image into a buffer
        auto copyImageToBuffer(ResourceRef<Image> src, ResourceRef<Buffer> dst, vk::Extent3D extent,
            uint32_t mipLevel = 0, uint32_t baseArrayLayer = 0, uint32_t layerCount = 1,
            vk::Offset3D offset = {0, 0, 0}) -> void;

        /// @brief Copies contents of one buffer to another buffer
        auto copyBufferToBuffer(ResourceRef<Buffer> src, ResourceRef<Buffer> dst,
            vk::DeviceSize srcOffset = 0, vk::DeviceSize dstOffset = 0, vk::DeviceSize size = vk::WholeSize)
            -> void;

        /// @brief Copies contents of a buffer into an image
        auto copyBufferToImage(ResourceRef<Buffer> src, ResourceRef<Image> dst, vk::DeviceSize srcOffset = 0,
            vk::Offset3D dstOffset = {0, 0, 0}, uint32_t mipLevel = 0) -> void;

        /// @brief Copies content of an image into another image
        auto copyImageToImage(ResourceRef<Image> src, ResourceRef<Image> dst,
            vk::Offset3D srcOffset = {0, 0, 0}, vk::Offset3D dstOffset = {0, 0, 0}) -> void;

        auto pipelineBarrier(std::span<ImageMemoryBarrier> images, std::span<BufferMemoryBarrier> buffers)
            -> void;

        /// @brief Binds a pipeline
        auto bindPipeline(ResourceRef<Pipeline> pipeline) -> void;

        /// @brief Binds vertex buffers
        auto bindVertexBuffers(std::span<ResourceRef<Buffer>> buffers, std::span<vk::DeviceSize> offsets)
            -> void;

        /// @brief Binds descriptor sets
        auto bindDescriptorSets(vk::PipelineBindPoint bindPoint, ResourceRef<Pipeline> pipeline,
            std::span<DescriptorSet> sets) -> void;

        /// @brief Set viewport
        auto setViewport(vk::Viewport viewport) -> void;

        /// @brief Set scissor
        auto setScissor(vk::Rect2D scissor) -> void;

        /// @brief Push constants
        auto pushConstants(ResourceRef<Pipeline> pipeline, vk::ShaderStageFlags stages, uint32_t offset,
            uint32_t size, const void* data) -> void;

       private:
        Device& device_;
        CommandPool& pool_;
        CommandQueueFamily family_;
        vk::CommandBuffer commandBuffer_;
        std::vector<vk::SemaphoreSubmitInfo> waitSemaphoreSubmitInfos_;
        std::vector<vk::SemaphoreSubmitInfo> signalSemaphoreSubmitInfos_;
        bool inProgress_{false};
        CommandBufferLevel level_{CommandBufferLevel::Primary};
    };
}  // namespace hammock::core
