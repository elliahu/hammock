#pragma once

#include <algorithm>
#include <cstdint>
#include <expected>
#include <memory>
#include <queue>
#include <unordered_map>
#include <vector>

#include "hammock/core/Buffer.h"
#include "hammock/core/CommandBuffer.h"
#include "hammock/core/CoreUtils.h"
#include "hammock/core/Types.h"
#include "hammock/core/core.h"
#include "hammock/rendergraph/Edge.h"
#include "hammock/rendergraph/Node.h"
#include "hammock/rendergraph/Pass.h"
#include "hammock/core/Descriptors.h"

namespace Hammock {
    namespace Rendergraph {

        class Graph {
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
            struct CreateInfo {
                Device& device;
                ResourceManager& resourceManager;
            };

            // Delete default constructor
            Graph() = delete;

            // Explicit one-param ctor
            explicit Graph(const CreateInfo& createInfo);

            /// Adds pass to the graph
            /// @param pass Unique pointer to the pass to be added
            /// @returns hash name of the pass
            auto addPass(std::unique_ptr<Pass> pass) -> uint32_hash_t {
                uint32_hash_t hash = pass->getHashName();
                passes.insert({pass->getHashName(), std::move(pass)});
                return hash;
            };

            /// Creates image resource in the graph
            /// @param desc image description
            /// @param name name of the image
            /// @returns hash name of the created image
            auto createImage(const std::string& name, ImageDesc desc) -> uint32_hash_t {
                uint32_hash_t hash = HashName(name.c_str());
                auto imageResource = std::make_shared<ImageResource>(hash);
                imageResource->setResolver(
                    [desc, name](ResourceManager& rm, uint32_t frameIndex) -> ResourceHandle {
                        return rm.createResource<Image>(name, desc);
                    });

                resources[hash] = std::move(imageResource);
                return hash;
            };

            /// Creates buffer resource in the graph
            /// @desc: buffer description
            /// @name: name of the buffer
            /// @returns hash name of the created buffer
            auto createBuffer(const std::string& name, BufferDesc desc) -> uint32_hash_t {
                uint32_hash_t hash = HashName(name.c_str());
                auto bufferResource = std::make_shared<BufferResource>(hash);
                bufferResource->setResolver(
                    [desc, name](ResourceManager& rm, uint32_t frameIndex) -> ResourceHandle {
                        return rm.createResource<Buffer>(name, desc);
                    });

                resources[hash] = std::move(bufferResource);
                return hash;
            };

            /// Add a resolver to get the swapchain image for the current frame.
            /// @param resolver Function that resolves the swapchain image for a given frame index
            auto useSwapChainImages(SwapChainImageResolver resolver) -> uint32_hash_t;

            /// Constructs the graph, makes optimizations.
            auto build() -> std::expected<void, std::string>;

            /// Executes the graph in order
            auto execute() -> void;

           private:
            ResourceManager& rm;
            Device& device;
            std::unordered_map<uint32_hash_t, std::unique_ptr<Pass>> passes{};  // Logical passes in the graph
            std::unordered_map<uint32_hash_t, std::shared_ptr<Resource>>
                resources{};  // resources owned by the graph or external resources accessed by the graph
            std::vector<Edge> edges{};                 // Edges that connect resources with passes
            std::vector<uint32_hash_t> sortedNodes{};  // Nodes in the graph sorted topologically
            SwapChainImageResolver swapChainImageResolver{
                nullptr};  // The resolver is used to get the current swap chain image for a given frame

            std::unordered_map<uint32_hash_t, CommandBuffer>
                commandBuffers{};  // Map of all command buffers allocated by the graph indexed by pass hash
            std::unordered_map<uint32_hash_t, DescriptorSetsAndLayout>
                descriptors{};  // Map of all descriptor sets allocated by the graph indexed by pass hash

            /// Executes a function for each pass in the graph in topological order
            /// @param cb Callback
            auto forEachPass(std::function<void(PassPtr)> cb) -> void;
            auto forEachPassExpected(std::function<std::expected<void, std::string>(PassPtr)> cb)
                -> std::expected<void, std::string>;

            /// Executes a function for each resource in the graph in topological order
            /// @param cb Callback
            auto forEachResource(std::function<void(std::shared_ptr<Resource>)> cb) -> void;
            auto forEachResourceExpected(
                std::function<std::expected<void, std::string>(std::shared_ptr<Resource>)> cb)
                -> std::expected<void, std::string>;

            /// Finds resource by name in the graph.
            /// @param name Name of the resource to be found
            /// @return resource or error
            auto findResourceByName(uint32_hash_t name)
                -> std::expected<std::shared_ptr<Resource>, std::string>;

            /// Get the Swap Chain Image object (uses the resolver).
            /// @returns SwapChainImage
            auto getCurrentSwapChainImage() -> SwapChainImage const { return swapChainImageResolver(); }

            /// Checks if a graph node is a (render) pass (node can also be a resource).
            /// @param hash Node hash
            auto isNodePass(uint32_hash_t hash) -> bool { return passes.contains(hash); }

            /// Checks if a graph node is a resource (node can also be a pass).
            /// @param hash Node hash
            auto isNodeResource(uint32_hash_t hash) -> bool { return resources.contains(hash); }

            ///
            auto getCommandBufferForPass(uint32_t hash) -> std::expected<CommandBuffer, std::string>;

            /// Fills the edges vector and removes unused resources.
            auto buildEdges() -> std::expected<void, std::string>;

            /// Sorts the graph topologically.
            auto sortTopologically() -> std::expected<void, std::string>;

            /// Allocates command buffers for all passes.
            /// There is one command buffer per each pass.
            auto buildCommandBuffers() -> std::expected<void, std::string>;

            /// Builds graph specific descriptors sets.
            /// These sets are automatically bound for each pass so that the pass can use graph-owned
            /// resources
            auto buildGraphDescriptors() -> std::expected<void, std::string>;
        };
    }  // namespace Rendergraph
};  // namespace Hammock
