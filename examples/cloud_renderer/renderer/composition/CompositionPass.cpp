#include "CompositionPass.h"

void CompositionPass::initialize(HmckVec2 resolution) {
    prepareBuffer();
    prepareTargets(static_cast<uint32_t>(resolution.X), static_cast<uint32_t>(resolution.Y));
    prepareDescriptors();
    preparePipelines();
    device.waitIdle();
    processDeletionQueue();

    data.resX = resolution.X;
    data.resY = resolution.Y;
}

void CompositionPass::recordCommands(VkCommandBuffer commandBuffer, uint32_t frameIndex) {
    Image *compositedColorTarget = resourceManager.getResource<Image>(compositedImage);
    Image *skyColorTarget = resourceManager.getResource<Image>(skyColor);
    VkExtent2D extent = {compositedColorTarget->getExtent().width, compositedColorTarget->getExtent().height};

    resourceManager.getResource<Buffer>(buffer)->writeToBuffer(&data);

    // Transition attachments into required layouts
    if (terrainColor->getLayout() != VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        terrainColor->transition(commandBuffer, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    }

    if (terrainDepth->getLayout() != VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        terrainDepth->transition(commandBuffer, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    }

    if (compositedColorTarget->getLayout() != VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL) {
        compositedColorTarget->transition(commandBuffer, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    }

    if (skyColorTarget->getLayout() != VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL) {
        skyColorTarget->transition(commandBuffer, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    }

    // Acquire ownership from compute queue
    cloudsColor->transition(commandBuffer, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_GENERAL, CommandQueueFamily::Graphics);


    // Begin rendering into intermediate composition image
    VkRenderingAttachmentInfo colorTarget = skyColorTarget->getRenderingAttachmentInfo();
    colorTarget.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorTarget.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

    VkRenderingInfo renderingInfo{VK_STRUCTURE_TYPE_RENDERING_INFO_KHR};
    renderingInfo.renderArea = {0, 0, extent.width, extent.height};
    renderingInfo.layerCount = 1;
    renderingInfo.colorAttachmentCount = 1;
    renderingInfo.pColorAttachments = &colorTarget;

    // Sky
    vkCmdBeginRendering(commandBuffer, &renderingInfo);

    // Viewport
    VkViewport compositionViewport{
        0.0f, 0.0f, static_cast<float>(extent.width), static_cast<float>(extent.height), 0.0f, 1.0f
    };
    vkCmdSetViewport(commandBuffer, 0, 1, &compositionViewport);

    // Scissors
    VkRect2D compositionScissors{0, 0, extent.width, extent.height};
    vkCmdSetScissor(commandBuffer, 0, 1, &compositionScissors);

    // Bind composition descriptor set
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, skyPipeline->pipelineLayout, 0, 1,
                            &compositionDescriptor, 0, nullptr);

    // Bind sky descriptor set
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, skyPipeline->pipelineLayout, 1, 1,
                            &skyDescriptor, 0, nullptr);

    // Bind composition pipeline
    skyPipeline->bind(commandBuffer);

    // Draw
    vkCmdDraw(commandBuffer, 3, 1, 0, 0);

    // Finish the rendering
    vkCmdEndRendering(commandBuffer);

    // transition sky image
    if (skyColorTarget->getLayout() != VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        skyColorTarget->transition(commandBuffer, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    }

    // Composition
    colorTarget = compositedColorTarget->getRenderingAttachmentInfo();
    colorTarget.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorTarget.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    renderingInfo.renderArea = {0, 0, extent.width, extent.height};
    renderingInfo.layerCount = 1;
    renderingInfo.colorAttachmentCount = 1;
    renderingInfo.pColorAttachments = &colorTarget;

    vkCmdBeginRendering(commandBuffer, &renderingInfo);
    vkCmdSetViewport(commandBuffer, 0, 1, &compositionViewport);
    vkCmdSetScissor(commandBuffer, 0, 1, &compositionScissors);

    compositionPipeline->bind(commandBuffer);

    // Draw
    vkCmdDraw(commandBuffer, 3, 1, 0, 0);

    // Finish the rendering
    vkCmdEndRendering(commandBuffer);


}

void CompositionPass::prepareBuffer() {
    buffer = resourceManager.createResource<Buffer>(
        "composition-uniform-buffer", BufferDesc{
            .instanceSize = sizeof(CompositionData),
            .instanceCount = 1,
            .usageFlags = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            .allocationFlags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
            .queueFamilies = {CommandQueueFamily::Graphics},
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        }
    );
    // Map the buffers
    resourceManager.getResource<Buffer>(buffer)->map();
}

void CompositionPass::prepareTargets(uint32_t width, uint32_t height) {
    compositedImage = resourceManager.createResource<Image>(
        "composited-color-image", ImageDesc{
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
    resourceManager.getResource<Image>(compositedImage)->queueImageLayoutTransition(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    skyColor = resourceManager.createResource<Image>(
        "sky-color-image", ImageDesc{
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
    resourceManager.getResource<Image>(skyColor)->queueImageLayoutTransition(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);


    sampler = resourceManager.createResource<Sampler>("composition-sampler", SamplerDesc{});
}

void CompositionPass::prepareDescriptors() {
    compositionLayout = DescriptorSetLayout::Builder(device)
            .addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT)
            .addBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT) // Terrain color image
            .addBinding(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT) // Terrain depth image
            .addBinding(3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT) // Clouds color image
            .addBinding(4, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT) // Sky color
            .build();

    skyLayout = DescriptorSetLayout::Builder(device)
           .addBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT) // Sky view LUT
           .build();

    Sampler *s = resourceManager.getResource<Sampler>(sampler);
    VkDescriptorBufferInfo bufferInfo = resourceManager.getResource<Buffer>(buffer)->descriptorInfo();
    VkDescriptorImageInfo terrainColorImageInfo = terrainColor->getDescriptorImageInfo(s->getSampler());
    VkDescriptorImageInfo terrainDepthImageInfo = terrainDepth->getDescriptorImageInfo(s->getSampler());
    VkDescriptorImageInfo cloudsColorTarget = cloudsColor->getDescriptorImageInfo(s->getSampler());
    VkDescriptorImageInfo skyViewLUTInfo = skyView->getDescriptorImageInfo(s->getSampler());
    VkDescriptorImageInfo skyColorInfo = resourceManager.getResource<Image>(skyColor)->getDescriptorImageInfo(s->getSampler());
    DescriptorWriter(*compositionLayout, *descriptorPool)
            .writeBuffer(0, &bufferInfo)
            .writeImage(1, &terrainColorImageInfo)
            .writeImage(2, &terrainDepthImageInfo)
            .writeImage(3, &cloudsColorTarget)
            .writeImage(4, &skyColorInfo)
            .build(compositionDescriptor);

    DescriptorWriter(*skyLayout, *descriptorPool)
            .writeImage(0, &skyViewLUTInfo)
            .build(skyDescriptor);
}

void CompositionPass::preparePipelines() {
    compositionPipeline = GraphicsPipeline::create({
        .debugName = "composition-pipeline",
        .device = device,
        .vertexShader
        {.byteCode = Filesystem::readFile(COMPILED_SHADER_PATH("compose.vert")),},
        .fragmentShader
        {.byteCode = Filesystem::readFile(COMPILED_SHADER_PATH("compose.frag")),},
        .descriptorSetLayouts = {compositionLayout->getDescriptorSetLayout()},
        .pushConstantRanges{},
        .graphicsState{
            .cullMode = VK_CULL_MODE_NONE,
            .vertexBufferBindings{}
        },
        .dynamicRendering = {
            .enabled = true,
            .colorAttachmentCount = 1, // We are rendering to single color attachment
            .colorAttachmentFormats = {resourceManager.getResource<Image>(compositedImage)->getFormat()},
        }
    });

    skyPipeline = GraphicsPipeline::create({
        .debugName = "sky-pipeline",
        .device = device,
        .vertexShader
        {.byteCode = Filesystem::readFile(COMPILED_SHADER_PATH("sky.vert")),},
        .fragmentShader
        {.byteCode = Filesystem::readFile(COMPILED_SHADER_PATH("sky.frag")),},
        .descriptorSetLayouts = {
            compositionLayout->getDescriptorSetLayout(),
            skyLayout->getDescriptorSetLayout(),
        },
        .pushConstantRanges{},
        .graphicsState{
            .cullMode = VK_CULL_MODE_NONE,
            .vertexBufferBindings{}
        },
        .dynamicRendering = {
            .enabled = true,
            .colorAttachmentCount = 1, // We are rendering to single color attachment
            .colorAttachmentFormats = {resourceManager.getResource<Image>(skyColor)->getFormat()},
        }
    });

}
