#pragma once
#include <algorithm>
#include <optional>
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
    /// @class CommandBuffer
    /// Wrapper class around vulkan command buffer
    class CommandBuffer {
       public:
        CommandBuffer(Device& device, CommandQueueFamily queueFamily);

        ~CommandBuffer();

        /// @brief Set a semaphore that needs to be signaled before the command buffer starts
        /// Execution will wait on all wait semaphores in their corresponding stages
        auto addWaitSemaphore(ResourceRef<Semaphore> semaphore, vk::PipelineStageFlagBits2 stageFlagBits)
            -> void;

        /// @brief Set a semaphore that will be signaled after the command buffer finishes
        auto addSignalSemaphore(ResourceRef<Semaphore> semaphore) -> void;

        /// @brief begin command buffer recording
        auto begin() -> void;

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
        auto beginRendering(vk::Rect2D renderArea, std::span<ResourceRef<Image>> colorAttachments, std::span<vk::ImageLayout> colorAttachmentLayouts,
            std::optional<ResourceRef<Image>> depthAttachment = std::nullopt, std::optional<vk::ImageLayout> depthAttachmentLayout = std::nullopt,
            std::optional<ResourceRef<Image>> stencilAttachment = std::nullopt, std::optional<vk::ImageLayout> stencilAttachmentLayout = std::nullopt,
            uint32_t layerCount = 1)
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

        /// @brief Records a pipeline barrier on an image
        auto imagePipelineBarrier(ResourceRef<Image> image, vk::PipelineStageFlags2 srcStageMask,
            vk::AccessFlags2 srcAccessMask, vk::PipelineStageFlags2 dstStageMask,
            vk::AccessFlags2 dstAccessMask, vk::ImageLayout oldLayout, vk::ImageLayout newLayout) -> void;

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
        CommandQueueFamily queueFamily_;
        vk::CommandBuffer commandBuffer_;
        std::vector<vk::SemaphoreSubmitInfo> waitSemaphoreSubmitInfos_;
        std::vector<vk::SemaphoreSubmitInfo> signalSemaphoreSubmitInfos_;
        bool inProgress_{false};
    };
}  // namespace hammock::core
