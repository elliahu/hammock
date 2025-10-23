#pragma once

#include <functional>
#include "hammock/core/core.h"
#include "hammock/rendergraph/NodeInterface.h"

namespace Hammock {
    namespace Rendergraph {
        typedef std::function<ResourceHandle(ResourceManager&, uint32_t frameIndex)>
            ResourceResolver;

        /// RenderGraph node representing a resource
        class LogicalResource : public NodeInterface {
           public:

           enum class Type {
                UniformBuffer,
                VertexBuffer,
                IndexBuffer,
                StorageBuffer,
                PushConstantData,
                StorageImage,
                SampledImage,
                SwapChainImage,
                ColorAttachment,
                DepthStencilAttachment,
            };

            LogicalResource(uint32_hash_t hashName,  Type type) : NodeInterface(hashName), type(type){}

            
            

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

            bool isBuffer() const {
                return type == Type::UniformBuffer || type == Type::VertexBuffer || type == Type::IndexBuffer || type == Type::StorageBuffer;
            }

            bool isImage() const {
                return type == Type::ColorAttachment || type == Type::DepthStencilAttachment || type == Type::SwapChainImage || type == Type::StorageImage || type == Type::SampledImage;
            }

            bool isColorAttachment() const {
                return type == Type::ColorAttachment || type == Type::SwapChainImage;
            }

            bool isDepthAttachment() const {
                return type == Type::DepthStencilAttachment;
            }

            bool isSwapChainImage() const {
                return type == Type::SwapChainImage;
            }

            void setDirty() {
                isDirty = true;
            }

            

           private:
            // Type of the resource
            Type type;
            // cache to store handles to avoid constant recreation
            std::vector<ResourceHandle> cachedHandles;
            // Needs recreation
            bool isDirty = true;
        };
    }  // namespace Rendergraph
};  // namespace Hammock
