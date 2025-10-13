#pragma once

#include "hammock/core/Types.h"
#include "hammock/rendergraph/RGResource.h"

namespace Hammock {
    /// RenderPass interface
    /// Inherit from this interface to create a concrete render pass
    class IRGRenderPass {
    public:
        // Pure virtual destructor to make sure that derived class frees its resources
        virtual ~IRGRenderPass() = 0;

        // Make sure that the constructor is required
        IRGRenderPass() = delete;

        explicit IRGRenderPass(const CommandQueueFamily &commandQueueFamily, ResourceManager &resourceManager)
            : resourceManager(resourceManager), commandQueueFamily(commandQueueFamily) {
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
        void read(const RGResourceAccess &resourceAccess) {
            resourceReads.push_back(resourceAccess);
        }

        // Declare write access
        void write(const RGResourceAccess &resourceAccess) {
            resourceWrites.push_back(resourceAccess);
        }

        CommandQueueFamily commandQueueFamily;
        ResourceManager &resourceManager;
        std::vector<RGResourceAccess> resourceReads;
        std::vector<RGResourceAccess> resourceWrites;
    };
};
