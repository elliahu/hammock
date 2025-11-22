#include "hammock/rendergraph/Graph.h"

#include <algorithm>
#include <exception>
#include <functional>
#include <memory>
#include <stdexcept>
#include <string>

#include "hammock/core/CommandBuffer.h"
#include "hammock/core/CoreUtils.h"
#include "hammock/core/Descriptors.h"
#include "hammock/core/Device.h"
#include "hammock/core/GraphicsPipeline.h"
#include "hammock/core/SwapChain.h"
#include "hammock/core/Types.h"
#include "hammock/rendergraph/Edge.h"
#include "hammock/rendergraph/ExecutionContext.h"
#include "hammock/rendergraph/Node.h"
#include "hammock/rendergraph/Pass.h"
#include "hammock/rendergraph/Resource.h"

Hammock::Rendergraph::Graph::Graph(Device& device) : device(device) {}

auto Hammock::Rendergraph::Graph::sortTopologically() -> void {
    // Create list of all nodes
    std::vector<uint32_hash_t> allNodes{};
    allNodes.reserve(resources.size() + passes.size());

    for (const auto& pair : resources) {
        allNodes.push_back(pair.first);
    }

    for (const auto& pair : passes) {
        allNodes.push_back(pair.first);
    }

    // Create adjacency map and indegree map
    for (const auto& node : allNodes) {
        adj[node];
        indegree[node] = 0u;
    }

    for (const auto& edge : edges) {
        adj[edge->srcHashName].push_back(edge->dstHashName);
        indegree[edge->dstHashName] += 1;
    }

    // Initialize queue with nodes that have no incoming edges
    std::queue<uint32_hash_t> queue{};

    for (const auto& node : allNodes) {
        if (indegree[node] == 0) {
            queue.push(node);
        }
    }

    // Initialize the sortedNodes list
    nodes.clear();

    while (!queue.empty()) {
        auto node = queue.front();
        queue.pop();

        nodes.push_back(node);

        for (const auto& neighbor : adj[node]) {
            indegree[neighbor] -= 1;
            if (indegree[neighbor] == 0) {
                queue.push(neighbor);
            }
        }
    }

    // Detect cycles
    if (nodes.size() != allNodes.size()) {
        throw std::runtime_error("Cycle detected in render graph");
    }

    // Create topo sorted vector of only passes
    forEachPass([this](PassPtr pass) { sortedPasses.push_back(pass->getHashName()); });

    // Create topo sorted vector of only resources
    forEachResource(
        [this](std::shared_ptr<Resource> resource) { sortedResources.push_back(resource->getHashName()); });
}

auto Hammock::Rendergraph::Graph::findResourceByName(uint32_hash_t name) -> std::shared_ptr<Resource> {
    auto it = resources.find(name);
    if (it != resources.end()) {
        return it->second;
    }
    throw std::runtime_error("Resource with hash " + std::to_string(name) + " not found in render graph.");
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
        }
    });
}

auto Hammock::Rendergraph::Graph::build() -> void {
    // Resolve dependencies
    try {
        resolveDependencies();
    } catch (const std::exception& error) {
        throw error;
    }

    // Sort graph topologically
    try {
        sortTopologically();
    } catch (const std::exception& error) {
        throw error;
    }

    try {
        analyze();
    } catch (const std::exception& error) {
        throw error;
    }
}

auto Hammock::Rendergraph::Graph::importSwapChainImage() -> uint32_hash_t {
    swapChainImageResolver = [] {
        auto& fm = FrameManager::getInstance();
        return Graph::SwapChainImage{
            .image = fm.getSwapChain()->getImage(fm.getSwapChainImageIndex()),
            .imageView = fm.getSwapChain()->getImageView(fm.getSwapChainImageIndex()),
            .format = fm.getSwapChain()->getSwapChainImageFormat(),
        };
    };

    uint32_hash_t hash = HashName(SWAP_CHAIN_IMAGE_RESOURCE_NAME);

    // Create a logical resource that represents a swap chain image
    auto swapImage = std::make_shared<SwapChainImageResource>(SWAP_CHAIN_IMAGE_RESOURCE_NAME);

    // Add to resources
    resources.insert({hash, std::move(swapImage)});

    return hash;
}

auto Hammock::Rendergraph::Graph::forEachPass(std::function<void(PassPtr)> cb) -> void {
    for (const auto& passHash : nodes) {
        if (isNodePass(passHash)) {
            cb(passes[passHash].get());
        }
    }
};

