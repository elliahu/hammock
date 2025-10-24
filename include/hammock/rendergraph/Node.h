#pragma once
#include <vector>

#include "hammock/core/core.h"

namespace Hammock {
    namespace Rendergraph {

        class Node {
           protected:
            uint32_hash_t hashName;

           public:

           Node(uint32_hash_t hashName) : hashName(hashName){}

            std::vector<uint32_hash_t> to;        // If resource - pass that write to this resource, If pass - resource that this pass reads from
            std::vector<uint32_hash_t> from;      // If resource - Pass that reads from this resource, If pass - resource
            std::vector<uint32_t> incomingEdges;  // indices of edges coming into this node
            std::vector<uint32_t> outgoingEdges;  // indices of edges going out from this node

            [[nodiscard]] uint32_hash_t getHashName() const {return hashName;}
        };
    }  // namespace Rendergraph
}  // namespace Hammock