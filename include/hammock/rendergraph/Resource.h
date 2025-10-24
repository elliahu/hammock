#pragma once

#include <functional>

#include "hammock/core/core.h"
#include "hammock/rendergraph/Pass.h"
#include "hammock/rendergraph/Node.h"


namespace Hammock {
    namespace Rendergraph {
        typedef std::function<ResourceHandle(ResourceManager&, uint32_t frameIndex)> ResourceResolver;

        /// RenderGraph node representing a resource
        class Resource : public Node {
           public:
            enum class Type { Buffer, Image, SwapChainImage };

            Resource(uint32_hash_t hashName) : Node(hashName) {}

            // Resolver is used to resolve the actual resource handle for resources that are buffered
            ResourceResolver resolver;

            /**
             * Returns handle corresponding to resource of specific frame
             * @param rm ResourceManager where resource is registered
             * @param frameIndex Frame index of the resource
             * @return Returns resolved handle
             */
            ResourceHandle resolve(ResourceManager& rm, uint32_t frameIndex) {
                if (isDirty || cachedHandles.empty()) {
                    cachedHandles.resize(SwapChain::MAX_FRAMES_IN_FLIGHT);
                    isDirty = false;
                }

                if (!cachedHandles[frameIndex].isValid()) {
                    cachedHandles[frameIndex] = resolver(rm, frameIndex);
                }

                return cachedHandles[frameIndex];
            }

            virtual auto getType() -> Type = 0;

            void setDirty() { isDirty = true; }

           private:
            // cache to store handles to avoid constant recreation
            std::vector<ResourceHandle> cachedHandles;
            // Needs recreation
            bool isDirty = true;
        };

        class ImageResource : public Resource {
           public:
            ImageResource(uint32_hash_t hashName) : Resource(hashName) {}

            auto getType() -> Type override { return Type::Image; }
        };

        class BufferResource : public Resource {
            public:
            BufferResource(uint32_hash_t hashName) : Resource(hashName) {}

            auto getType() -> Type override { return Type::Buffer; }
        };

        class SwapChainImageResource : public Resource {
            public:
            SwapChainImageResource(uint32_hash_t hashName) : Resource(hashName) {}

            auto getType() -> Type override { return Type::SwapChainImage; }
        };

    }  // namespace Rendergraph
};  // namespace Hammock
