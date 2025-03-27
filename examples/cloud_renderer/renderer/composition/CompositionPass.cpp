#include "CompositionPass.h"

void CompositionPass::initialize(HmckVec2 resolution) {
    prepareTargets(static_cast<uint32_t>(resolution.X), static_cast<uint32_t>(resolution.Y));
    prepareDescriptors();
    preparePipelines();
    device.waitIdle();
    processDeletionQueue();
}

void CompositionPass::recordCommands(VkCommandBuffer commandBuffer, uint32_t frameIndex) {
    Image *compositedColor = resourceManager.getResource<Image>(color);
    VkExtent2D extent = {compositedColor->getExtent().width, compositedColor->getExtent().height};

    // Transition attachments into required layouts
    if (terrainColor->getLayout() != VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        terrainColor->transition(commandBuffer, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    }

    if (terrainDepth->getLayout() != VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        terrainDepth->transition(commandBuffer, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    }

    if (compositedColor->getLayout() != VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL) {
        compositedColor->transition(commandBuffer, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    }

    // Acquire ownership from compute queue
    cloudsColor->transition(commandBuffer, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_GENERAL, CommandQueueFamily::Graphics);


    // Begin rendering into intermediate composition image
    VkRenderingAttachmentInfo colorTarget = compositedColor->getRenderingAttachmentInfo();
    colorTarget.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorTarget.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

    VkRenderingInfo compositionRenderingInfo{VK_STRUCTURE_TYPE_RENDERING_INFO_KHR};
    compositionRenderingInfo.renderArea = {0, 0, extent.width, extent.height};
    compositionRenderingInfo.layerCount = 1;
    compositionRenderingInfo.colorAttachmentCount = 1;
    compositionRenderingInfo.pColorAttachments = &colorTarget;

    // Composition
    vkCmdBeginRendering(commandBuffer, &compositionRenderingInfo);

    // Viewport
    VkViewport compositionViewport{
        0.0f, 0.0f, static_cast<float>(extent.width), static_cast<float>(extent.height), 0.0f, 1.0f
    };
    vkCmdSetViewport(commandBuffer, 0, 1, &compositionViewport);

    // Scissors
    VkRect2D compositionScissors{0, 0, extent.width, extent.height};
    vkCmdSetScissor(commandBuffer, 0, 1, &compositionScissors);

    // Bind composition descriptor set
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->pipelineLayout, 0, 1,
                            &descriptor, 0, nullptr);

    // Bind composition pipeline
    pipeline->bind(commandBuffer);


    vkCmdPushConstants(commandBuffer, pipeline->pipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT,
                           0, sizeof(CompositionData), &data);


    // Draw
    vkCmdDraw(commandBuffer, 3, 1, 0, 0);

    // Finish the rendering
    vkCmdEndRendering(commandBuffer);
}

void CompositionPass::prepareTargets(uint32_t width, uint32_t height) {
    color = resourceManager.createResource<Image>(
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
    resourceManager.getResource<Image>(color)->queueImageLayoutTransition(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    sampler = resourceManager.createResource<Sampler>("composition-sampler", SamplerDesc{});
}

void CompositionPass::prepareDescriptors() {
    layout = DescriptorSetLayout::Builder(device)
            .addBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT) // Terrain color image
            .addBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT) // Terrain depth image
            .addBinding(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT) // Clouds color image
            .addBinding(3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT) // Sky view
            .build();

    Sampler *s = resourceManager.getResource<Sampler>(sampler);
    VkDescriptorImageInfo terrainColorImageInfo = terrainColor->getDescriptorImageInfo(s->getSampler());
    VkDescriptorImageInfo terrainDepthImageInfo = terrainDepth->getDescriptorImageInfo(s->getSampler());
    VkDescriptorImageInfo cloudsColorTarget = cloudsColor->getDescriptorImageInfo(s->getSampler());
    VkDescriptorImageInfo skyViewInfo = skyView->getDescriptorImageInfo(s->getSampler());
    DescriptorWriter(*layout, *descriptorPool)
            .writeImage(0, &terrainColorImageInfo)
            .writeImage(1, &terrainDepthImageInfo)
            .writeImage(2, &cloudsColorTarget)
            .writeImage(3, &skyViewInfo)
            .build(descriptor);
}

void CompositionPass::preparePipelines() {
    pipeline = GraphicsPipeline::create({
        .debugName = "composition-pipeline",
        .device = device,
        .vertexShader
        {.byteCode = Filesystem::readFile(COMPILED_SHADER_PATH("compose.vert")),},
        .fragmentShader
        {.byteCode = Filesystem::readFile(COMPILED_SHADER_PATH("compose.frag")),},
        .descriptorSetLayouts = {layout->getDescriptorSetLayout()},
        .pushConstantRanges{{VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(CompositionData)}},
        .graphicsState{
            .cullMode = VK_CULL_MODE_NONE,
            .vertexBufferBindings{}
        },
        .dynamicRendering = {
            .enabled = true,
            .colorAttachmentCount = 1, // We are rendering to single color attachment
            .colorAttachmentFormats = {resourceManager.getResource<Image>(color)->getFormat()},
        }
    });
}
