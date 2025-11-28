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
#include "hammock/rendergraph/ExecutionContext.h"
#include "hammock/rendergraph/Node.h"
#include "hammock/rendergraph/Pass.h"
#include "hammock/rendergraph/Resource.h"
#include "hammock/rendergraph/FrameGraphNodeHandle.h"

namespace Hammock {
    namespace Rendergraph {

        class FrameGraph {
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
            FrameGraph() = delete;

            // Explicit one-param ctor
            explicit FrameGraph(Device& device);

            /// Adds pass to the graph
            /// @param pass Unique pointer to the pass to be added
            /// @returns hash name of the pass
            auto addPass(std::shared_ptr<Pass> pass) -> FrameGraphNodeHandle {
                FrameGraphNodeHandle hash = pass->getHashName();
                passes.emplace(hash, std::move(pass));
                return hash;
            };

            /// Creates image resource in the graph
            /// @param desc image description
            /// @param name name of the image
            /// @returns hash name of the created image
            auto createImage(const std::string& name, ImageDesc desc) -> FrameGraphNodeHandle {
                auto imageResource = std::make_shared<ImageResource>(name);
                imageResource->setResolver([desc, name](uint32_t frameIndex) -> ResourceHandle {
                    return ResourceManager::getInstance().createResource<Image>(name, desc);
                });

                FrameGraphNodeHandle hash = imageResource->getHashName();
                resources[hash] = std::move(imageResource);
                return hash;
            };

            /// Add external image to the graph
            /// @param name name of the image
            /// @param resolver function  that resolves the actual resource handle
            /// @returns hash name of the created image
            auto importImage(const std::string& name, ResourceResolver resolver) -> FrameGraphNodeHandle {
                auto imageResource = std::make_shared<ImageResource>(name);
                imageResource->setResolver(resolver);
                FrameGraphNodeHandle hash = imageResource->getHashName();
                resources[hash] = std::move(imageResource);
                return hash;
            }

            /// Creates buffer resource in the graph
            /// @desc: buffer description
            /// @name: name of the buffer
            /// @returns hash name of the created buffer
            auto createBuffer(const std::string& name, BufferDesc desc) -> FrameGraphNodeHandle {
                auto bufferResource = std::make_shared<BufferResource>(name);
                bufferResource->setResolver([desc, name](uint32_t frameIndex) -> ResourceHandle {
                    return ResourceManager::getInstance().createResource<Buffer>(name, desc);
                });
                FrameGraphNodeHandle hash = bufferResource->getHashName();
                resources[hash] = std::move(bufferResource);
                return hash;
            };

            /// Add external buffer to the graph
            /// @param name name of the buffer
            /// @param resolver function  that resolves the actual resource handle
            /// @returns hash name of the created bufferResource
            auto importBuffer(const std::string& name, ResourceResolver resolver) -> FrameGraphNodeHandle {
                auto bufferResource = std::make_shared<BufferResource>(name);
                bufferResource->setResolver(resolver);
                FrameGraphNodeHandle hash = bufferResource->getHashName();
                resources[hash] = std::move(bufferResource);
                return hash;
            }

            /// Add a resolver to get the swapchain image for the current frame.
            /// @param resolver Function that resolves the swapchain image for a given frame index
            auto importSwapChainImage() -> FrameGraphNodeHandle;

            /// Constructs the graph, makes optimizations.
            auto build() -> void;

            /// Executes the graph in order
            auto execute() -> void;

            auto dumpDotfile(const std::string& filename) const -> void;

           private:

            struct PassAllocation {
                std::vector<std::unique_ptr<CommandBuffer>> commandBuffers{};
                std::vector<DescriptorSet> descriptorSets{};
                std::unique_ptr<DescriptorSetLayout> descriptorSetLayout{nullptr};
            };

            Device& device;
            std::unordered_map<FrameGraphNodeHandle, std::shared_ptr<Pass>> passes{};  // Logical passes in the graph
            std::unordered_map<FrameGraphNodeHandle, std::shared_ptr<Resource>>
                resources{};  // resources owned by the graph or external resources accessed by the graph
            SwapChainImageResolver swapChainImageResolver{
                nullptr};  // The resolver is used to get the current swap chain image for a given frame
            std::vector<std::unique_ptr<Edge>> edges{};  // Edges that connect resources with passes
            std::vector<FrameGraphNodeHandle> nodes{};          // Nodes in the graph sorted topologically
            std::vector<FrameGraphNodeHandle> sortedPasses{};
            std::vector<FrameGraphNodeHandle> sortedResources{};
            std::unordered_map<FrameGraphNodeHandle, std::vector<FrameGraphNodeHandle>> adj{};  // Adjacency map
            std::unordered_map<FrameGraphNodeHandle, uint32_t> indegree{};               // Indegree map
            std::unordered_map<FrameGraphNodeHandle, std::unique_ptr<PassAllocation>> allocations{};    // Allocations per pass

            /// Executes a function for each pass in the graph in topological order
            /// @param cb Callback
            auto forEachPass(std::function<void(PassPtr)> cb) -> void;

            /// Executes a function for each resource in the graph in topological order
            /// @param cb Callback
            auto forEachResource(std::function<void(std::shared_ptr<Resource>)> cb) -> void;

            /// Finds resource by name in the graph.
            /// @param name Name of the resource to be found
            /// @return resource or error
            auto findResourceByName(FrameGraphNodeHandle name) -> std::shared_ptr<Resource>;

            /// Get the Swap Chain Image object (uses the resolver).
            /// @returns SwapChainImage
            auto getCurrentSwapChainImage() -> SwapChainImage const { return swapChainImageResolver(); }

            /// Checks if a graph node is a (render) pass (node can also be a resource).
            /// @param hash Node hash
            auto isNodePass(FrameGraphNodeHandle hash) -> bool { return passes.contains(hash); }

            /// Checks if a graph node is a resource (node can also be a pass).
            /// @param hash Node hash
            auto isNodeResource(FrameGraphNodeHandle hash) -> bool { return resources.contains(hash); }

            /// Sorts the graph topologically.
            auto sortTopologically() -> void;

            /// Resolves all dependencies and creates edges
            auto resolveDependencies() -> void;

            /// Finds all nodes contributing to a given target.
            /// @param target target
            /// @returns unordered set of contributors
            auto findContributorsTo(FrameGraphNodeHandle target) -> std::unordered_set<FrameGraphNodeHandle>;

            /// Analyses the graph by marking nodes with flags
            auto analyze() -> void;

            /// Optimizes the graph based on the prevous analysis
            auto optimize() -> void;

            /// Builds execution contexts for each pass
            auto allocate() -> void;
        };
    }  // namespace Rendergraph
};  // namespace Hammock
