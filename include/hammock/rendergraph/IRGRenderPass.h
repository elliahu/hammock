#pragma once

#include "hammock/core/Types.h"

namespace hammock {
    /**
        * Rendergraph node interface for creating render passes
        *
        */
    class IRGRenderPass {
    public:
        // Pure virtual destructor to make sure that derived class frees its resources
        virtual ~IRGRenderPass() = 0;

        // Make sure that the constructor is required
        IRGRenderPass() = delete;
        explicit IRGRenderPass(const CommandQueueFamily &commandQueueFamily, ResourceManager &resourceManager)
        : resourceManager(resourceManager), commandQueueFamily(commandQueueFamily) {}

        // Declare what resources this pass will read/write
        // Call read(...) to declare read access
        // Call write(...) to declare write access
        virtual void declareResources() = 0;

        // Declare read access
        void read(const RGResourceAccess &resourceAccess) {
            resourceReads.push_back(resourceAccess);
        }

        // Declare write access
        void write(const RGResourceAccess &resourceAccess) {
            resourceWrites.push_back(resourceAccess);
        }

        [[nodiscard]] CommandQueueFamily getCommandQueueFamily() const {return commandQueueFamily;}

    protected:
        CommandQueueFamily commandQueueFamily;
        ResourceManager &resourceManager;
        std::vector<RGResourceAccess> resourceReads;
        std::vector<RGResourceAccess> resourceWrites;

    };
};