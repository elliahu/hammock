#pragma once

#include <vulkan/vulkan.h>

#include <cstdint>
#include <memory>

#include "hammock/core/Types.h"

namespace Hammock {
    namespace Rendergraph {

        struct Edge;

        typedef Edge* EdgePtr;

        struct ImageDependencyInfo {
            VkImageLayout requiredLayout;
            VkAttachmentStoreOp storeOp;
            VkAttachmentLoadOp loadOp;
            VkDescriptorType descriptorType;
            VkShaderStageFlags stageFlags = VK_SHADER_STAGE_ALL;
        };

        struct BufferDependencyInfo{
            VkShaderStageFlags stageFlags = VK_SHADER_STAGE_ALL;
        };

        struct Edge {

            enum class Type {
                PassToResource,  // Pass WRITES to resource
                ResourceToPass   // Pass READS from resource
            };

            uint32_hash_t srcHashName;  // hash name of Pass or Resource node
            uint32_hash_t dstHashName;  // hash name of Pass or Resource node

            Type type;

            // Dependency metadata
            ImageDependencyInfo imageDependency;

            BufferDependencyInfo bufferDependency;
        };

    }  // namespace Rendergraph
}  // namespace Hammock