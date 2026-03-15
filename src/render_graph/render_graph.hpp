#pragma once
#include <cstdint>
#include <vector>

#include "render_pass.hpp"
#include "render_graph_types.hpp"


namespace hammock::graph {

    /// @class RenderGraph
    /// @brief Represents DAG of tasks and resources
    class RenderGraph final {
        friend class RenderGraphCompiler;
        friend struct CompiledLogicalResource;

        struct RenderPassSlot {
            RenderPass pass;
            std::uint32_t generation;
            std::uint32_t nextFreeSlot;
            bool active = false;
        };

        std::vector<RenderPassSlot> passes_;
        std::int32_t firstFreePassSlot_ = -1;

        struct Dependency {
            RenderPassHandle srcPassHandle;
            RenderPassHandle dstPassHandle;
            DependencyType dependencyType;
        };

        std::vector<Dependency> explicitDependencies_;

        struct LogicalResourceSlot {
            LogicalResourceInterface resource;
            std::uint32_t generation;
            std::uint32_t nextFreeSlot;
            bool active = false;
        };

        std::vector<LogicalResourceSlot> logicalResources_;
        std::int32_t firstFreeLogicalResourceSlot_ = -1;

        RenderPassHandle root_;
        bool hasRoot_{false};

       public:
        /// @brief Adds a resource to the graph
        [[nodiscard]] LogicalResourceHandle resource(LogicalResourceInterface riface);

        /// @brief Adds an image to the graph
        /// @note Wrapper around addResource
        [[nodiscard]] LogicalResourceHandle image(LogicalImageResource imageResource);

        /// @brief Adds a buffer to the graph
        /// @note Wrapper around addResource
        [[nodiscard]] LogicalResourceHandle buffer(LogicalBufferResource bufferResource);

        /// @brief Returns true if a handle is valid
        [[nodiscard]] bool isHandleValid(RenderPassHandle h) const;

        /// @brief Adds a gpu pass to the graph
        [[nodiscard]] RenderPassHandle pass(RenderPass&& pass);

        /// @brief Removes a pass from the graph
        void remove(RenderPassHandle h);

        /// @brief Removes a resource from the graph
        void remove(LogicalResourceHandle h);

        /// @brief Declares an explicit execution dependency between two pass.
        void dependency(RenderPassHandle srcPassHandle, RenderPassHandle dstPassHandle, DependencyType dependencyType);

        /// @brief Marks this pass as root (last pass)
        /// Passes that do not contribute to the root pass might get culled out during compilation.
        void root(RenderPassHandle pass);
    };
}  // namespace hammock::graph
