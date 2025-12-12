#pragma once

#include <vulkan/vulkan.h>

#include "hammock/rendergraph/FrameGraphNodeHandle.h"

namespace Hammock {
    namespace Rendergraph {

        struct Edge;

        typedef Edge* EdgePtr;

        struct ImageDependencyInfo {
            VkImageLayout requiredLayout;
            VkAttachmentStoreOp storeOp;
            VkAttachmentLoadOp loadOp;
        };

        struct BufferDependencyInfo {};

        struct DependencyInfo {
            // Shared fields
            VkShaderStageFlags stageFlags = VK_SHADER_STAGE_ALL;
            VkDescriptorType descriptorType;

            // Specific fields
            ImageDependencyInfo imageDependency{};
            BufferDependencyInfo bufferDependency{};
        };

        struct Edge {
            enum class Type {
                PassToResource,  // Pass WRITES to resource
                ResourceToPass   // Pass READS from resource
            };

            FrameGraphNodeHandle srcHashName;  // hash name of Pass or Resource node
            FrameGraphNodeHandle dstHashName;  // hash name of Pass or Resource node

            Type type;

            // Dependency metadata
            DependencyInfo dependency;
        };

    }  // namespace Rendergraph
}  // namespace Hammock