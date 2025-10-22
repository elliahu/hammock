#pragma once

#include <cstdint>
#include <vulkan/vulkan.h>

#include "hammock/core/Types.h"

namespace Hammock {
    namespace Rendergraph {
        struct Edge {
            uint32_hash_t srcHashName;  // hash name of Pass or Resource node
            uint32_hash_t dstHashName;  // hash name of Pass or Resource node

            enum class Type {
                PassToResource, // Pass WRITES to resource
                ResourceToPass  // Pass READS from resource
            } type;

            // Dependency metadata: TODO

        };

    }  // namespace Rendergraph
}  // namespace Hammock