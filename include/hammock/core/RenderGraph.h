#pragma once

#include <hammock/core/Device.h>
#include <hammock/core/CoreUtils.h>
#include <functional>
#include <cassert>
#include <variant>

#include "hammock/core/RenderContext.h"
#include "hammock/core/Types.h"


namespace hammock {
    /**
     * RenderGraph node representing a resource
     */
    struct ResourceNode {
        enum class Type {
            Buffer,
            Image,
            SwapChainImage
            // Special type of image. If resource is SwapChain, it is the final output resource that gets presented.
        } type;



        // Name is used for lookup
        std::string name;

        // Resource description
        std::variant<BufferDesc, ImageDesc> desc;

        // One handle per frame in flight
        std::vector<ResourceRef> refs;

        // Whether resource lives only within a frame
        bool isTransient = true;
        // Whether resource has one instance per frame in flight or just one overall
        bool isBuffered = true;
        // True if resource is externally owned (e.g. swapchain images)
        bool isExternal = false;
    };

    /**
     * Describes access intentions of a resource. Resource is references by its name from a resource map.
     */
    struct ResourceAccess {
        std::string resourceName;
        VkImageLayout requiredLayout;
        VkAttachmentLoadOp loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        VkAttachmentStoreOp storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        VkPipelineStageFlags stageFlags = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
    };

    /**
     * Describes a render pass context. This data is passed into rendering callback, and it is only infor available for rendering
     * TODO add relevant resources so they can be bound
     */
    struct RenderPassContext {
        VkCommandBuffer commandBuffer;
        uint32_t frameIndex;
    };

    /**
     * RenderGraph node representing a render pass
     */
    struct RenderPassNode {
        enum class Type {
            // Represents a pass that draws into some image
            Graphics,
            // Represents a compute pass
            Compute,
            // Represents a transfer pass - data is moved from one location to another (eg. host -> device or device -> host)
            Transfer
        } type; // Type of the pass

        std::string name; // Name of the pass for debug purposes and lookup
        VkExtent2D extent;
        std::vector<ResourceAccess> inputs; // Read accesses
        std::vector<ResourceAccess> outputs; // Write accesses

        // callback for rendering
        std::function<void(RenderPassContext)> executeFunc;

        std::vector<VkFramebuffer> framebuffers;
        VkRenderPass renderPass = VK_NULL_HANDLE;
    };


    /**
     * Wraps necessary operation needed for pipeline barrier transitions for a single resource
     */
    class Barrier {
        ResourceNode &node;
        ResourceAccess &access;
        VkCommandBuffer commandBuffer;
        uint32_t frameIndex = 0;

    public:
        explicit Barrier(ResourceNode &node, ResourceAccess &access, VkCommandBuffer commandBuffer,
                         const uint32_t frameIndex) : node(node),
                                                      access(access), commandBuffer(commandBuffer),
                                                      frameIndex(frameIndex) {
        }

        /**
         *  Checks if resource needs barrier transition give its access
         * @return true if resource needs barrier transition, else false
         */
        bool isNeeded() const {
            if (node.type == ResourceNode::Type::Image) {
                ASSERT(std::holds_alternative<ImageResourceRef>(node.refs[frameIndex].resource),
                       "Node is image but does not hold image ref")
                ;
                const ImageResourceRef& ref = std::get<ImageResourceRef>(node.refs[frameIndex].resource);
                return ref.currentLayout != access.requiredLayout;
            }
            // TODO support for buffer transition
            return false;
        }

