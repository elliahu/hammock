#pragma once

#include "render_graph_compiler.hpp"

namespace hammock::graph {

    /// @class RenderGraphExecutor
    /// @brief Executes compiled graph, orchestrates the recording and submission of command buffers
    class RenderGraphExecutor final {
    public:
        /// @brief Executes the compiled dependency graph
        inline void execute(const CompiledRenderGraph& compiledGraph){
            for(auto execLevel: compiledGraph.executionLevels){
                for(auto gidx : execLevel){
                    if(compiledGraph.compiledRenderPasses[gidx].execFunc){
                        //compiledGraph.compiledTasks[gidx].execFunc();
                    }
                }
            }
        }
    };


}
