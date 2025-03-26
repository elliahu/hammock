#include "DepthPass.h"

void DepthPass::initialize(HmckVec2 resolution) {
    prepareTargets(static_cast<uint32_t>(resolution.X), static_cast<uint32_t>(resolution.Y));
    preparePipelines();
    device.waitIdle();
    processDeletionQueue();
}

void DepthPass::recordCommands(VkCommandBuffer commandBuffer, uint32_t frameIndex) {
    // Get render target pointers
    Image *cameraDepthImage = resourceManager.getResource<Image>(cameraDepth);
    Image *sunDepthImage = resourceManager.getResource<Image>(sunDepth);
    VkExtent3D renderingExtent = cameraDepthImage->getExtent();

    if (cameraDepthImage->getLayout() != VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
        cameraDepthImage->transition(commandBuffer, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
    if (sunDepthImage->getLayout() != VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
        sunDepthImage->transition(commandBuffer, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

    VkRenderingAttachmentInfo cameraDepthTarget = cameraDepthImage->getRenderingAttachmentInfo();
    cameraDepthTarget.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    cameraDepthTarget.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

    VkRenderingAttachmentInfo sunDepthTarget = sunDepthImage->getRenderingAttachmentInfo();
    sunDepthTarget.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    sunDepthTarget.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

    // Camera depth

    VkRenderingInfo renderingInfo{VK_STRUCTURE_TYPE_RENDERING_INFO_KHR};
    renderingInfo.renderArea = {0, 0, renderingExtent.width, renderingExtent.height};
    renderingInfo.layerCount = 1;
    renderingInfo.colorAttachmentCount = 0;
    renderingInfo.pDepthAttachment = &cameraDepthTarget;

    vkCmdBeginRendering(commandBuffer, &renderingInfo);

    // Viewport
    VkViewport viewport{0.0f, 0.0f, static_cast<float>(renderingExtent.width), static_cast<float>(renderingExtent.height), 0.0f, 1.0f};
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

    // Scissors
    VkRect2D scissor{0, 0, renderingExtent.width, renderingExtent.height};
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

    pipeline->bind(commandBuffer);

    // Bind triangle vertex buffer (contains position and colors)
    VkDeviceSize offsets[1]{0};
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertexBuffer->m_buffer, offsets);
    // Bind triangle index buffer
    vkCmdBindIndexBuffer(commandBuffer, indexBuffer->m_buffer, 0, VK_INDEX_TYPE_UINT32);

    // Draw indexed
    for (int i = 0; i < geometry.renderMeshes.size(); i++) {
        Geometry::MeshInstance &mesh = geometry.renderMeshes[i];
        HmckMat4 mvp = cameraProjection * cameraView * mesh.transform;

        vkCmdPushConstants(commandBuffer, pipeline->pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT,
                           0, sizeof(HmckMat4), &mvp);

        vkCmdDrawIndexed(commandBuffer, mesh.indexCount, 1, mesh.firstIndex, 0,
                         0);
    }

    // Finish the rendering
    vkCmdEndRendering(commandBuffer);

    // sun depth
    renderingInfo.pDepthAttachment = &sunDepthTarget;

    vkCmdBeginRendering(commandBuffer, &renderingInfo);

    // Viewport
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

    // Scissors
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

    pipeline->bind(commandBuffer);

    // Bind triangle vertex buffer (contains position and colors)
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertexBuffer->m_buffer, offsets);
    // Bind triangle index buffer
    vkCmdBindIndexBuffer(commandBuffer, indexBuffer->m_buffer, 0, VK_INDEX_TYPE_UINT32);

    // Draw indexed
    for (int i = 0; i < geometry.renderMeshes.size(); i++) {
        Geometry::MeshInstance &mesh = geometry.renderMeshes[i];
        HmckMat4 mvp = sunProjection * sunView * mesh.transform;

        vkCmdPushConstants(commandBuffer, pipeline->pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT,
                           0, sizeof(HmckMat4), &mvp);

        vkCmdDrawIndexed(commandBuffer, mesh.indexCount, 1, mesh.firstIndex, 0,
                         0);
    }

    // Finish the rendering
    vkCmdEndRendering(commandBuffer);

    // Release camera depth to the compute queue
    // cameraDepthImage->transition(commandBuffer, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
    //                              CommandQueueFamily::Compute);
}

void DepthPass::prepareTargets(uint32_t width, uint32_t height) {
    // Terrain depth
    cameraDepth = resourceManager.createResource<Image>(
        "camera-depth", ImageDesc{
            .width = width,
            .height = height,
            .channels = 1,
            .format = VK_FORMAT_D32_SFLOAT, // Guaranteed support on all devices
            .usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
            .imageType = VK_IMAGE_TYPE_2D,
            .imageViewType = VK_IMAGE_VIEW_TYPE_2D,
            .aspectFlags = VK_IMAGE_ASPECT_DEPTH_BIT,
            .clearValue = {.depthStencil = {1.0f, 0}},
            .currentQueueFamily = CommandQueueFamily::Graphics,
            .queueFamilies = {CommandQueueFamily::Graphics, CommandQueueFamily::Compute},
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        }
    );
    // Set initial layout
    resourceManager.getResource<Image>(cameraDepth)->queueImageLayoutTransition(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    sunDepth = resourceManager.createResource<Image>(
        "sun-depth", ImageDesc{
            .width = width,
            .height = height,
            .channels = 1,
            .format = VK_FORMAT_D32_SFLOAT, // Guaranteed support on all devices
            .usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
            .imageType = VK_IMAGE_TYPE_2D,
            .imageViewType = VK_IMAGE_VIEW_TYPE_2D,
            .aspectFlags = VK_IMAGE_ASPECT_DEPTH_BIT,
            .clearValue = {.depthStencil = {1.0f, 0}},
            .currentQueueFamily = CommandQueueFamily::Graphics,
            .queueFamilies = {CommandQueueFamily::Graphics, CommandQueueFamily::Compute},
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        }
    );
    // Set initial layout
    resourceManager.getResource<Image>(sunDepth)->queueImageLayoutTransition(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
}

void DepthPass::preparePipelines() {
    pipeline = GraphicsPipeline::create({
        .debugName = "terrain-color-and-depth-pipeline",
        .device = device,
        .vertexShader
        {.byteCode = Filesystem::readFile(COMPILED_SHADER_PATH("depth.vert")),},
        .fragmentShader
        {.byteCode = Filesystem::readFile(COMPILED_SHADER_PATH("depth.frag")),},
        .descriptorSetLayouts = {},
        .pushConstantRanges{{VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(HmckMat4)}},
        .graphicsState{
            .vertexBufferBindings{
                .vertexBindingDescriptions = Vertex::vertexInputBindingDescriptions(),
                .vertexAttributeDescriptions = Vertex::vertexInputAttributeDescriptions(),
            }
        },
        .dynamicRendering = {
            .enabled = true,
            .colorAttachmentCount = 0,
            .colorAttachmentFormats = {},
            .depthAttachmentFormat = resourceManager.getResource<Image>(sunDepth)->getFormat(),
        }
    });
}