        /**
         * Applies pipeline barrier transition
         */
        void apply() {
            if (node.type == ResourceNode::Type::Image) {
                // Verify that we're dealing with an image resource
                ASSERT(std::holds_alternative<ImageResourceRef>(node.refs[frameIndex].resource),
                       "Node is image node but does not hold image ref");

                // Get the image reference for the current frame
                ImageResourceRef &ref = std::get<ImageResourceRef>(node.refs[frameIndex].resource);

                ASSERT(std::holds_alternative<ImageDesc>(node.desc),
                       "Node is image node yet does not hold image desc.");
                const ImageDesc &desc = std::get<ImageDesc>(node.desc);

                // Skip if no transition is needed, this should already be checked but anyway...
                if (ref.currentLayout == access.requiredLayout) {
                    return;
                }

                // Create image memory barrier
                VkImageMemoryBarrier imageBarrier{};
                imageBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
                imageBarrier.oldLayout = ref.currentLayout;
                imageBarrier.newLayout = access.requiredLayout;
                imageBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                imageBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                imageBarrier.image = ref.image;

                // Define image subresource range
                imageBarrier.subresourceRange.aspectMask =
                        isDepthStencil(desc.format)
                            ? VK_IMAGE_ASPECT_DEPTH_BIT
                            : VK_IMAGE_ASPECT_COLOR_BIT;
                imageBarrier.subresourceRange.baseMipLevel = 0;
                imageBarrier.subresourceRange.levelCount = desc.mips;
                imageBarrier.subresourceRange.baseArrayLayer = 0;
                imageBarrier.subresourceRange.layerCount = desc.layers;

                // Set access masks based on layout transitions
                switch (ref.currentLayout) {
                    case VK_IMAGE_LAYOUT_UNDEFINED:
                        imageBarrier.srcAccessMask = 0;
                        break;
                    case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
                        imageBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                        break;
                    case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
                        imageBarrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
                        break;
                    case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
                        imageBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
                        break;
                    case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
                        imageBarrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
                        break;
                    default:
                        imageBarrier.srcAccessMask = 0;
                }

                switch (access.requiredLayout) {
                    case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
                        imageBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                        break;
                    case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
                        imageBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
                        break;
                    case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
                        imageBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
                        break;
                    case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
                        imageBarrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
                        break;
                    default:
                        imageBarrier.dstAccessMask = 0;
                }

                // Issue the pipeline barrier command
                vkCmdPipelineBarrier(
                    commandBuffer, // commandBuffer
                    access.stageFlags, // srcStageMask
                    access.stageFlags, // dstStageMask
                    0, // dependencyFlags
                    0, nullptr, // Memory barriers
                    0, nullptr, // Buffer memory barriers
                    1, &imageBarrier // Image memory barriers
                );

                // Update the current layout
                ref.currentLayout = access.requiredLayout;
            }
            // TODO support for buffer transition
        }
    };

    /**
     * RenderGraph is a class representing a directed acyclic graph (DAG) that describes the process of creation of a frame.
     * It consists of resource nodes and render pass nodes. Render pass node can be dependent on some resources and
     * can itself create resources that other passes may depend on. RenderGraph analyzes these dependencies, makes adjustments when possible,
     * makes necessary transitions between resource states. These transitions are done just in time as well as resource allocation.
     *  TODO support for Read Modify Write - render pass writes and reads from the same resource
     */
    class RenderGraph {
        // Vulkan device
        Device &device;
        // Rendering context
        RenderContext &renderContext;
        // Maximal numer of frames that GPU works on concurrently
        uint32_t maxFramesInFlight;
        // Holds all the resources
        std::unordered_map<std::string, ResourceNode> resources;
        // Holds all the render passes
        std::vector<RenderPassNode> passes;
        // Array of indexes into list of passes. Order is optimized by the optimizer.
        std::vector<uint32_t> optimizedOrderOfPasses;
        // Pass that is called last
        std::string presentPass;

        /**
         * Creates a resource for specific node
         * @param resourceNode Node for which to create a resource
         */
        void createResource(ResourceNode &resourceNode, ResourceAccess &access);

        /**
         *  Creates a buffer with a ref
         * @param resourceRef ResourceRef for resource to be created
         * @param bufferDesc Desc struct that describes the buffer to be created
         */
        void createBuffer(ResourceRef &resourceRef, BufferDesc &bufferDesc) const;

