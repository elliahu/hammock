#pragma once
#include <cstdint>
#include <unordered_map>
#include <vector>
#include <vulkan/vulkan.hpp>

#include "device.hpp"
#include "render_graph.hpp"
#include "render_pass.hpp"

namespace hammock::graph {

    // Compiled output types

    /// @brief A logical resource after compilation
    struct CompiledLogicalResource final {
        LogicalResourceHandle origin;       // Original handle. For debugging and identification
        LogicalResourceInterface resource;  // Resource that is being accessed by the pass
    };

    /// @brief A GPU pass after compilation, ready for execution.
    struct CompiledRenderPass final {
        RenderPassHandle origin;               // Original handle. For debugging and identification
        core::CommandQueueFamily family;       // family of the queue this pass executes on
        RenderPassExecFunc execFunc{nullptr};  // execution function for this pass
        std::vector<uint32_t> resources{};     // indices of resources accessed by this pass
        std::vector<uint32_t> dependencies{};  // indices of passes this pass depends on

        // Barriers need to be emitted
        struct {
            std::vector<vk::ImageMemoryBarrier2> image{};    // image barriers
            std::vector<vk::BufferMemoryBarrier2> buffer{};  // buffer barriers
        } barriers;
    };

    /// @brief Full output of the compiler.
    struct CompiledRenderGraph final {
        std::vector<CompiledLogicalResource> resources{};      // Compiled resources
        std::vector<CompiledRenderPass> passes{};              // Compiled passes
        std::vector<std::vector<uint32_t>> levels{};  // Execution levels. Indices into of passes.
        int32_t root = -1;  // Index of the root pass (present pass, last pass)
    };

    /// @class RenderGraphCompiler
    /// @brief Compiled dependencies between render passes based on the resource usage
    class RenderGraphCompiler final {
       public:
        explicit RenderGraphCompiler();

        /// Compile a DependencyGraph into a CompiledDependencyGraph.
        /// The result is self-contained and can be passed directly to the executor.
        [[nodiscard]] CompiledRenderGraph compile(RenderGraph& rg);

       private:
        // ----- types ---------------------------------------------------------

        enum class ReadWriteIntent { Read, Write, ReadWrite };

        /// One entry per (pass, access) pair recorded for a logical resource.
        struct ResourceUseEntry {
            uint32_t passIdx;
            AccessInterface access;
        };

        /// Internal adjacency-list node used during graph analysis.
        struct PassNode {
            RenderPass* pass = nullptr;
            std::vector<uint32_t> outgoing{};
            std::vector<uint32_t> incoming{};
            uint32_t indegree = 0;
        };

        // ----- data ----------------------------------------------------------

        std::vector<PassNode> nodes_;

        /// resource handle -> ordered list of (passIdx, access) pairs.
        /// Ordering is determined by data-flow (writes before reads), NOT declaration order.
        std::unordered_map<LogicalResourceHandle, std::vector<ResourceUseEntry>, LogicalResourceHandleHash>
            resourceUses_;

        CompiledRenderGraph result_;

        // Declaration
        void callDeclFuncs(RenderGraph& dg);

        // ----- graph analysis ------------------------------------------------

        /// Populate nodes_ and resourceUses_ from the dependency graph.
        void buildNodes(RenderGraph& dg);

        /// Add a directed edge src->dst (idempotent).
        void addEdge(uint32_t src, uint32_t dst);

        /// Apply explicit (user-declared) execution and debug dependencies.
        void applyExplicitDependencies(RenderGraph& dg);

        /// For each shared resource, insert edges between all writer->reader and
        /// writer->writer pairs, regardless of declaration order.
        void buildDataFlowEdges();

        /// Validate that every debug-asserted edge actually exists.
        void assertDebugEdges(RenderGraph& dg) const;

        /// Topological sort into execution levels (Kahn's algorithm).
        void buildExecutionLevels();

        // ----- resource compilation ------------------------------------------

        void compileResources(RenderGraph& dg);

        // ----- pass & barrier compilation ------------------------------------

        void compileRenderPasses(RenderGraph& dg);

        /// Emit a barrier between prev and curr accesses of the same resource.
        void emitBarrier(
            const LogicalResourceAccess& prev, const LogicalResourceAccess& curr, CompiledRenderPass& dst);

        /// Emit an initialisation barrier for a resource that has no prior access
        /// (UNDEFINED -> first-use layout / buffer acquire).
        void emitInitBarrier(const LogicalResourceAccess& firstUse, CompiledRenderPass& dst);

        // ----- access helpers (easy to extend) -------------------------------

        static ReadWriteIntent intentOf(AccessInterface a);
        static bool isHazard(ReadWriteIntent a, ReadWriteIntent b);
        static vk::PipelineStageFlags2 stageOf(AccessInterface a);
        static vk::AccessFlags2 accessOf(AccessInterface a);
        static vk::ImageLayout layoutOf(ImageAccess a);
    };

}  // namespace hammock::graph
