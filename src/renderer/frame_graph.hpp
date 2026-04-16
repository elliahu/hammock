#pragma once

#include <cstdint>
#include <functional>
#include <optional>
#include <variant>
#include <vector>
#include <vulkan/vulkan.hpp>

#include "command_buffer.hpp"
#include "command_cache.hpp"
#include "image.hpp"
#include "resource_manager.hpp"
#include "semaphore.hpp"

namespace hammock::renderer {

    struct PassTransitionsInfo {
        std::vector<core::ImageMemoryBarrier> images;
        std::vector<core::BufferMemoryBarrier> buffers;
    };

    struct PushConstantsInfo {
        core::ShaderStage stages;
        uint32_t size;
        const void* data;
    };

    struct PassDataInfo {
        std::vector<core::DescriptorSet> descriptors;
        PushConstantsInfo constants;
    };

    struct PassSyncInfo {
        std::optional<core::ResourceRef<core::Semaphore>> semaphore;
        std::optional<uint64_t> wait;  // wait value
        core::PipelineStage waitStage = core::PipelineStage::Invalid;
        std::optional<uint64_t> signal;  // signal value
    };

    struct GraphicsPassRenderingInfo {
        std::vector<core::ResourceRef<core::Image>> colorAttachments;
        std::optional<core::ResourceRef<core::Image>> depthAttachment;
        std::optional<core::ResourceRef<core::Image>> stencilAttachment;
        std::optional<core::Rect2D> area;
        std::optional<core::Viewport> viewport;
        std::optional<core::Rect2D> scissors;
        std::optional<core::ResourceRef<core::Pipeline>> pipeline;
    };

    /// @struct GraphicsPassDesc
    struct GraphicsPassDesc {
        std::string name;
        PassTransitionsInfo transitions;
        GraphicsPassRenderingInfo rendering;
        PassDataInfo data;
        PassSyncInfo sync;
        std::function<void(core::ResourceRef<core::CommandBuffer>)> execute;
    };

    struct ComputePassDispatchGroupSizes {
        uint32_t x = 0, y = 0, z = 0;
    };

    struct ComputePassDispatchInfo {
        ComputePassDispatchGroupSizes sizes;
        std::optional<core::ResourceRef<core::Pipeline>> pipeline;
    };

    /// @struct ComputePassDesc
    struct ComputePassDesc {
        std::string name;
        PassTransitionsInfo transitions;
        ComputePassDispatchInfo dispatch;
        PassDataInfo data;
        PassSyncInfo sync;
        std::function<void(core::ResourceRef<core::CommandBuffer>)> execute;
    };

    struct TransferPassDesc {
        std::string name;
        PassTransitionsInfo transitions;
        PassDataInfo data;
        PassSyncInfo sync;
        std::function<void(core::ResourceRef<core::CommandBuffer>)> execute;
    };

    struct FrameGraphDesc {
        std::vector<std::variant<GraphicsPassDesc, ComputePassDesc, TransferPassDesc>> passes;
    };

    /// @class FrameGraph
    class FrameGraph {
       public:
        explicit FrameGraph(FrameGraphDesc&& desc, CommandCache& commandCache);

        void execute(uint32_t frameIndex = 0);

       private:
        void recordBarrier(PassTransitionsInfo& transitions, core::ResourceRef<core::CommandBuffer> cmd);
        void recordGraphicsPass(GraphicsPassDesc& pass, core::ResourceRef<core::CommandBuffer> cmd);
        void recordComputePass(ComputePassDesc& pass, core::ResourceRef<core::CommandBuffer> cmd);

        FrameGraphDesc desc_;
        CommandCache& commandCache_;
    };

    /// @class FrameGraphBuilder
    /// @brief Builds the frame graph descriptions
    class FrameGraphBuilder {
       public:
        /// @struct FrameGraphPassWrapper
        /// Internal type for storing pass descriptions
        struct FrameGraphPassWrapper {
            // Description
            std::variant<GraphicsPassDesc, ComputePassDesc, TransferPassDesc> desc;

            // Constructors
            FrameGraphPassWrapper(GraphicsPassDesc&& graphicsDesc) : desc(graphicsDesc) {}
            FrameGraphPassWrapper(ComputePassDesc&& computeDesc) : desc(computeDesc) {}
        };

        /// Creates a graphics pass
        core::Handle<FrameGraphPassWrapper> createGraphicsPass(GraphicsPassDesc &&graphicsDesc);
        /// Creates a compute pass
        core::Handle<FrameGraphPassWrapper> createComputePass(ComputePassDesc &&computeDesc);

        FrameGraphDesc build();

       private:
        core::ResourceManager<FrameGraphPassWrapper>
            passes_;  // Store the actuall pass descriptions in a wrapper object
        std::vector<core::Handle<FrameGraphPassWrapper>> handles_;  // stores the handles give by the builder
    };

}  // namespace hammock::renderer
