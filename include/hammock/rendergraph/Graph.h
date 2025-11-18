#pragma once

#include <algorithm>
#include <cstdint>
#include <fstream>
#include <memory>
#include <queue>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "hammock/core/Buffer.h"
#include "hammock/core/CommandBuffer.h"
#include "hammock/core/CoreUtils.h"
#include "hammock/core/Descriptors.h"
#include "hammock/core/GraphicsPipeline.h"
#include "hammock/core/Types.h"
#include "hammock/core/core.h"
#include "hammock/rendergraph/Edge.h"
#include "hammock/rendergraph/Node.h"
#include "hammock/rendergraph/Pass.h"
#include "hammock/rendergraph/Resource.h"


namespace Hammock {
    namespace Rendergraph {

        class Graph{
           public:
            // Internal struct that represents swapchain image
            struct SwapChainImage {
                VkImage image;
                VkImageView imageView;
                VkFormat format;
            };

            /// name of the swapchain image for referencing
            inline static auto SWAP_CHAIN_IMAGE_RESOURCE_NAME = "__swap_chain_image__";

            // Resolver typedef
            typedef std::function<SwapChainImage()> SwapChainImageResolver;

            // Delete default constructor
            Graph() = delete;

            // Explicit one-param ctor
            explicit Graph(Device& device);

            /// Adds pass to the graph
            /// @param pass Unique pointer to the pass to be added
            /// @returns hash name of the pass
            auto addPass(std::shared_ptr<Pass> pass) -> uint32_hash_t {
                uint32_hash_t hash = pass->getHashName();
                passes.emplace(hash, std::move(pass));
                return hash;
            };

            /// Creates image resource in the graph
            /// @param desc image description
            /// @param name name of the image
            /// @returns hash name of the created image
            auto createImage(const std::string& name, ImageDesc desc) -> uint32_hash_t {
                auto imageResource = std::make_shared<ImageResource>(name);
                imageResource->setResolver([desc, name](uint32_t frameIndex) -> ResourceHandle {
                    return ResourceManager::getInstance().createResource<Image>(name, desc);
                });

                uint32_hash_t hash = imageResource->getHashName();
                resources[hash] = std::move(imageResource);
                return hash;
            };

            /// Add external image to the graph
            /// @param name name of the image
            /// @param resolver function  that resolves the actual resource handle
            /// @returns hash name of the created image
            auto importImage(const std::string& name, ResourceResolver resolver) -> uint32_hash_t {
                auto imageResource = std::make_shared<ImageResource>(name);
                imageResource->setResolver(resolver);
                uint32_hash_t hash = imageResource->getHashName();
                resources[hash] = std::move(imageResource);
                return hash;
            }

            /// Creates buffer resource in the graph
            /// @desc: buffer description
            /// @name: name of the buffer
            /// @returns hash name of the created buffer
            auto createBuffer(const std::string& name, BufferDesc desc) -> uint32_hash_t {
                auto bufferResource = std::make_shared<BufferResource>(name);
                bufferResource->setResolver([desc, name](uint32_t frameIndex) -> ResourceHandle {
                    return ResourceManager::getInstance().createResource<Buffer>(name, desc);
                });
                uint32_hash_t hash = bufferResource->getHashName();
                resources[hash] = std::move(bufferResource);
                return hash;
            };

            /// Add external buffer to the graph
            /// @param name name of the buffer
            /// @param resolver function  that resolves the actual resource handle
            /// @returns hash name of the created bufferResource
            auto importBuffer(const std::string& name, ResourceResolver resolver) -> uint32_hash_t {
                auto bufferResource = std::make_shared<BufferResource>(name);
                bufferResource->setResolver(resolver);
                uint32_hash_t hash = bufferResource->getHashName();
                resources[hash] = std::move(bufferResource);
                return hash;
            }

            /// Add a resolver to get the swapchain image for the current frame.
            /// @param resolver Function that resolves the swapchain image for a given frame index
            auto importSwapChainImage() -> uint32_hash_t;

            /// Constructs the graph, makes optimizations.
            auto build() -> void;

            /// Executes the graph in order
            auto execute() -> void;

            auto dumpDotfile(const std::string& filename) const -> void;

           private:
            Device& device;
            std::unordered_map<uint32_hash_t, std::shared_ptr<Pass>> passes{};  // Logical passes in the graph
            std::unordered_map<uint32_hash_t, std::shared_ptr<Resource>>
                resources{};  // resources owned by the graph or external resources accessed by the graph
            SwapChainImageResolver swapChainImageResolver{
                nullptr};  // The resolver is used to get the current swap chain image for a given frame
            std::vector<Edge> edges{};           // Edges that connect resources with passes
            std::vector<uint32_hash_t> nodes{};  // Nodes in the graph sorted topologically
            std::unordered_map<uint32_hash_t, std::vector<uint32_hash_t>> adj{};  // Adjacency map
            std::unordered_map<uint32_hash_t, uint32_t> indegree{};               // Indegree map

            /// Executes a function for each pass in the graph in topological order
            /// @param cb Callback
            auto forEachPass(std::function<void(PassPtr)> cb) -> void;

            /// Executes a function for each resource in the graph in topological order
            /// @param cb Callback
            auto forEachResource(std::function<void(std::shared_ptr<Resource>)> cb) -> void;

            /// Finds resource by name in the graph.
            /// @param name Name of the resource to be found
            /// @return resource or error
            auto findResourceByName(uint32_hash_t name)
                -> std::shared_ptr<Resource>;

            /// Duplicates resource. This is useful when modeling read-write access
            /// @param name name of the resource to be duplicated
            /// @returns resource duplicate
            auto duplicateResource(uint32_hash_t name) -> std::shared_ptr<Resource>;

            /// Get the Swap Chain Image object (uses the resolver).
            /// @returns SwapChainImage
            auto getCurrentSwapChainImage() -> SwapChainImage const { return swapChainImageResolver(); }

            /// Checks if a graph node is a (render) pass (node can also be a resource).
            /// @param hash Node hash
            auto isNodePass(uint32_hash_t hash) -> bool { return passes.contains(hash); }

            /// Checks if a graph node is a resource (node can also be a pass).
            /// @param hash Node hash
            auto isNodeResource(uint32_hash_t hash) -> bool { return resources.contains(hash); }

            /// Sorts the graph topologically.
            auto sortTopologically() -> void;

            /// Resolves all dependencies and creates edges
            auto resolveDependencies() -> void;

            /// Finds all nodes contributing to a given target.
            /// @param target target
            /// @returns unordered set of contributors
            auto findContributorsTo(uint32_hash_t target) -> std::unordered_set<uint32_hash_t>;

            /// Analyses the graph by marking nodes with flags
            auto analyze() -> void;
        };
    }  // namespace Rendergraph
};  // namespace Hammock