        /**
         *  Creates image with a ref
         * @param resourceRef ResourceRef for resource to be created
         * @param imageDesc Desc struct that describes the image to be created
         */
        void createImage(ResourceRef &resourceRef, ImageDesc &desc, ResourceAccess &access) const;

        /**
         * Loops through resources and destroys those that are transient. This should be called at the end of a frame
         */
        void destroyTransientResources();

        /**
         * Destroys resource connected with this node
         * @param resourceNode Resource node which resource will be destroyed
         */
        void destroyResource(ResourceNode &resourceNode) const;

        void createRenderPassAndFramebuffers(RenderPassNode &renderPassNode) const {
            std::vector<VkAttachmentDescription> attachmentDescriptions;
            std::vector<VkAttachmentReference> colorReferences;
            VkAttachmentReference depthReference = {};
            std::vector<std::vector<VkImageView>> attachmentViews;
            attachmentViews.resize(SwapChain::MAX_FRAMES_IN_FLIGHT);

            bool hasDepth = false;
            bool hasColor = false;
            uint32_t attachmentIndex = 0;
            uint32_t maxLayers = 0;

            ASSERT(!renderPassNode.outputs.empty(), "Render pass has no outputs");

            // loop over outputs to collect attachmetn descriptions of color/depth targets
            for (auto &access: renderPassNode.outputs) {
                ResourceNode resourceNode = resources.at(access.resourceName);

                // We skip non image outputs as they are not part of the framebuffer
                if (resourceNode.type != ResourceNode::Type::Image) { continue; }

                ImageDesc& imageDesc = std::get<ImageDesc>(resourceNode.desc);

                attachmentDescriptions.push_back(imageDesc.attachmentDesc);

                if (isDepthStencil(imageDesc.format)) {
                    ASSERT(!hasDepth, "Only one depth attachment allowed");
                    depthReference.attachment = attachmentIndex;
                    depthReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
                    hasDepth = true;
                } else {
                    colorReferences.push_back({attachmentIndex, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL});
                    hasColor = true;
                }

                if (imageDesc.layers > maxLayers) {
                    maxLayers = imageDesc.layers;
                }

                attachmentIndex++;
            }

            // collect image views
            for (int i = 0; i < attachmentViews.size(); i++) {
                std::vector<VkImageView> views = attachmentViews[i];
                for (auto &access: renderPassNode.outputs) {
                    ResourceNode resourceNode = resources.at(access.resourceName);
                    // We skip non image outputs as they are not part of the framebuffer
                    if (resourceNode.type != ResourceNode::Type::Image) { continue; }

                    ImageResourceRef& imageRef = std::get<ImageResourceRef>(resourceNode.refs[i].resource);
                    views.push_back(imageRef.view);
                }
                attachmentViews[i] = views;
            }

            // Default render pass setup uses only one subpass
            VkSubpassDescription subpass = {};
            subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
            if (hasColor) {
                subpass.pColorAttachments = colorReferences.data();
                subpass.colorAttachmentCount = static_cast<uint32_t>(colorReferences.size());
            }
            if (hasDepth) {
                subpass.pDepthStencilAttachment = &depthReference;
            }

            // Use subpass dependencies for attachment layout transitions
            std::array<VkSubpassDependency, 2> dependencies{};
            dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
            dependencies[0].dstSubpass = 0;
            dependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
            dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            dependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
            dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

            dependencies[1].srcSubpass = 0;
            dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
            dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            dependencies[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
            dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            dependencies[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
            dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

            // Create render pass
            VkRenderPassCreateInfo renderPassInfo = {};
            renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
            renderPassInfo.pAttachments = attachmentDescriptions.data();
            renderPassInfo.attachmentCount = static_cast<uint32_t>(attachmentDescriptions.size());
            renderPassInfo.subpassCount = 1;
            renderPassInfo.pSubpasses = &subpass;
            renderPassInfo.dependencyCount = 2;
            renderPassInfo.pDependencies = dependencies.data();
            checkResult(vkCreateRenderPass(device.device(), &renderPassInfo, nullptr, &renderPassNode.renderPass));

            // create framebuffer for each frame in flight
            renderPassNode.framebuffers.resize(SwapChain::MAX_FRAMES_IN_FLIGHT);
            for (int i = 0; i < renderPassNode.framebuffers.size(); i++) {
                VkFramebufferCreateInfo framebufferInfo = {};
                framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
                framebufferInfo.renderPass = renderPassNode.renderPass;
                framebufferInfo.pAttachments = attachmentViews[i].data();
                framebufferInfo.attachmentCount = static_cast<uint32_t>(attachmentViews[i].size());
                framebufferInfo.width = renderPassNode.extent.width;
                framebufferInfo.height = renderPassNode.extent.height;
                framebufferInfo.layers = maxLayers;
                checkResult(vkCreateFramebuffer(device.device(), &framebufferInfo, nullptr, &renderPassNode.framebuffers[i]));
            }

        }

        void destroyRenderPassAndFrambuffers(RenderPassNode &renderPassNode) {
            ASSERT(!renderPassNode.framebuffers.empty(), "Render pass has no framebuffer");
            ASSERT(renderPassNode.renderPass, "RenderPass is null");

            for (int i = 0; i < renderPassNode.framebuffers.size(); i++) {
                vkDestroyFramebuffer(device.device(), renderPassNode.framebuffers[i], nullptr);
            }
            vkDestroyRenderPass(device.device(), renderPassNode.renderPass, nullptr);
        }

    public:
        RenderGraph(Device &device, RenderContext &context): device(device), renderContext(context),
                                                             maxFramesInFlight(SwapChain::MAX_FRAMES_IN_FLIGHT) {
        }

        ~RenderGraph() {
            for (auto &resource: resources) {
                ResourceNode &resourceNode = resource.second;

                // We skip nodes of resources that are externally managed, these were created somewhere else and should be destroyed there as well
                if (resourceNode.isExternal) { continue; }

                destroyResource(resourceNode);
            }

            for (int i = 0; i < passes.size(); i++) {
                if (passes[i].renderPass != VK_NULL_HANDLE) {
                    destroyRenderPassAndFrambuffers(passes[i]);
                }
            }
        }

        /**
         * Adds a resource node to the render graph
         * @param resource Resource node to be added
         */
        void addResource(const ResourceNode &resource) {
            resources[resource.name] = resource;
        }

        /**
         * Adds render pass to the render graph
         * @param pass Render pass to be added
         */
        void addPass(const RenderPassNode &pass) {
            passes.push_back(pass);
        }

        /**
         * Adds a render pass as a present pass
         * @param pass Render pass to be added
         */
        void addPresentPass(const RenderPassNode &pass) {
            addPass(pass);
            ASSERT(presentPass.empty(), "Present pass already set");
            presentPass = pass.name;
        }

        /**
         * Compiles the render graph - analyzes dependencies and makes optimization
         */
        void compile() {
            ASSERT(!presentPass.empty(), "Missing present pass");

            // TODO we are now making no optimizations and assume that rendering goes in order of render pass submission

            // To be replaced when above task is implemented
            for (int i = 0; i < passes.size(); i++) {
                optimizedOrderOfPasses.push_back(i);
            }
        }


        /**
         * Executes the graph and makes necessary barrier transitions.
         * @param cmd CommandBuffer to which the render calls will be recorded
         * @param frameIndex Index of the current frame.This is used to identify which resource ref should be used of the resource node is buffered.
         */
        void execute() {
            ASSERT(optimizedOrderOfPasses.size() == passes.size(),
                   "RenderPass queue size don't match. This should not happen.");

            // Begin frame by creating master command buffer
            if (VkCommandBuffer cmd = renderContext.beginFrame()) {
                // Get the frame index. This is used to select buffered refs
                uint32_t frameIndex = renderContext.getFrameIndex();

                // Loop over passes in the optimized order
                for (const auto &passIndex: optimizedOrderOfPasses) {
                    ASSERT(
                        std::find(optimizedOrderOfPasses.begin(),optimizedOrderOfPasses.end(), passIndex) !=
                        optimizedOrderOfPasses.end(),
                        "Could not find the pass based of index. This should not happen")
                    ;
                    RenderPassNode &pass = passes[passIndex];

                    // Check for necessary transitions of input resources
                    for (auto &access: pass.inputs) {
                        ASSERT(resources.find(access.resourceName) != resources.end(),
                               "Could not find the input");

                        ResourceNode &resourceNode = resources[access.resourceName];

                        // We do not allow on-the-fly creation of input resources

                        // Apply barrier if it is needed
                        if (Barrier barrier(resourceNode, access, cmd, frameIndex); barrier.isNeeded()) {
                            barrier.apply();
                        }
                    }

                    // clear values for framebuffer
                    std::vector<VkClearValue> clearValues{};

                    // Check for necessary transitions of output resources
                    // If the resource was just created we need to transform it into correct layout
                    for (auto &access: pass.outputs) {
                        ASSERT(resources.find(access.resourceName) != resources.end(),
                               "Could not find the output");
                        ResourceNode &resourceNode = resources[access.resourceName];

                        // Check if the node contains refs to resources
                        if (resourceNode.refs.empty()) {
                            // Resource node does not contain resource refs
                            // That means this is first time using this resource and needs to be created
                            createResource(resourceNode, access);
                        }

                        // Apply barrier if it is needed
                        if (Barrier barrier(resourceNode, access, cmd, frameIndex); barrier.isNeeded()) {
                            barrier.apply();
                        }

                        if (resourceNode.type == ResourceNode::Type::Image) {
                            ImageDesc &dec = std::get<ImageDesc>(resourceNode.desc);
                            if (isDepthStencil(dec.format)) {
                                clearValues.push_back({.depthStencil = {1.0f, 0}});
                            } else {
                                clearValues.push_back({.color = {0.0f, 0.0f, 0.0f, 0.0f}});
                            }
                        }
                    }
                    // Check if this is a present pass
                    if (pass.name == presentPass) {
                        // Use the swap chain render pass
                        renderContext.beginSwapChainRenderPass(cmd);
                    } else {
                        // create the render pass if it does not exist yet
                        if (pass.renderPass == VK_NULL_HANDLE) {
                            // TODO in here find out next render pass using this resource and set final layout of the attachment to the requiredLayout to save one barrier
                            createRenderPassAndFramebuffers(pass);
                        }

                        // begin render pass
                        VkRenderPassBeginInfo renderPassInfo{};
                        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
                        renderPassInfo.renderPass = pass.renderPass;
                        renderPassInfo.framebuffer = pass.framebuffers[frameIndex];
                        renderPassInfo.renderArea.offset = {0, 0};
                        renderPassInfo.renderArea.extent = pass.extent;
                        renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
                        renderPassInfo.pClearValues = clearValues.data();
                        vkCmdBeginRenderPass(cmd, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
                    }

                    // Set viewport and scissors
                    VkViewport viewport = hammock::Init::viewport(
                        static_cast<float>(pass.extent.width), static_cast<float>(pass.extent.height),
                        0.0f, 1.0f);
                    VkRect2D scissor{{0, 0}, pass.extent};
                    vkCmdSetViewport(cmd, 0, 1, &viewport);
                    vkCmdSetScissor(cmd, 0, 1, &scissor);

                    // create pass context
                    RenderPassContext context{};
                    context.commandBuffer = cmd;
                    context.frameIndex = frameIndex;

                    // Execute the rendering
                    pass.executeFunc(context);

                    // End the render pass
                    vkCmdEndRenderPass(cmd);
                }

                renderContext.endFrame();
            }
        }
    };
};
