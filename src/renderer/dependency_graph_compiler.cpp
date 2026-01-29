#include <cstdint>
#include <memory>
#include <stdexcept>
#include <variant>
#include <vector>
#include <vulkan/vulkan.hpp>

#include "dependency_graph_compiler.hpp"
#include "dependency_graph.hpp"
#include "hammock_core.hpp"



namespace hammock::renderer {
    void DependencyGraphCompiler::buildGraphNodes(DependencyGraph& dependencyGraph) {
        nodes_.resize(dependencyGraph.tasks_.size());
        for (auto& dependency : dependencyGraph.tasks_) {
            
        }
    }
}  // namespace hammock::renderer
