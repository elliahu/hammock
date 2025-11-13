#pragma once
#include <algorithm>
#include <vector>

#include "hammock/core/CoreUtils.h"
#include "hammock/core/core.h"
#include "hammock/rendergraph/Edge.h"


namespace Hammock {
    namespace Rendergraph {

        class Node;

        typedef Node* NodePtr;

        enum class NodeFlag : int { SwapChainContributing };

        /// This interface represents a node in the graph.
        /// It allows us to operate on all nodes like regardless of their nature.
        /// Using this interface we can perform all kind of graph operations easily without the need of
        /// additional context.
        class Node {
           protected:
            std::vector<NodeFlag> flags{};
            uint32_hash_t hashName;
            std::string name;

            Node(std::string name) : hashName(HashName(name.c_str())), name(name) {}

            std::vector<EdgePtr> incomingEdges;  // indices of edges coming into this node
            std::vector<EdgePtr> outgoingEdges;  // indices of edges going out from this node

           public:
            [[nodiscard]] auto getHashName() -> uint32_hash_t const { return hashName; }
            [[nodiscard]] auto getName() -> std::string { return name; }
            [[nodiscard]] auto getIncomingEdges() -> std::vector<EdgePtr>& { return incomingEdges; }
            [[nodiscard]] auto getOutgoingEdges() -> std::vector<EdgePtr>& { return outgoingEdges; }

            auto setFlag(NodeFlag flag) { flags.push_back(flag); }
            auto hasFlag(NodeFlag flag) -> bool {
                return std::find(flags.begin(), flags.end(), flag) != flags.end();
            }

            auto flagsToString() -> std::string {
                std::string output = "[";

                for (auto flag : flags) {
                    switch (flag) {
                        case NodeFlag::SwapChainContributing:
                            output += "SwapChainContributing";
                            break;
                        default:
                            break;
                    }
                }

                output += "]";
                return output;
            }
        };
    }  // namespace Rendergraph
}  // namespace Hammock