#pragma once

#include "hammock/core/Types.h"
#include "hammock/rendergraph/LogicalResource.h"

namespace Hammock {
    namespace Rendergraph {
        /// RenderPass interface
        /// Inherit from this interface to create a concrete render pass
        class ILogicalRenderPass {
        public:
            // This is called when pass is created, initialize all needed resources only in here
            virtual void create() = 0;

            // Release all allocated resources here
            virtual void release() = 0;

            struct CreateInfo {
                const CommandQueueFamily &commandQueueFamily;
                ResourceManager &resourceManager;
            };

            explicit ILogicalRenderPass(const CreateInfo &createInfo)
                : resourceManager(createInfo.resourceManager), commandQueueFamily(createInfo.commandQueueFamily) {
            }

            // Declare what resources this pass will read/write
            // Call read(...) to declare read access
            // Call write(...) to declare write access
            virtual void declareResources() = 0;

            // Execute pass code in this method
            virtual void recordCommands(VkCommandBuffer) = 0;

            // Retrieves command queue family for this pass
            [[nodiscard]] CommandQueueFamily getCommandQueueFamily() const { return commandQueueFamily; }

        protected:
            // Declare read access
            void read(const LogicalResourceAccess &resourceAccess) {
                resourceReads.push_back(resourceAccess);
            }

            // Declare write access
            void write(const LogicalResourceAccess &resourceAccess) {
                resourceWrites.push_back(resourceAccess);
            }

            CommandQueueFamily commandQueueFamily;
            ResourceManager &resourceManager;
            std::vector<LogicalResourceAccess> resourceReads;
            std::vector<LogicalResourceAccess> resourceWrites;
        };
    }
};
