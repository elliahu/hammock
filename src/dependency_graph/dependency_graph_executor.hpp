#pragma once

#include "dependency_graph_compiler.hpp"

namespace hammock::renderer {

    /// @class DependencyGraphExecutor
    /// @brief Executes compiled graph, orchestrates the recording and submission of command buffers
    class DependencyGraphExecutor final {
    public:
        /// @brief Executes the compiled dependency graph
        void execute(const CompiledDependencyGraph& compiledGraph);
    };


}