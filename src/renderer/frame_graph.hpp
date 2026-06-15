#pragma once

#include <cstdint>
#include <functional>
#include <optional>
#include <unordered_map>
#include <variant>
#include <vector>
#include <vulkan/vulkan.hpp>

#include "command_buffer.hpp"
#include "command_cache.hpp"
#include "device.hpp"
#include "image.hpp"
#include "resource_manager.hpp"
#include "semaphore.hpp"

namespace hammock::renderer {

    struct PassTransitionsInfo {
        std::vector<core::ImageMemoryBarrier> images;
        std::vector<core::BufferMemoryBarrier> buffers;
    };

    struct ImageAccessInfo {
        core::ResourceRef<core::Image> image;  // What image
        core::ResourceState state;             // What state is required
    };

    struct BufferAccessInfo {
        core::ResourceRef<core::Buffer> buffer;  // Buffer
        core::ResourceState state;               // What state is required
    };

    struct PassResourceAccessInfo {
        std::vector<ImageAccessInfo> images;    // Image accesses
        std::vector<BufferAccessInfo> buffers;  // Buffer accesses
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

    struct PassSyncSemaphoreInfo {
        uint64_t value;
        core::CommandQueueFamily queue;
        core::PipelineStage stage;
    };

    struct PassSyncInfo {
        std::vector<PassSyncSemaphoreInfo> waits;
        std::vector<PassSyncSemaphoreInfo> signals;
    };

    struct FrameGraphPassDeclarationWrapper;

    struct PassDependencyInfo {
        std::vector<core::Handle<FrameGraphPassDeclarationWrapper>> passes;
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

    /// @struct User definition of a graphics pass
    struct GraphicsPassDeclaration {
        std::string name;
        PassResourceAccessInfo resources;
        GraphicsPassRenderingInfo rendering;
        PassDataInfo data;
        PassDependencyInfo dependency;
        std::function<void(core::ResourceRef<core::CommandBuffer>)> execute;
    };

    /// Internal struct with deduced values
    struct GraphicsPassDescription {
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

    /// @struct User definition of a compute pass
    struct ComputePassDeclaration {
        std::string name;
        PassResourceAccessInfo resources;
        ComputePassDispatchInfo dispatch;
        PassDataInfo data;
        PassDependencyInfo dependency;
        std::function<void(core::ResourceRef<core::CommandBuffer>)> execute;
    };

    /// Internal struct with deduced values
    struct ComputePassDescription {
        std::string name;
        PassTransitionsInfo transitions;
        ComputePassDispatchInfo dispatch;
        PassDataInfo data;
        PassSyncInfo sync;
        std::function<void(core::ResourceRef<core::CommandBuffer>)> execute;
    };

    struct TransferPassTransferInfo {};

    struct TransferPassDeclaration {
        std::string name;
        PassResourceAccessInfo resources;
        TransferPassTransferInfo transfer;
        PassDataInfo data;
        PassDependencyInfo dependency;
        std::function<void(core::ResourceRef<core::CommandBuffer>)> execute;
    };

    struct TransferPassDescription {
        std::string name;
        PassTransitionsInfo transitions;
        PassDataInfo data;
        PassSyncInfo sync;
        std::function<void(core::ResourceRef<core::CommandBuffer>)> execute;
    };

    struct FrameGraphDesc {
        std::vector<std::variant<GraphicsPassDescription, ComputePassDescription, TransferPassDescription>>
            passes;
    };

    /// @class FrameGraph
    class FrameGraph {
       public:
        explicit FrameGraph(FrameGraphDesc&& desc, CommandCache& commandCache,
            core::ResourceRef<core::Semaphore> graphicsTimeline,
            core::ResourceRef<core::Semaphore> computeTimeline,
            core::ResourceRef<core::Semaphore> transferTimeline);

        void execute(uint32_t frameIndex = 0);

       private:
        void recordBarrier(PassTransitionsInfo& transitions, core::ResourceRef<core::CommandBuffer> cmd);
        void recordGraphicsPass(GraphicsPassDescription& pass, core::ResourceRef<core::CommandBuffer> cmd);
        void recordComputePass(ComputePassDescription& pass, core::ResourceRef<core::CommandBuffer> cmd);

        FrameGraphDesc desc_;
        CommandCache& commandCache_;

        struct {
            core::ResourceRef<core::Semaphore> graphics;
            core::ResourceRef<core::Semaphore> compute;
            core::ResourceRef<core::Semaphore> transfer;
        } timeline_;
    };

    /// @struct FrameGraphPassWrapper
    /// Internal type for storing pass descriptions
    struct FrameGraphPassDeclarationWrapper {
        // Description
        std::variant<GraphicsPassDeclaration, ComputePassDeclaration, TransferPassDeclaration> declaration;

        // Constructors
        FrameGraphPassDeclarationWrapper(GraphicsPassDeclaration&& graphics) : declaration(graphics) {}
        FrameGraphPassDeclarationWrapper(ComputePassDeclaration&& compute) : declaration(compute) {}
    };

    /// @class FrameGraphBuilder
    /// @brief Builds the frame graph descriptions
    class FrameGraphBuilder {
       public:
        /// Internal enum for DFS algorith
        enum class NodeVisitState { Unvisited, Visiting, Visited };

        /// Creates a graphics pass
        core::Handle<FrameGraphPassDeclarationWrapper> createGraphicsPass(
            GraphicsPassDeclaration&& graphicsDesc);
        /// Creates a compute pass
        core::Handle<FrameGraphPassDeclarationWrapper> createComputePass(
            ComputePassDeclaration&& computeDesc);

        FrameGraphDesc build();

       private:
        GraphicsPassDescription buildGraphicsPassDescription(GraphicsPassDeclaration& decl);
        ComputePassDescription buildComputePassDescription(ComputePassDeclaration& decl);
        void buildSyncValues(
            FrameGraphDesc& desc, std::vector<core::Handle<FrameGraphPassDeclarationWrapper>>& sorted);

        PassTransitionsInfo deduceFromState(
            PassResourceAccessInfo& accesses, PassDependencyInfo& dependencies);

        /// Depth first search
        void dfs(core::Handle<FrameGraphPassDeclarationWrapper> node,
            std::unordered_map<core::Handle<FrameGraphPassDeclarationWrapper>, NodeVisitState>& state,
            std::vector<core::Handle<FrameGraphPassDeclarationWrapper>>& sorted);
        /// Performs topological sort of the passes using DFS
        void performTopoSort(std::vector<core::Handle<FrameGraphPassDeclarationWrapper>>& sorted);

        template <typename PassType>
        constexpr core::CommandQueueFamily getQueueFamily() {
            if constexpr (std::is_same_v<PassType, GraphicsPassDescription> ||
                          std::is_same_v<PassType, GraphicsPassDeclaration>) {
                return core::CommandQueueFamily::Graphics;
            } else if constexpr (std::is_same_v<PassType, ComputePassDescription> ||
                                 std::is_same_v<PassType, ComputePassDeclaration>) {
                return core::CommandQueueFamily::Compute;
            } else if constexpr (std::is_same_v<PassType, TransferPassDescription> ||
                                 std::is_same_v<PassType, TransferPassDeclaration>) {
                return core::CommandQueueFamily::Transfer;
            } else {
                return core::CommandQueueFamily::Ignored;
            }
        }

        core::ResourceManager<FrameGraphPassDeclarationWrapper> declarations_;
        std::vector<core::Handle<FrameGraphPassDeclarationWrapper>>
            handles_;  // stores the handles given by the builder
    };

}  // namespace hammock::renderer