auto Hammock::Rendergraph::Graph::forEachResource(std::function<void(std::shared_ptr<Resource>)> cb) -> void {
    for (const auto& resourceHash : nodes) {
        if (isNodeResource(resourceHash)) {
            try {
                auto resource = findResourceByName(resourceHash);
                cb(resource);
            } catch (const std::exception& error) {
                throw error;
            }
        }
    }
};

auto Hammock::Rendergraph::Graph::resolveDependencies() -> void {
    // Look for resources access by each pass and create edges
    for (const auto& [passHashName, pass] : passes) {
        // Uniform buffers
        for (const auto& ub : pass->getUniformBuffers()) {
            const auto resource = findResourceByName(ub.buffer);

            BufferDependencyInfo dep{};

            // Create the edge
            auto edge = std::make_unique<Edge>();
            edge->srcHashName = resource->getHashName();
            edge->dstHashName = pass->getHashName();
            edge->type = Edge::Type::ResourceToPass;  // Pass is reading the resource R -> P
            edge->bufferDependency = dep;

            pass->getIncomingEdges().push_back(edge.get());

            edges.push_back(std::move(edge));
        }

        // Storage buffers
        for (const auto& sb : pass->getStorageBuffers()) {
            const auto resource = findResourceByName(sb.buffer);

            // Here we can both read and write so this would make a cycle (RenderGraph is strictly DAG)
            // To resolve this we consider this as a write access (even tho we can read as well)

            // Define the dependency
            BufferDependencyInfo dep{.stageFlags = sb.stageFlags};

            // Create the write edge
            auto edge = std::make_unique<Edge>();
            edge->srcHashName = pass->getHashName();
            edge->dstHashName = resource->getHashName();
            edge->type = Edge::Type::PassToResource;
            edge->bufferDependency = dep;

            pass->getOutgoingEdges().push_back(edge.get());

            edges.push_back(std::move(edge));
        }

        // Storage images
        for (const auto& si : pass->getStorageImages()) {
            const auto resource = findResourceByName(si.image);

            // Define the dependency
            ImageDependencyInfo dep{
                .requiredLayout = VK_IMAGE_LAYOUT_GENERAL,
                .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
                .stageFlags = si.stageFlags,
            };

            // Create the write edge
            auto edge = std::make_unique<Edge>();
            edge->srcHashName = pass->getHashName();
            edge->dstHashName = resource->getHashName();
            edge->type = Edge::Type::PassToResource;
            edge->imageDependency = dep;

            pass->getOutgoingEdges().push_back(edge.get());

            edges.push_back(std::move(edge));
        }

        // Combined image samplers
        for (const auto& ci : pass->getCombinedImageSamplers()) {
            const auto resource = findResourceByName(ci.image);

            // Create the dependency
            ImageDependencyInfo dep{
                .requiredLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .stageFlags = ci.stageFlags,
            };

            // Create the read edge
            auto edge = std::make_unique<Edge>();
            edge->srcHashName = resource->getHashName();
            edge->dstHashName = pass->getHashName();
            edge->type = Edge::Type::ResourceToPass;
            edge->imageDependency = dep;

            pass->getIncomingEdges().push_back(edge.get());

            edges.push_back(std::move(edge));
        }

        // Compute pass specific
        if (auto cPass = std::dynamic_pointer_cast<ComputePass>(pass)) {
        }

        // Graphics pass specific
        if (auto gPass = std::dynamic_pointer_cast<GraphicsPass>(pass)) {
            // Color targets
            for (const auto& ct : gPass->getColorTargets()) {
                const auto resource = findResourceByName(ct.image);

                // Create the dependency
                ImageDependencyInfo dep{
                    .requiredLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                    .storeOp = ct.storeOp,
                    .loadOp = ct.loadOp,
                };

                // Create the edge
                auto edge = std::make_unique<Edge>();
                edge->srcHashName = pass->getHashName();
                edge->dstHashName = resource->getHashName();
                edge->type = Edge::Type::PassToResource;
                edge->imageDependency = dep;

                pass->getOutgoingEdges().push_back(edge.get());

                edges.push_back(std::move(edge));
            }

            if (gPass->hasDepthStencil()) {
                // Depth stencil target
                const auto& ds = gPass->getDepthStencil();
                const auto resource = findResourceByName(ds.image);

                // Create the dependency
                ImageDependencyInfo dep{
                    .requiredLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                    .storeOp = ds.storeOp,
                    .loadOp = ds.loadOp,
                };

                // Create the edge
                auto edge = std::make_unique<Edge>();
                edge->srcHashName = pass->getHashName();
                edge->dstHashName = resource->getHashName();
                edge->type = Edge::Type::PassToResource;
                edge->imageDependency = dep;

                pass->getOutgoingEdges().push_back(edge.get());

                edges.push_back(std::move(edge));
            }
        }
    }
};

