#include "hammock/rendergraph/Graph.h"

#include <expected>
#include <functional>
#include <memory>
#include <string>

#include "hammock/core/CommandBuffer.h"
#include "hammock/core/Device.h"
#include "hammock/core/SwapChain.h"
#include "hammock/core/Types.h"
#include "hammock/rendergraph/ExecutionContext.h"
#include "hammock/rendergraph/Pass.h"
#include "hammock/rendergraph/Resource.h"
#include "hammock/resources/Descriptors.h"

Hammock::Rendergraph::Graph::Graph(const CreateInfo& createInfo)
    : rm{createInfo.resourceManager}, device(createInfo.device) {}

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
                return std::unexpected(
                    "Failed to find the resource with hash " + std::to_string(resourceHashName));
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
                return std::unexpected(
                    "Failed to find the resource with hash" + std::to_string(resourceHashName));
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

auto Hammock::Rendergraph::Graph::getCommandBufferForPass(uint32_t hash)
    -> std::expected<CommandBuffer, std::string> {
    if (!isNodePass(hash))
        return std::unexpected("Resource with hash " + std::to_string(hash) + " is not a pass");

    if (commandBuffers.contains(hash)) {
        return commandBuffers.at(hash);
    }

    return std::unexpected(
        "Could not find command buffer for pass " + std::to_string(hash) + ". This should not happen!");
}

auto Hammock::Rendergraph::Graph::execute() -> void {
    forEachPass([this](PassPtr pass) {
        // Create execution context
        ExecutionContext context{};

        // Pass type specific actions
        if (pass->getType() == PassType::Graphics) {
            if (auto* graphics = dynamic_cast<GraphicsPass*>(pass)) {
            }
        } else if (pass->getType() == PassType::Compute) {
            if (auto* compute = dynamic_cast<ComputePass*>(pass)) {
            }
        } else if (pass->getType() == PassType::Transfer) {
            if (auto* transfer = dynamic_cast<TransferPass*>(pass)) {
            }
        }

        // Call execution callback
        pass->execute(context);
    });
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

    // Create and asign command buffers
    if (const auto result = buildCommandBuffers(); !result) {
        return result;
    }

    return {};  // Success
}

auto Hammock::Rendergraph::Graph::useSwapChainImages(SwapChainImageResolver resolver) -> uint32_hash_t {
    swapChainImageResolver = std::move(resolver);

    uint32_hash_t hash = HashName(SWAP_CHAIN_IMAGE_RESOURCE_NAME);

    // Create a logical resource that represents a swap chain image
    auto swapImage = std::make_shared<SwapChainImageResource>(hash);

    // Add to resources
    resources.insert({hash, std::move(swapImage)});

    return hash;
}

auto Hammock::Rendergraph::Graph::buildCommandBuffers() -> std::expected<void, std::string> {
    return forEachPassExpected([this](PassPtr pass) -> std::expected<void, std::string> {
        VkCommandBuffer vkCmd;

        // Create vulkan command buffer
        if (pass->getType() == PassType::Graphics) {
            const auto result = device.createVulkanCommandBuffers<CommandQueueFamily::Graphics>(1);
            if (!result) return std::unexpected(result.error());
            vkCmd = result.value()[0];
        } else if (pass->getType() == PassType::Compute) {
            const auto result = device.createVulkanCommandBuffers<CommandQueueFamily::Compute>(1);
            if (!result) return std::unexpected(result.error());
            vkCmd = result.value()[0];
        } else if (pass->getType() == PassType::Transfer) {
            const auto result = device.createVulkanCommandBuffers<CommandQueueFamily::Transfer>(1);
            if (!result) return std::unexpected(result.error());
            vkCmd = result.value()[0];
        }

        // Create command buffer
        CommandBuffer commandBuffer(vkCmd);  // FIXME possible expensive copy
        commandBuffers.emplace(pass->getHashName(), commandBuffer);

        return {};
    });
}

auto Hammock::Rendergraph::Graph::buildGraphDescriptors() -> std::expected<void, std::string> {
    return forEachPassExpected([this](PassPtr pass) -> std::expected<void, std::string> {
        DescriptorSetsAndLayout setAndLayouts{};

        // Create layout
        auto builder = DescriptorSetLayout::Builder(device);

        for (const auto& binding : pass->bindings) {
            // Find the resource
            auto expectedResource = findResourceByName(binding.resource);
            if (!expectedResource) return std::unexpected(expectedResource.error());
            std::shared_ptr<Resource> resource = expectedResource.value();

            builder.addBinding(
                binding.binding, binding.descriptorType, binding.stageFlags, binding.count, binding.flags);
        }

        setAndLayouts.layout = std::move(builder.build());

        // Create sets
        SwapChain::forEachFrameInFlight([&, this](int frame) -> void {
           // auto writer = DescriptorWriter(setAndLayouts.layout->getDescriptorSetLayout(), )
        });

        return {};
    });
}

auto Hammock::Rendergraph::Graph::forEachPass(std::function<void(PassPtr)> cb) -> void {
    for (const auto& passHash : sortedNodes) {
        if (isNodePass(passHash)) {
            cb(passes[passHash].get());
        }
    }
};

auto Hammock::Rendergraph::Graph::forEachPassExpected(
    std::function<std::expected<void, std::string>(PassPtr)> cb) -> std::expected<void, std::string> {
    for (const auto& passHash : sortedNodes) {
        if (isNodePass(passHash)) {
            auto result = cb(passes[passHash].get());
            if (!result) return result;  // propagate error upward
        }
    }
    return {};
}

auto Hammock::Rendergraph::Graph::forEachResource(std::function<void(std::shared_ptr<Resource>)> cb) -> void {
    for (const auto& resourceHash : sortedNodes) {
        if (isNodeResource(resourceHash)) {
            auto expectedResource = findResourceByName(resourceHash);
            if (expectedResource) {
                std::shared_ptr<Resource> resource = expectedResource.value();
                cb(resource);
            }
        }
    }
};
auto Hammock::Rendergraph::Graph::forEachResourceExpected(
    std::function<std::expected<void, std::string>(std::shared_ptr<Resource>)> cb)
    -> std::expected<void, std::string> {
    for (const auto& resourceHash : sortedNodes) {
        if (isNodeResource(resourceHash)) {
            auto expectedResource = findResourceByName(resourceHash);
            if (expectedResource) {
                std::shared_ptr<Resource> resource = expectedResource.value();
                auto result = cb(resource);
                if (!result) return result;  // propagate error upward
            }
        }
    }
    return {};
};