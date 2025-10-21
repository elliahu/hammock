#pragma once

#include <unordered_map>

#include "hammock/core/core.h"
#include "hammock/rendergraph/ILogicalRenderPass.h"

namespace Hammock {
    namespace Rendergraph {
        class Graph {
        private:
            ResourceManager &rm;
            std::vector<std::unique_ptr<ILogicalRenderPass> > passes;
            std::unordered_map<std::string, LogicalResource> resources{};

        public:
            struct CreateInfo {
                ResourceManager &resourceManager;
            };

            Graph() = delete;

            explicit Graph(const CreateInfo &createInfo);


            // Add a pass to the render graph
            // Note: this transfers ownership of the pass to the graph
            void addPass(std::unique_ptr<ILogicalRenderPass> pass) {
                passes.push_back(std::move(pass));
            }

            /**
             * Creates a resource in the graph
             * @tparam Type Type of the resource Node
             * @tparam ResourceType Type of the actual resource
             * @tparam DescriptionType Type of the description of the resource based on the type of the resource
             * @param name Name of the resource for lookup and reference
             * @param desc Description of the resource
             */
            template<LogicalResource::Type Type, typename ResourceType, typename DescriptionType>
            void addResource(const std::string &name, const DescriptionType &desc) {
                LogicalResource node;
                node.type = Type;
                node.name = name;
                node.resolver = [name, desc](ResourceManager &rm, uint32_t frameIndex) {
                    return rm.createResource<ResourceType>(name, desc);
                };
                resources[name] = std::move(node);
            }

            /**
             * Create a static (external, non-buffered) resource
             * @tparam Type Type of the resource node
             * @param name Name of the resource
             * @param handle Handle of the actuall resource
             */
            template<LogicalResource::Type Type>
            void addExternalResource(const std::string &name, ResourceHandle handle) {
                LogicalResource node;
                node.type = Type;
                node.name = name;
                node.resolver = [handle](ResourceManager &rm, uint32_t frameIndex) {
                    return handle;
                };
                resources[name] = std::move(node);
            }
        };
    }
};
