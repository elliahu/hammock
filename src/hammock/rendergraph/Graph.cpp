#include "hammock/rendergraph/Graph.h"
#include "hammock/core/Types.h"
#include "hammock/rendergraph/Resource.h"
#include <memory>

Hammock::Rendergraph::Graph::Graph(const CreateInfo& createInfo) : rm{createInfo.resourceManager} {}

auto Hammock::Rendergraph::Graph::sortTopologically() -> std::expected<void, std::string> {
    // Create list of all nodes
    std::vector<uint32_hash_t> nodes{};
    nodes.reserve(resources.size() + passes.size());

    for (const auto& pair : resources) {
        nodes.push_back(pair.first);
    }

    for (const auto& pair : passes) {
        nodes.push_back(pair.first);
    }

    // Create adjacency map and indegree map
    std::unordered_map<uint32_hash_t, std::vector<uint32_hash_t>> adj{};
    std::unordered_map<uint32_hash_t, uint32_t> indegree{};

    for (const auto& node : nodes) {
        adj[node];
        indegree[node] = 0u;
    }

    for (const auto& edge : edges) {
        adj[edge.srcHashName].push_back(edge.dstHashName);
        indegree[edge.dstHashName] += 1;
    }

    // Initialize queue with nodes that have no incoming edges
    std::queue<uint32_hash_t> queue{};

    for (const auto& node : nodes) {
        if (indegree[node] == 0) {
            queue.push(node);
        }
    }

    // Initialize the sortedNodes list
    sortedNodes.clear();

    while (!queue.empty()) {
        auto node = queue.front();
        queue.pop();

        sortedNodes.push_back(node);

        for (const auto& neighbor : adj[node]) {
            indegree[neighbor] -= 1;
            if (indegree[neighbor] == 0) {
                queue.push(neighbor);
            }
        }
    }

    // Detect cycles
    if (sortedNodes.size() != nodes.size()) {
        return std::unexpected("Cycle detected in render graph");
    }

    return {};  // Success
}

auto Hammock::Rendergraph::Graph::buildEdges() -> std::expected<void, std::string> {
    // Look for resources access by each pass and create edges
    // Build the visited map of resources to purge unused resources from the graph
    std::vector<uint32_hash_t> visitedResources;
    for (const auto& [passHashName, pass] : passes) {
        // Resource reads are incoming edges
        for (const auto& resourceHashName : pass->to) {
            const auto expectedResource = findResourceByName(resourceHashName);
            if (!expectedResource) {
                return std::unexpected("Failed to find the resource with hash " + std::to_string(resourceHashName));
            }

            // Retrieve the value
            auto resource = expectedResource.value();

            // Add to visited resources
            visitedResources.push_back(resource->getHashName());

            // Create the edge
            edges.push_back(Edge{
                .srcHashName = resource->getHashName(),
                .dstHashName = pass->getHashName(),
                .type = Edge::Type::ResourceToPass,  // Pass is reading the resource R -> P
            });

            pass->incomingEdges.push_back(edges.size() - 1);
        }

        // Resource writes are outgoing edges
        for (const auto& resourceHashName : pass->from) {
            const auto expectedResource = findResourceByName(resourceHashName);
            if (!expectedResource) {
                return std::unexpected("Failed to find the resource with hash" + std::to_string(resourceHashName));
            }

            // Retrieve the value
            auto resource = expectedResource.value();

            // Add to visited resources
            visitedResources.push_back(resource->getHashName());

            // Create the edge
            edges.push_back(Edge{
                .srcHashName = pass->getHashName(),
                .dstHashName = resource->getHashName(),
                .type = Edge::Type::PassToResource,  // Pass is writing the resource P -> R
            });

            pass->outgoingEdges.push_back(edges.size() - 1);
        }
    }

    // Clear unvisited resources
    std::vector<uint32_hash_t> toBeDeletedResource;
    for (const auto& [resourceHashName, resource] : resources) {
        if (std::ranges::find(visitedResources, resourceHashName) == visitedResources.end()) {
            toBeDeletedResource.push_back(resourceHashName);
        }
    }

    for (const uint32_hash_t& resourceHashName : toBeDeletedResource) {
        resources.erase(resourceHashName);
    }

    // TODO prove that some passes do not contribute to final result and erase those passes

    return {};  // Success
}

auto Hammock::Rendergraph::Graph::findResourceByName(uint32_hash_t name)
    -> std::expected<std::shared_ptr<Resource>, std::string> {
    auto it = resources.find(name);
    if (it != resources.end()) {
        return it->second;
    } else {
        return std::unexpected("Resource with hash " + std::to_string(name) + " not found in render graph.");
    }
}

auto Hammock::Rendergraph::Graph::execute() -> void {
    for (auto& pass : passes) {
        
    }
}

auto Hammock::Rendergraph::Graph::build() -> std::expected<void, std::string> {
    // Build the edges
    if (const auto result = buildEdges(); !result) {
        return result;
    }

    // Sort graph topologically
    if (const auto result = sortTopologically(); !result) {
        return result;
    }

    return {};  // Success
}

auto Hammock::Rendergraph::Graph::useSwapChainImageResolver(SwapChainImageResolver resolver) -> uint32_hash_t {
    swapChainImageResolver = std::move(resolver);

    uint32_hash_t hash = HashName(SWAP_CHAIN_IMAGE_RESOURCE_NAME);

    // Create a logical resource that represents a swap chain image
    auto swapImage = std::make_shared<SwapChainImageResource>(hash);

    // Add to resources
    resources.insert({hash, std::move(swapImage)});

    return hash;
}

