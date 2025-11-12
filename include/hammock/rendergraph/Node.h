#pragma once
#include <vector>

#include "hammock/core/core.h"

namespace Hammock {
    namespace Rendergraph {

        /// This interface represents a node in the graph.
        /// It allows us to operate on all nodes like regardless of their nature.
        /// Using this interface we can perform all kind of graph operations easily without the need of
        /// additional context.
        class Node {
           protected:
            uint32_hash_t hashName;

            Node(uint32_hash_t hashName) : hashName(hashName) {}

            std::vector<uint32_hash_t> to;  // If resource - pass that write to this resource, If pass -
                                            // resource that this pass reads from
            std::vector<uint32_hash_t>
                from;  // If resource - Pass that reads from this resource, If pass - resource
            std::vector<uint32_t> incomingEdges;  // indices of edges coming into this node
            std::vector<uint32_t> outgoingEdges;  // indices of edges going out from this node

           public:
            // GET
            [[nodiscard]] auto getHashName() -> uint32_hash_t const { return hashName; }
            [[nodiscard]] auto getTo() -> std::vector<uint32_hash_t>& { return to; }
            [[nodiscard]] auto getFrom() -> std::vector<uint32_hash_t>& { return from; }
            [[nodiscard]] auto getIncomingEdges() -> std::vector<uint32_hash_t>& { return incomingEdges; }
            [[nodiscard]] auto getOutgoingEdges() -> std::vector<uint32_hash_t>& { return outgoingEdges; }
            // SET
        };
    }  // namespace Rendergraph
}  // namespace Hammock