#pragma once

namespace Hammock {
    namespace Rendergraph {
        typedef std::function<ResourceHandle(ResourceManager &, uint32_t frameIndex)>
        ResourceResolver;

        /// Describes how render pass accesses specific resource identified by its name
        struct LogicalResourceAccess {
            std::string name; // resource identification
            VkImageLayout requiredLayout = VK_IMAGE_LAYOUT_UNDEFINED; // layout in which the resource is to be used
            VkAttachmentLoadOp loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR; // preserve previous content ?
            VkAttachmentStoreOp storeOp = VK_ATTACHMENT_STORE_OP_STORE; // store current content ?
            CommandQueueFamily requiredQueueFamily = CommandQueueFamily::Ignored;
            // required command queue family (which family is taking ownership)
        };

        /// RenderGraph node representing a resource
        struct LogicalResource {
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
            } type;

            // Uniquely identifies the resource
            std::string name;

            // Resolver is used to resolve the actual resource handle for resources that are buffered
            ResourceResolver resolver;

            // cache to store handles to avoid constant recreation
            std::vector<ResourceHandle> cachedHandles;

            // Needs recreation
            bool isDirty = true;

            /**
             * Returns handle corresponding to resource of specific frame
             * @param rm ResourceManager where resource is registered
             * @param frameIndex Frame index of the resource
             * @return Returns resolved handle
             */
            ResourceHandle resolve(ResourceManager &rm, uint32_t frameIndex) {
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
                return type == Type::UniformBuffer || type == Type::VertexBuffer || type == Type::IndexBuffer || type ==
                       Type::StorageBuffer;
            }

            bool isImage() const {
                return type == Type::ColorAttachment || type == Type::DepthStencilAttachment || type ==
                       Type::SwapChainImage || type == Type::StorageImage || type == Type::SampledImage;
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
        };
    }
};
