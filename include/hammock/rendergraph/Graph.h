#pragma once

#include <algorithm>
#include <expected>
#include <queue>
#include <unordered_map>

#include "hammock/core/core.h"
#include "hammock/rendergraph/Edge.h"
#include "hammock/rendergraph/LogicalRenderPassInterface.h"
#include "hammock/rendergraph/NodeInterface.h"

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

            inline static const char* SWAP_CHAIN_IMAGE_RESOURCE_NAME = "__swap_chain_image__";

            // Resolver typedef
            typedef std::function<SwapChainImage()> SwapChainImageResolver;
            struct CreateInfo {
                ResourceManager& resourceManager;
            };

            Graph() = delete;

            explicit Graph(const CreateInfo& createInfo);

            /**
             * @brief Adds a pass to the render graph
             * @attention this transfers ownership of the pass to the graph
             * @param pass Logical render pass to add
             */
            auto addPass(std::unique_ptr<LogicalRenderPassInterface> pass) -> void;

            auto addResource(const LogicalResource& resource) -> void;

            /**
             * @brief Add a resolver to get the swapchain image for the current frame
             *
             * @param resolver Function that resolves the swapchain image for a given frame index
             */
            auto useSwapChainImageResolver(SwapChainImageResolver resolver) -> void;

            auto build() -> std::expected<void, std::string>;

            /**
             * @brief Executes the render graph by executing all passes
             */
            auto execute() -> void;

           private:
            ResourceManager& rm;
            std::unordered_map<uint32_hash_t, std::unique_ptr<LogicalRenderPassInterface>>
                passes{};  // Logical passes in the graph
            std::unordered_map<uint32_hash_t, LogicalResource>
                resources{};            // resources owned by the graph or external resources accessed by the graph
            std::vector<Edge> edges{};  // Edges that connect resources with passes
            std::vector<uint32_hash_t> sortedNodes{};
            SwapChainImageResolver swapChainImageResolver{
                nullptr};  // The resolver is used to get the current swap chain image for a given frame

            /**
             * @brief Finds resource by name in the graph
             *
             * @param name Name of the resource to be found
             * @return std::expected<LogicalResource&, std::string>
             */
            auto findResourceByName(uint32_hash_t name)
                -> std::expected<std::reference_wrapper<LogicalResource>, std::string>;

            /**
             * @brief Get the Swap Chain Image object (uses the resolver)
             *
             * @return SwapChainImage
             */
            auto getCurrentSwapChainImage() -> SwapChainImage const { return swapChainImageResolver(); }

            /**
             * @brief Fills the edges vector and removes unused resources
             */
            auto buildEdges() -> std::expected<void, std::string>;

            /**
            * @brief Sorts the graph topologically
            */
            auto sortTopologically() -> std::expected<void, std::string>;
        
        };
    }  // namespace Rendergraph
};  // namespace Hammock
