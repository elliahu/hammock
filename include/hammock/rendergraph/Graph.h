#pragma once

#include <expected>
#include <algorithm>
#include <unordered_map>
#include <queue>

#include "hammock/core/core.h"
#include "hammock/rendergraph/LogicalRenderPassInterface.h"
#include "hammock/rendergraph/Edge.h"
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
            void addPass(std::unique_ptr<LogicalRenderPassInterface> pass) {
                // TODO resolve possible collisions (RARE)
                passes.insert({pass->getHashName(), std::move(pass)});
            }

            void addResource(const LogicalResource& resource) {
                // TODO resolve possible collisions (RARE)
                resources.insert({resource.getHashName(), resource});
            }

            /**
             * @brief Add a resolver to get the swapchain image for the current frame
             *
             * @param resolver Function that resolves the swapchain image for a given frame index
             */
            void useSwapChainImageResolver(SwapChainImageResolver resolver) {
                swapChainImageResolver = std::move(resolver);

                // Create a logical resource that represents a swap chain image
                LogicalResource logicalSwapImage{HashName(SWAP_CHAIN_IMAGE_RESOURCE_NAME), LogicalResource::Type::SwapChainImage};
                logicalSwapImage.resolver = nullptr;

                // Add to resources
                resources.insert({HashName(SWAP_CHAIN_IMAGE_RESOURCE_NAME),
                                  std::move(logicalSwapImage)});
            }


            std::expected<void, std::string> build() {
                // Build the edges
                if(const auto result = buildEdges(); !result) {
                    return result;
                }

                // Sort graph topologically
                if(const auto result = sortTopologically(); !result){
                    return result;
                }

                return {}; // Success
            }

            /**
             * @brief Executes the render graph by executing all passes
             */
            void execute() {
                for (auto& pass : passes) {
                    pass.second->onRecordCommands(VK_NULL_HANDLE);  // TODO: Pass actual command buffer
                }
            }

           private:
            ResourceManager& rm;
            std::unordered_map<uint32_hash_t, std::unique_ptr<LogicalRenderPassInterface>> passes{}; // Logical passes in the graph
            std::unordered_map<uint32_hash_t, LogicalResource> resources{};   // resources owned by the graph or external resources accessed by the graph
            std::vector<Edge> edges{}; // Edges that connect resources with passes
            std::vector<uint32_hash_t> sortedNodes{};
            SwapChainImageResolver swapChainImageResolver{nullptr}; // The resolver is used to get the current swap chain image for a given frame


            /**
             * @brief Finds resource by name in the graph
             *
             * @param name Name of the resource to be found
             * @return std::expected<LogicalResource&, std::string>
             */
            std::expected<std::reference_wrapper<LogicalResource>, std::string> findResourceByName(uint32_hash_t name) {
                auto it = resources.find(name);
                if (it != resources.end()) {
                    return std::ref(it->second);
                } else {
                    return std::unexpected("Resource with hash " + std::to_string(name) + " not found in render graph.");
                }
            }

            /**
             * @brief Get the Swap Chain Image object (uses the resolver)
             *
             * @return SwapChainImage
             */
            SwapChainImage getCurrentSwapChainImage() const {
                return swapChainImageResolver();
            }


            /**
             * @brief Fills the edges vector and removes unused resources
             */
            std::expected<void, std::string> buildEdges() {
                // Look for resources access by each pass and create edges
                // Build the visited map of resources to purge unused resources from the graph
                std::vector<uint32_hash_t> visitedResources;
                for(const auto& [passHashName, pass] : passes){
                    // Resource reads are incoming edges 
                    for(const auto& resourceHashName : pass->to){
                        const auto expectedResource = findResourceByName(resourceHashName);
                        if(!expectedResource) {
                            return std::unexpected("Failed to find the resource with hash " + std::to_string(resourceHashName));
                        }

                        // Retrieve the value
                        LogicalResource& resource = expectedResource.value();

                        // Add to visited resources
                        visitedResources.push_back(resource.getHashName());

                        // Create the edge
                        edges.push_back(Edge{
                            .srcHashName = resource.getHashName(),
                            .dstHashName = pass->getHashName(),
                            .type = Edge::Type::ResourceToPass, // Pass is reading the resource R -> P
                        });

                       pass->incomingEdges.push_back(edges.size() - 1);
                    }

                    // Resource writes are outgoing edges
                    for(const auto& resourceHashName : pass->from) {
                        const auto expectedResource = findResourceByName(resourceHashName);
                        if(!expectedResource) {
                            return std::unexpected("Failed to find the resource with hash" + std::to_string(resourceHashName));
                        }

                        // Retrieve the value
                        LogicalResource& resource = expectedResource.value();

                        // Add to visited resources
                        visitedResources.push_back(resource.getHashName());

                        // Create the edge
                        edges.push_back(Edge{
                            .srcHashName = pass->getHashName(),
                            .dstHashName = resource.getHashName(),
                            .type = Edge::Type::PassToResource, // Pass is writing the resource P -> R
                        });

                        pass->outgoingEdges.push_back(edges.size() - 1);
                    }
                }

                // Clear unvisited resources
                std::vector<uint32_hash_t> toBeDeletedResource;
                for(const auto&[resourceHashName, resource] : resources) {
                    if(std::ranges::find(visitedResources, resourceHashName) == visitedResources.end()) {
                        toBeDeletedResource.push_back(resourceHashName);
                    }
                }

                for(const uint32_hash_t& resourceHashName : toBeDeletedResource) {
                    resources.erase(resourceHashName);
                }

                return {}; // Success
            }


            std::expected<void, std::string> sortTopologically(){
                // Create list of all nodes
                std::vector<uint32_hash_t> nodes{};
                nodes.reserve(resources.size() + passes.size());

                for(const auto& pair : resources){
                    nodes.push_back(pair.first);
                }

                for(const auto& pair : passes){
                    nodes.push_back(pair.first);
                }
                
                // Create adjacency map and indegree map
                std::unordered_map<uint32_hash_t, std::vector<uint32_hash_t>> adj{};
                std::unordered_map<uint32_hash_t, uint32_t> indegree{};

                for(const auto& node : nodes){
                    adj[node];
                    indegree[node] = 0u;
                }

                for(const auto& edge : edges){
                    adj[edge.srcHashName].push_back(edge.dstHashName);
                    indegree[edge.dstHashName] += 1;
                }

                // Initialize queue with nodes that have no incoming edges
                std::queue<uint32_hash_t> queue{};

                for(const auto& node : nodes){
                    if(indegree[node] == 0){
                        queue.push(node);
                    }
                }

                // Initialize the sortedNodes list
                sortedNodes.clear();

                while(!queue.empty()){
                    auto node = queue.front();
                    queue.pop();

                    sortedNodes.push_back(node);

                    for(const auto& neighbor : adj[node]){
                        indegree[neighbor] -= 1;
                        if(indegree[neighbor] == 0){
                            queue.push(neighbor);
                        }
                    }
                }

                // Detect cycles
                if(sortedNodes.size() != nodes.size()){
                    return std::unexpected("Cycle detected in render graph");
                }

                return {}; // Success
            }
        };
    }  // namespace Rendergraph
};  // namespace Hammock
