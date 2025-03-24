#include "GeometryPass.h"

#include "Types.h"

void GeometryPass::initialize(HmckVec2 resolution) {
    prepareGeometry();
    prepareTargets(static_cast<uint32_t>(resolution.X), static_cast<uint32_t>(resolution.Y));
    preparePipelines();
    device.waitIdle();
    processDeletionQueue();
}

void GeometryPass::recordCommands(VkCommandBuffer commandBuffer, uint32_t frameIndex) {
    // Get render target pointers
    Image *terrainColorTarget = resourceManager.getResource<Image>(color);
    Image *terrainDepthTarget = resourceManager.getResource<Image>(depth);
    VkExtent3D renderingExtent = terrainColorTarget->getExtent();

    // Pipeline barriers
    if (terrainColorTarget->getLayout() != VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL) {
        // Record the transition to required layout
        terrainColorTarget->transition(commandBuffer, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    }

    if (terrainDepthTarget->getLayout() != VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
        // Record the transition to required layout
        terrainDepthTarget->transition(commandBuffer, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
    }

    // Begin rendering
    VkRenderingAttachmentInfo colorTarget = terrainColorTarget->getRenderingAttachmentInfo();
    colorTarget.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorTarget.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

    VkRenderingAttachmentInfo depthTarget = terrainDepthTarget->getRenderingAttachmentInfo();
    depthTarget.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthTarget.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

    VkRenderingInfo renderingInfo{VK_STRUCTURE_TYPE_RENDERING_INFO_KHR};
    renderingInfo.renderArea = {0, 0, renderingExtent.width, renderingExtent.height};
    renderingInfo.layerCount = 1;
    renderingInfo.colorAttachmentCount = 1;
    renderingInfo.pColorAttachments = &colorTarget;
    renderingInfo.pDepthAttachment = &depthTarget;

    vkCmdBeginRendering(commandBuffer, &renderingInfo);

    // Viewport
    VkViewport viewport{0.0f, 0.0f, static_cast<float>(renderingExtent.width), static_cast<float>(renderingExtent.height), 0.0f, 1.0f};
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

    // Scissors
    VkRect2D scissor{0, 0, renderingExtent.width, renderingExtent.height};
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

    if (type == Type::DepthOnly) {
        depthOnlyPipeline->bind(commandBuffer);
    }
    else if (type == Type::ColorAndDepth) {
        colorAndDepthPipeline->bind(commandBuffer);
    }


    // Bind triangle vertex buffer (contains position and colors)
    VkDeviceSize offsets[1]{0};
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, &resourceManager.getResource<Buffer>(vertexBuffer)->m_buffer, offsets);
    // Bind triangle index buffer
    vkCmdBindIndexBuffer(commandBuffer, resourceManager.getResource<Buffer>(indexBuffer)->m_buffer, 0, VK_INDEX_TYPE_UINT32);

    // Draw indexed
    for (int i = 0; i < geometry.renderMeshes.size(); i++) {
        Geometry::MeshInstance &mesh = geometry.renderMeshes[i];

        shaderData.modelViewProjection = projection * view * mesh.transform;

        if (type == Type::DepthOnly) {
            vkCmdPushConstants(commandBuffer, depthOnlyPipeline->pipelineLayout,VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                           0, sizeof(ShaderData), &shaderData);
        }
        else if (type == Type::ColorAndDepth) {
            vkCmdPushConstants(commandBuffer, colorAndDepthPipeline->pipelineLayout,VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                          0, sizeof(ShaderData), &shaderData);
        }

        vkCmdDrawIndexed(commandBuffer, mesh.indexCount, 1, mesh.firstIndex, 0,
                         0);
    }

    // Finish the rendering
    vkCmdEndRendering(commandBuffer);
}

void GeometryPass::prepareGeometry() {
    Loader(geometry, device, resourceManager).loadglTF(ASSET_PATH("terrain.glb"));

    ASSERT(!geometry.vertices.empty(), "No vertices loaded!");

    vertexBuffer = resourceManager.createResource<Buffer>(
        "vertex-buffer", BufferDesc{
            .instanceSize = sizeof(Vertex),
            .instanceCount = static_cast<uint32_t>(geometry.vertices.size()),
            .usageFlags = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
            .currentQueueFamily = CommandQueueFamily::Ignored,
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        }
    );

    // Create vertex staging buffer
    ResourceHandle vertexStagingBuffer = queueForDeletion(resourceManager.createResource<Buffer>(
        "vertex-staging-buffer", BufferDesc{
            .instanceSize = sizeof(Vertex),
            .instanceCount = static_cast<uint32_t>(geometry.vertices.size()),
            .usageFlags = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            .allocationFlags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
        }
    ));

    // Write vertex data to the buffer
    resourceManager.getResource<Buffer>(vertexStagingBuffer)->map();
    resourceManager.getResource<Buffer>(vertexStagingBuffer)->writeToBuffer(geometry.vertices.data());

    // Copy data from staging buffer to actual vertex buffer
    VkDeviceSize vertexBufferSize = sizeof(Vertex) * geometry.vertices.size();
    resourceManager.getResource<Buffer>(vertexBuffer)->queuCopyFromBuffer(
        resourceManager.getResource<Buffer>(vertexStagingBuffer)->getBuffer(), vertexBufferSize);
    resourceManager.getResource<Buffer>(vertexStagingBuffer)->unmap();

    // Create index buffer
    indexBuffer = resourceManager.createResource<Buffer>(
        "index-buffer", BufferDesc{
            .instanceSize = sizeof(uint32_t),
            .instanceCount = static_cast<uint32_t>(geometry.indices.size()),
            .usageFlags = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
            .currentQueueFamily = CommandQueueFamily::Ignored,
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        }
    );

    // Create index staging buffer
    ResourceHandle indexStagingBuffer = queueForDeletion(resourceManager.createResource<Buffer>(
        "index-staging-buffer", BufferDesc{
            .instanceSize = sizeof(uint32_t),
            .instanceCount = static_cast<uint32_t>(geometry.indices.size()),
            .usageFlags = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            .allocationFlags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
        }
    ));

    // Write index data into the staging buffer
    resourceManager.getResource<Buffer>(indexStagingBuffer)->map();
    resourceManager.getResource<Buffer>(indexStagingBuffer)->writeToBuffer(geometry.indices.data());

    // Copy the data from staging buffer into actual index buffer
    VkDeviceSize indexBufferSize = sizeof(uint32_t) * geometry.indices.size();
    resourceManager.getResource<Buffer>(indexBuffer)->queuCopyFromBuffer(
        resourceManager.getResource<Buffer>(indexStagingBuffer)->getBuffer(), indexBufferSize);
    resourceManager.getResource<Buffer>(indexStagingBuffer)->unmap();
}

void GeometryPass::prepareTargets(uint32_t width, uint32_t height) {
    // Terrain image
    color = resourceManager.createResource<Image>(
        "terrain-image", ImageDesc{
            .width = width,
            .height = height,
            .channels = 4,
            .format = VK_FORMAT_R8G8B8A8_UNORM,
            .usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
            .imageType = VK_IMAGE_TYPE_2D,
            .imageViewType = VK_IMAGE_VIEW_TYPE_2D,
            .clearValue = {.color = {0.f, 0.f, 0.f, 0.f}},

        }
    );
    // Set initial layout
    resourceManager.getResource<Image>(color)->queueImageLayoutTransition(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    // Terrain depth
    depth = resourceManager.createResource<Image>(
        "terrain-depth", ImageDesc{
            .width = width,
            .height = height,
            .channels = 1,
            .format = VK_FORMAT_D32_SFLOAT, // Guaranteed support on all devices
            .usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
            .imageType = VK_IMAGE_TYPE_2D,
            .imageViewType = VK_IMAGE_VIEW_TYPE_2D,
            .aspectFlags = VK_IMAGE_ASPECT_DEPTH_BIT,
            .clearValue = {.depthStencil = {1.0f, 0}},
        }
    );
    // Set initial layout
    resourceManager.getResource<Image>(depth)->queueImageLayoutTransition(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
}

void GeometryPass::preparePipelines() {
    depthOnlyPipeline = GraphicsPipeline::create({
        .debugName = "terrain-depth-pipeline",
        .device = device,
        .vertexShader
        {.byteCode = Filesystem::readFile(COMPILED_SHADER_PATH("terrain.vert")),},
        .fragmentShader
        {.byteCode = Filesystem::readFile(COMPILED_SHADER_PATH("terrain-depth.frag")),},
        .descriptorSetLayouts = {},
        .pushConstantRanges{{VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(ShaderData)}},
        .graphicsState{
            .vertexBufferBindings{
                .vertexBindingDescriptions = Vertex::vertexInputBindingDescriptions(),
                .vertexAttributeDescriptions = Vertex::vertexInputAttributeDescriptions(),
            }
        },
        .dynamicRendering = {
            .enabled = true,
            .colorAttachmentCount = 1, // We are rendering to single color attachment
            .colorAttachmentFormats = {resourceManager.getResource<Image>(color)->getFormat()},
            .depthAttachmentFormat = resourceManager.getResource<Image>(depth)->getFormat(),
            // guaranteed to be supported on all hardware
        }
    });

    colorAndDepthPipeline = GraphicsPipeline::create({
        .debugName = "terrain-color-and-depth-pipeline",
        .device = device,
        .vertexShader
        {.byteCode = Filesystem::readFile(COMPILED_SHADER_PATH("terrain.vert")),},
        .fragmentShader
        {.byteCode = Filesystem::readFile(COMPILED_SHADER_PATH("terrain.frag")),},
        .descriptorSetLayouts = {},
        .pushConstantRanges{{VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(ShaderData)}},
        .graphicsState{
            .vertexBufferBindings{
                .vertexBindingDescriptions = Vertex::vertexInputBindingDescriptions(),
                .vertexAttributeDescriptions = Vertex::vertexInputAttributeDescriptions(),
            }
        },
        .dynamicRendering = {
            .enabled = true,
            .colorAttachmentCount = 1, // We are rendering to single color attachment
            .colorAttachmentFormats = {resourceManager.getResource<Image>(color)->getFormat()},
            .depthAttachmentFormat = resourceManager.getResource<Image>(depth)->getFormat(),
        }
    });
}
