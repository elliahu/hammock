#pragma once

#include <expected>
#include <algorithm>
#include <unordered_map>

#include "hammock/core/core.h"
#include "hammock/rendergraph/ILogicalRenderPass.h"
#include "hammock/rendergraph/Edge.h"

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
            void addPass(std::unique_ptr<ILogicalRenderPass> pass) {
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
                if(const auto result = buildEdges(); !result) {
                    return result;
                }

                return {}; // Success
            }

            /**
             * @brief Executes the render graph by executing all passes
             *
             */
            void execute() {
                for (auto& pass : passes) {
                    pass.second->onRecordCommands(VK_NULL_HANDLE);  // TODO: Pass actual command buffer
                }
            }

           private:
            ResourceManager& rm;
            std::unordered_map<uint32_hash_t, std::unique_ptr<ILogicalRenderPass>> passes{}; // Logical passes in the graph
            std::unordered_map<uint32_hash_t, LogicalResource> resources{};   // resources owned by the graph or external resources accessed by the graph
            std::vector<Edge> edges{}; // Edges that connect resources with passes
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
            * srcStageMask = A.pipelineStage;
dstStageMask = B.pipelineStage;
srcAccessMask = A.accessMask;
dstAccessMask = B.accessMask;
oldLayout = A.layout;
newLayout = B.layout;
             */


            /**
             * @brief Fills the edges vector and removes unused resources
             */
            std::expected<void, std::string> buildEdges() {
                // Look for resources access by each pass and create edges
                // Build the visited map of resources to purge unused resources from the graph
                std::vector<uint32_hash_t> visitedResources;
                for(const auto& [passHashName, pass] : passes){
                    // Resource reads are ResourceToPass edges
                    for(const auto& resourceHashName : pass->resourceReads){
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
                            .srcHashName = pass->getHashName(),
                            .dstHashName = resource.getHashName(),
                            .type = Edge::Type::ResourceToPass, // Pass is reading the resource R -> P
                        });

                       pass->incomingEdges.push_back(edges.size() - 1);
                    }

                    // Resource writes are PassToResource edges
                    for(const auto& resourceHashName : pass->resourceWrites) {
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
                            .srcHashName = resource.getHashName(),
                            .dstHashName = pass->getHashName(),
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
        };
    }  // namespace Rendergraph
};  // namespace Hammock
