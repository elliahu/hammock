#pragma once

#include <array>
#include <cstdint>
#include <functional>
#include <limits>

#include "hammock/core/Types.h"
#include "hammock/core/core.h"
#include "hammock/rendergraph/Node.h"

namespace Hammock {
    namespace Rendergraph {
        typedef std::function<ResourceHandle(uint32_t frameIndex)> ResourceResolver;

        class Resource;
        class ImageResource;
        class BufferResource;

        typedef Resource* ResourcePtr;
        typedef ImageResource* ImageResourcePtr;
        typedef BufferResource* BufferResourcePtr;

        /// RenderGraph node representing a resource
        class Resource : public Node {
           public:
            enum class Type { Buffer, Image, SwapChainImage };

            Resource(std::string name) : Node(name) {}

            /// Returns a type of the resource
            virtual auto getType() -> Type = 0;

            /// Returns handle corresponding to resource of specific frame
            /// @param rm ResourceManager where resource is registered
            /// @param frameIndex Frame index of the resource
            /// @return Returns resolved handle
            ResourceHandle resolve(uint32_t frameIndex) {
                if (_isDirty || _cachedHandles.empty()) {
                    _cachedHandles.resize(SwapChain::MAX_FRAMES_IN_FLIGHT);
                    _isDirty = false;
                }

                if (!_cachedHandles[frameIndex].isValid()) {
                    _cachedHandles[frameIndex] = _resolver(frameIndex);
                }

                return _cachedHandles[frameIndex];
            }

            /// Sets resource resolver
            auto setResolver(ResourceResolver resolver) -> void { _resolver = std::move(resolver); }
            auto getResover() { return _resolver; }

            /// Marks resource as dirty
            auto setDirty() -> void { _isDirty = true; }

            auto setTransient() -> void { _transient = true; }

            /// Returns true if resource is frame-local (has one copy per frame-in-flight)
            auto isFrameLocal() -> bool { return _frameLocal_; }

            auto isTransient() -> bool { return _transient; }

            struct{
                int32_t firstUse = INT32_MAX;
                int32_t lastUse = -1;
            } lifetime;

           private:
            // Resolver is used to resolve the actual resource handle for resources that are buffered
            ResourceResolver _resolver;
            // cache to store handles to avoid constant recreation
            std::vector<ResourceHandle> _cachedHandles;
            // Needs recreation
            bool _isDirty = true;
            bool _frameLocal_ = true;
            bool _transient = false;        
        };

        class ImageResource : public Resource {
           public:
            ImageResource(std::string name) : Resource(name) {}

            auto getType() -> Type override { return Type::Image; }
        };

        class BufferResource : public Resource {
           public:
            BufferResource(std::string name) : Resource(name) {}

            auto getType() -> Type override { return Type::Buffer; }
        };

        class SwapChainImageResource : public Resource {
           public:
            SwapChainImageResource(std::string name) : Resource(name) {}

            auto getType() -> Type override { return Type::SwapChainImage; }
        };

    }  // namespace Rendergraph
};  // namespace Hammock