auto Hammock::Rendergraph::Graph::dumpDotfile(const std::string& filename) const -> void {
    std::ofstream ofs(filename);
    ofs << "digraph RenderGraph {\n";
    ofs << "  rankdir=LR;\n";  // Left-to-right layout
    ofs << "  node [style=filled, fontname=\"Helvetica\"];\n";

    // --- Define nodes ---
    for (int i = 0; i < sortedResources.size(); i++) {
        auto hash = sortedResources[i];
        const auto& resource = resources.at(hash);
        const std::string flags = resource->flagsToString();  // e.g. "Used | Written"
        std::string label =
            "{ " + resource->getName() + " | (" + std::to_string(hash) + ") | [" + std::to_string(resource->lifetime.firstUse) + ", " + std::to_string(resource->lifetime.lastUse) + "]";
        if (!flags.empty()) {
            label += " | {" + flags + "}";
        }
        label += " }";

        ofs << "  \"" << hash << "\" [\n"
            << "    shape=Mrecord,\n"
            << "    fillcolor=lightblue,\n"
            << "    label=\"" << label << "\"\n"
            << "  ];\n";
    }

    for(int i = 0; i < sortedPasses.size(); i++){
        auto hash = sortedPasses[i];
        const auto& pass = passes.at(hash);
        const std::string flags = pass->flagsToString();  // e.g. "Compute | Async"
        std::string label = "{ (" + std::to_string(i) + ") " + pass->getName() + " | (" + std::to_string(hash) + ") ";
        if (!flags.empty()) {
            label += " | {" + flags + "}";
        }
        label += " }";

        ofs << "  \"" << hash << "\" [\n"
            << "    shape=record,\n"
            << "    fillcolor=lightcoral,\n"
            << "    label=\"" << label << "\"\n"
            << "  ];\n";
    }

    // --- Define edges ---
    for (const auto& edge : edges) {
        ofs << "  \"" << edge->srcHashName << "\" -> \"" << edge->dstHashName << "\"";
        /*if (!edge.flags.empty()) { // if you have metadata for edges
            ofs << " [label=\"" << edge.flags << "\"]";
        }*/
        ofs << ";\n";
    }

    ofs << "}\n";
}

auto Hammock::Rendergraph::Graph::findContributorsTo(uint32_hash_t target)
    -> std::unordered_set<uint32_hash_t> {
    // Build reversed graph
    std::unordered_map<uint32_hash_t, std::vector<uint32_hash_t>> reverseAdj;
    for (const auto& [src, neighbors] : adj) {
        for (auto dst : neighbors) {
            reverseAdj[dst].push_back(src);
        }
    }

    // BFS or DFS from target
    std::unordered_set<uint32_hash_t> reachable;
    std::queue<uint32_hash_t> q;

    q.push(target);
    reachable.insert(target);

    while (!q.empty()) {
        auto node = q.front();
        q.pop();

        for (auto pred : reverseAdj[node]) {
            if (reachable.insert(pred).second) {  // not seen before
                q.push(pred);
            }
        }
    }

    return reachable;
}

auto Hammock::Rendergraph::Graph::analyze() -> void {
    // Mark all resources that contribute to the swapchain image
    if (swapChainImageResolver != nullptr) {
        // only if swapchain is used
        uint32_hash_t sc = HashName(SWAP_CHAIN_IMAGE_RESOURCE_NAME);
        auto contributors = findContributorsTo(sc);

        for (const auto& contributor : contributors) {
            if (resources.contains(contributor))
                resources.at(contributor)->setFlag(NodeFlag::SwapChainContributing);
            else
                passes.at(contributor)->setFlag(NodeFlag::SwapChainContributing);
        }
    }

    // Analyze resource lifetimes
    for (int passIdx = 0; passIdx < sortedPasses.size(); passIdx++) {
        auto& pass = passes.at(sortedPasses[passIdx]);
        // For each read
        for (auto& edge : pass->getIncomingEdges()) {
            auto& resource = resources.at(edge->srcHashName);
            resource->lifetime.firstUse = std::min(resource->lifetime.firstUse, passIdx);
            resource->lifetime.lastUse = std::max(resource->lifetime.lastUse, passIdx);
        }
        // for each write
        for (auto& edge : pass->getOutgoingEdges()) {
            auto& resource = resources.at(edge->dstHashName);
            resource->lifetime.firstUse = std::min(resource->lifetime.firstUse, passIdx);
            resource->lifetime.lastUse = std::max(resource->lifetime.lastUse, passIdx);
        }
    }
};