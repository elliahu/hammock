#pragma once
#include <cstdint>
#include <unordered_map>
#include <vector>
#include <vulkan/vulkan.hpp>

#include "dependency_graph.hpp"
#include "device.hpp"
#include "gpu_task.hpp"

namespace hammock::graph {

    // Compiled output types

    /// @brief A logical resource after compilation
    struct CompiledLogicalResource final {
        LogicalResourceHandle origin; // For debugging and identification
        LogicalResourceInterface resource;
    };

    /// @brief A GPU task after compilation, ready for execution.
    struct CompiledTask final {
        TaskHandle origin; // For debugging and identification
        core::CommandQueueFamily family;
        TaskExecutionFunction execFunc{nullptr};

        /// Indices into CompiledDependencyGraph::compiledLogicalResources accessed by this task.
        std::vector<uint32_t> compiledResourceAccesses{};

        /// Barriers to insert *before* this task runs (includes UNDEFINED->layout init barriers).
        std::vector<vk::ImageMemoryBarrier2> imageBarriers{};
        std::vector<vk::BufferMemoryBarrier2> bufferBarriers{};
    };

    /// @brief Full output of the compiler.
    struct CompiledDependencyGraph final {
        std::vector<CompiledLogicalResource> compiledLogicalResources{};
        std::vector<CompiledTask> compiledTasks{};
        /// Topological levels: tasks within the same level may run in parallel.
        std::vector<std::vector<uint32_t>> executionLevels{};
        /// Index of the present / root task (-1 if not yet set).
        int32_t rootIdx = -1;
    };

    // Compiler

    class DependencyGraphCompiler final {
       public:
        explicit DependencyGraphCompiler();

        /// Compile a DependencyGraph into a CompiledDependencyGraph.
        /// The result is self-contained and can be passed directly to the executor.
        [[nodiscard]] CompiledDependencyGraph compile(DependencyGraph& dependencyGraph);

       private:
        // ----- types ---------------------------------------------------------

        enum class ReadWriteIntent { Read, Write, ReadWrite };

        /// One entry per (task, access) pair recorded for a logical resource.
        struct ResourceUseEntry {
            uint32_t taskIdx;
            AccessInterface access;
        };

        /// Internal adjacency-list node used during graph analysis.
        struct TaskNode {
            GpuTask* task = nullptr;
            std::vector<uint32_t> outgoing{};
            std::vector<uint32_t> incoming{};
            uint32_t indegree = 0;
        };

        // ----- data ----------------------------------------------------------

        std::vector<TaskNode> nodes_;

        /// resource handle -> ordered list of (taskIdx, access) pairs.
        /// Ordering is determined by data-flow (writes before reads), NOT declaration order.
        std::unordered_map<LogicalResourceHandle, std::vector<ResourceUseEntry>, LogicalResourceHandleHash>
            resourceUses_;

        CompiledDependencyGraph result_;

        // ----- graph analysis ------------------------------------------------

        /// Populate nodes_ and resourceUses_ from the dependency graph.
        void buildNodes(DependencyGraph& dg);

        /// Add a directed edge src->dst (idempotent).
        void addEdge(uint32_t src, uint32_t dst);

        /// Apply explicit (user-declared) execution and debug dependencies.
        void applyExplicitDependencies(DependencyGraph& dg);

        /// For each shared resource, insert edges between all writer->reader and
        /// writer->writer pairs, regardless of declaration order.
        void buildDataFlowEdges();

        /// Validate that every debug-asserted edge actually exists.
        void assertDebugEdges(DependencyGraph& dg) const;

        /// Topological sort into execution levels (Kahn's algorithm).
        void buildExecutionLevels();

        // ----- resource compilation ------------------------------------------

        void compileResources(DependencyGraph& dg);

        // ----- task & barrier compilation ------------------------------------

        void compileTasks(DependencyGraph& dg);

        /// Emit a barrier between prev and curr accesses of the same resource.
        void emitBarrier(
            const LogicalResourceAccess& prev, const LogicalResourceAccess& curr, CompiledTask& dst);

        /// Emit an initialisation barrier for a resource that has no prior access
        /// (UNDEFINED -> first-use layout / buffer acquire).
        void emitInitBarrier(const LogicalResourceAccess& firstUse, CompiledTask& dst);

        // ----- access helpers (easy to extend) -------------------------------

        static ReadWriteIntent intentOf(AccessInterface a);
        static bool isHazard(ReadWriteIntent a, ReadWriteIntent b);
        static vk::PipelineStageFlags2 stageOf(AccessInterface a);
        static vk::AccessFlags2 accessOf(AccessInterface a);
        static vk::ImageLayout layoutOf(ImageAccess a);
    };

}  // namespace hammock::graph
