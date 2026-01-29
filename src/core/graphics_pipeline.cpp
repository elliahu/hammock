#include <memory>
#include <stdexcept>
#include <vector>
#include <compare>
#include <vulkan/vulkan.hpp>

#include "graphics_pipeline.hpp"



std::unique_ptr<hammock::core::GraphicsPipeline> hammock::core::GraphicsPipeline::create(
    GraphicsPipelineCreateInfo createInfo) {
    return std::make_unique<GraphicsPipeline>(createInfo);
}

void hammock::core::GraphicsPipeline::beginRendering(CommandBuffer &commandBuffer,
    const vk::RenderingInfo *renderingInfo) {
    commandBuffer.getCommandBuffer().beginRendering(renderingInfo);
}

void hammock::core::GraphicsPipeline::endRendering(CommandBuffer &commandBuffer) {
    commandBuffer.getCommandBuffer().endRendering();
}

void hammock::core::GraphicsPipeline::setViewport(CommandBuffer &commandBuffer, float x, float y, float width,
    float height, float minDepth, float maxDepth) {
    vk::Viewport viewport = {x, y, width, height, minDepth, maxDepth};
    commandBuffer.getCommandBuffer().setViewport(0, 1, &viewport);
}

void hammock::core::GraphicsPipeline::
setScissor(CommandBuffer &commandBuffer, vk::Offset2D offset, vk::Extent2D extent) {
    vk::Rect2D rec{offset, extent};
    commandBuffer.getCommandBuffer().setScissor(0, 1, &rec);
}

void hammock::core::GraphicsPipeline::bind(CommandBuffer &commandBuffer) {
    commandBuffer.getCommandBuffer().bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline);
}

void hammock::core::GraphicsPipeline::bindDescriptorSet(CommandBuffer &commandBuffer, uint32_t firstSet,
    const vk::DescriptorSet *descriptorSet) {
    commandBuffer.getCommandBuffer().bindDescriptorSets(
        vk::PipelineBindPoint::eGraphics,
        pipelineLayout,
        firstSet, 1,
        descriptorSet,
        0, nullptr
    );
}

void hammock::core::GraphicsPipeline::pushConstants(CommandBuffer &commandBuffer, vk::ShaderStageFlags stageFlags,
    uint32_t offset, uint32_t size, const void *pValues) {
    commandBuffer.getCommandBuffer().pushConstants(pipelineLayout, stageFlags, offset, size, pValues);
}

hammock::core::GraphicsPipeline::~GraphicsPipeline() {
    device.device().destroyShaderModule(vertShaderModule_, nullptr);
    device.device().destroyShaderModule(fragShaderModule_, nullptr);
    device.device().destroyPipelineLayout(pipelineLayout, nullptr);
    device.device().destroyPipeline(pipeline, nullptr);
}

hammock::core::GraphicsPipeline::GraphicsPipeline(hammock::core::GraphicsPipelineCreateInfo &createInfo) : BasePipeline(
    createInfo.device) {
    vk::PipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(createInfo.descriptorSetLayouts.size());
    pipelineLayoutInfo.pSetLayouts = createInfo.descriptorSetLayouts.data();
    pipelineLayoutInfo.pushConstantRangeCount = static_cast<uint32_t>(createInfo.pushConstantRanges.size());
    pipelineLayoutInfo.pPushConstantRanges = createInfo.pushConstantRanges.data();

    if (createInfo.device.device().createPipelineLayout(&pipelineLayoutInfo, nullptr, &pipelineLayout) !=
        vk::Result::eSuccess) {
        throw std::runtime_error("failed to create pipeline layout");
    }

    GraphicsPipelineConfig configInfo{};
    GraphicsPipeline::defaultRenderPipelineConfig(configInfo);
    if (!createInfo.blendAtaAttachmentStates.empty()) {
        configInfo.colorBlendInfo.attachmentCount = static_cast<uint32_t>(createInfo.blendAtaAttachmentStates.size());
        configInfo.colorBlendInfo.pAttachments = createInfo.blendAtaAttachmentStates.data();
    }
    configInfo.depthStencilInfo.depthTestEnable = createInfo.depthTest;
    configInfo.depthStencilInfo.depthWriteEnable = createInfo.depthTest;
    configInfo.rasterizationInfo.cullMode = createInfo.cullMode;
    configInfo.rasterizationInfo.frontFace = createInfo.frontFace;

    configInfo.dynamicStateEnables = createInfo.dynamicStateEnables;
    configInfo.dynamicStateInfo.pDynamicStates = configInfo.dynamicStateEnables.data();
    configInfo.dynamicStateInfo.dynamicStateCount =
            static_cast<uint32_t>(configInfo.dynamicStateEnables.size());
    configInfo.dynamicStateInfo.flags = {};

    std::vector<vk::PipelineShaderStageCreateInfo> shaderStages;
    shaderStages.resize(2);

    createShaderModule(createInfo.vertexShader.byteCode, &vertShaderModule_);
    createShaderModule(createInfo.fragmentShader.byteCode, &fragShaderModule_);
    shaderStages[0].stage = vk::ShaderStageFlagBits::eVertex;
    shaderStages[0].module = vertShaderModule_;
    shaderStages[0].pName = createInfo.vertexShader.entry.c_str();
    shaderStages[0].flags = {};
    shaderStages[0].pNext = nullptr;
    shaderStages[0].pSpecializationInfo = nullptr;
    shaderStages[1].stage = vk::ShaderStageFlagBits::eFragment;
    shaderStages[1].module = fragShaderModule_;
    shaderStages[1].pName = createInfo.fragmentShader.entry.c_str();
    shaderStages[1].flags = {};
    shaderStages[1].pNext = nullptr;
    shaderStages[1].pSpecializationInfo = nullptr;


    auto &bindingDescriptions = createInfo.vertexInputBindingDescriptions;
    auto &attributeDescriptions = createInfo.vertexInputAttributeDescriptions;
    vk::PipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.vertexAttributeDescriptionCount =
            static_cast<uint32_t>(attributeDescriptions.size());
    vertexInputInfo.vertexBindingDescriptionCount = static_cast<uint32_t>(bindingDescriptions.size());
    vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();
    vertexInputInfo.pVertexBindingDescriptions = bindingDescriptions.data();

    vk::GraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.stageCount = shaderStages.size();
    pipelineInfo.pStages = shaderStages.data();
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &configInfo.inputAssemblyInfo;
    pipelineInfo.pViewportState = &configInfo.viewportInfo;
    pipelineInfo.pRasterizationState = &configInfo.rasterizationInfo;
    pipelineInfo.pMultisampleState = &configInfo.multisampleInfo;
    pipelineInfo.pColorBlendState = &configInfo.colorBlendInfo;
    pipelineInfo.pDepthStencilState = &configInfo.depthStencilInfo;
    pipelineInfo.pDynamicState = &configInfo.dynamicStateInfo;

    pipelineInfo.layout = pipelineLayout;
    pipelineInfo.renderPass = createInfo.renderPass;
    pipelineInfo.subpass = 0;

    pipelineInfo.basePipelineIndex = -1;
    pipelineInfo.basePipelineHandle = nullptr;

    vk::PipelineRenderingCreateInfoKHR pipelineDynamicrenderingCreateInfo{};
    if (createInfo.renderPass == nullptr) {
        // Attachment information for dynamic rendering
        pipelineDynamicrenderingCreateInfo.colorAttachmentCount = static_cast<std::uint32_t>(createInfo.
            colorAttachmentFormats.size());
        pipelineDynamicrenderingCreateInfo.pColorAttachmentFormats = createInfo.colorAttachmentFormats.
                data();
        pipelineDynamicrenderingCreateInfo.depthAttachmentFormat = createInfo.depthAttachmentFormat;
        pipelineDynamicrenderingCreateInfo.stencilAttachmentFormat = createInfo.stencilAttachmentFormat;
        pipelineInfo.pNext = &pipelineDynamicrenderingCreateInfo;
    }


    if (createInfo.device.device().createGraphicsPipelines(
            nullptr,
            1,
            &pipelineInfo,
            nullptr,
            &pipeline) != vk::Result::eSuccess) {
        throw std::runtime_error("failed to create graphics pipeline");
    }
}


void hammock::core::GraphicsPipeline::defaultRenderPipelineConfig(GraphicsPipelineConfig &configInfo) {
    configInfo.inputAssemblyInfo.topology = vk::PrimitiveTopology::eTriangleList;
    configInfo.inputAssemblyInfo.primitiveRestartEnable = vk::False;

    configInfo.viewportInfo.viewportCount = 1;
    configInfo.viewportInfo.pViewports = nullptr;
    configInfo.viewportInfo.scissorCount = 1;
    configInfo.viewportInfo.pScissors = nullptr;

    configInfo.rasterizationInfo.depthClampEnable = vk::False;
    configInfo.rasterizationInfo.rasterizerDiscardEnable = vk::False;
    configInfo.rasterizationInfo.polygonMode = vk::PolygonMode::eFill; // VK_POLYGON_MODE_LINE VK_POLYGON_MODE_FILL;
    configInfo.rasterizationInfo.lineWidth = 1.0f;
    configInfo.rasterizationInfo.cullMode = vk::CullModeFlagBits::eBack;
    configInfo.rasterizationInfo.frontFace = vk::FrontFace::eClockwise;
    configInfo.rasterizationInfo.depthBiasEnable = true;

    configInfo.multisampleInfo.sampleShadingEnable = false;
    configInfo.multisampleInfo.rasterizationSamples = vk::SampleCountFlagBits::e1;
    configInfo.multisampleInfo.minSampleShading = 1.0f; // Optional
    configInfo.multisampleInfo.pSampleMask = nullptr; // Optional
    configInfo.multisampleInfo.alphaToCoverageEnable = false; // Optional
    configInfo.multisampleInfo.alphaToOneEnable = false; // Optional

    configInfo.colorBlendAttachment.colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
    configInfo.colorBlendAttachment.blendEnable = false;
    configInfo.colorBlendAttachment.srcColorBlendFactor = vk::BlendFactor::eOne; // Optional
    configInfo.colorBlendAttachment.dstColorBlendFactor = vk::BlendFactor::eZero; // Optional
    configInfo.colorBlendAttachment.colorBlendOp = vk::BlendOp::eAdd; // Optional
    configInfo.colorBlendAttachment.srcAlphaBlendFactor = vk::BlendFactor::eOne; // Optional
    configInfo.colorBlendAttachment.dstAlphaBlendFactor = vk::BlendFactor::eZero; // Optional
    configInfo.colorBlendAttachment.alphaBlendOp = vk::BlendOp::eAdd; // Optional

    configInfo.colorBlendInfo.logicOpEnable = false;
    configInfo.colorBlendInfo.logicOp = vk::LogicOp::eCopy; // Optional
    configInfo.colorBlendInfo.attachmentCount = 1;
    configInfo.colorBlendInfo.pAttachments = &configInfo.colorBlendAttachment;
    configInfo.colorBlendInfo.blendConstants[0] = 0.0f; // Optional
    configInfo.colorBlendInfo.blendConstants[1] = 0.0f; // Optional
    configInfo.colorBlendInfo.blendConstants[2] = 0.0f; // Optional
    configInfo.colorBlendInfo.blendConstants[3] = 0.0f; // Optional

    configInfo.depthStencilInfo.depthTestEnable = true;
    configInfo.depthStencilInfo.depthWriteEnable = true;
    configInfo.depthStencilInfo.depthCompareOp = vk::CompareOp::eLessOrEqual;
    configInfo.depthStencilInfo.depthBoundsTestEnable = false;
    configInfo.depthStencilInfo.minDepthBounds = 0.0f; // Optional
    configInfo.depthStencilInfo.maxDepthBounds = 1.0f; // Optional
    configInfo.depthStencilInfo.stencilTestEnable = false;
    configInfo.depthStencilInfo.front = vk::StencilOpState{}; // Optional
    configInfo.depthStencilInfo.back = vk::StencilOpState{}; // Optional

    configInfo.dynamicStateEnables = {vk::DynamicState::eViewport, vk::DynamicState::eScissor};
    configInfo.dynamicStateInfo.pDynamicStates = configInfo.dynamicStateEnables.data();
    configInfo.dynamicStateInfo.dynamicStateCount =
            static_cast<uint32_t>(configInfo.dynamicStateEnables.size());
    configInfo.dynamicStateInfo.flags = {};
}

hammock::core::GraphicsPipelineBuilder & hammock::core::GraphicsPipelineBuilder::setVertexShader(
    const std::vector<char> &byteCode, const std::string &entry) {
    createInfo.vertexShader.byteCode = byteCode;
    createInfo.vertexShader.entry = entry;
    return *this;
}

hammock::core::GraphicsPipelineBuilder & hammock::core::GraphicsPipelineBuilder::setFragmentShader(
    const std::vector<char> &byteCode, const std::string &entry) {
    createInfo.fragmentShader.byteCode = byteCode;
    createInfo.fragmentShader.entry = entry;
    return *this;
}

hammock::core::GraphicsPipelineBuilder & hammock::core::GraphicsPipelineBuilder::addDescriptorSetLayout(
    const std::unique_ptr<DescriptorSetLayout> &descriptorSetLayout) {
    createInfo.descriptorSetLayouts.push_back(descriptorSetLayout->getDescriptorSetLayout());
    return *this;
}

hammock::core::GraphicsPipelineBuilder & hammock::core::GraphicsPipelineBuilder::addPushConstantRange(
    vk::PushConstantRange pushConstantRange) {
    createInfo.pushConstantRanges.push_back(pushConstantRange);
    return *this;
}

hammock::core::GraphicsPipelineBuilder & hammock::core::GraphicsPipelineBuilder::setDepthTest(vk::Bool32 depthTest,
    vk::CompareOp compareOp) {
    createInfo.depthTest = depthTest;
    createInfo.depthTestCompareOp = compareOp;
    return *this;
}

hammock::core::GraphicsPipelineBuilder & hammock::core::GraphicsPipelineBuilder::setCullMode(vk::CullModeFlags cullMode,
    vk::FrontFace frontFace) {
    createInfo.cullMode = cullMode;
    createInfo.frontFace = frontFace;
    return *this;
}

hammock::core::GraphicsPipelineBuilder & hammock::core::GraphicsPipelineBuilder::addBlendAttachmentState(
    vk::ColorComponentFlags colorWriteMask, vk::Bool32 blendEnable) {
    createInfo.blendAtaAttachmentStates.push_back(
        graphicsPipelineColorBlendAttachmentState(colorWriteMask, blendEnable));
    return *this;
}

hammock::core::GraphicsPipelineBuilder & hammock::core::GraphicsPipelineBuilder::setVertexInputAttributeDescriptions(
    std::vector<vk::VertexInputAttributeDescription> vertexInputAttributeDescriptions) {
    createInfo.vertexInputAttributeDescriptions = std::move(vertexInputAttributeDescriptions);
    return *this;
}

hammock::core::GraphicsPipelineBuilder & hammock::core::GraphicsPipelineBuilder::addVertexInputAttributeDescription(
    std::uint32_t location, std::uint32_t binding, vk::Format format, std::uint32_t offset) {
    createInfo.vertexInputAttributeDescriptions.push_back({location, binding, format, offset});
    return *this;
}

hammock::core::GraphicsPipelineBuilder & hammock::core::GraphicsPipelineBuilder::setVertexInputBindingDescriptions(
    std::vector<vk::VertexInputBindingDescription> vertexInputBindingDescriptions) {
    createInfo.vertexInputBindingDescriptions = std::move(vertexInputBindingDescriptions);
    return *this;
}

hammock::core::GraphicsPipelineBuilder & hammock::core::GraphicsPipelineBuilder::addVertexInputBindingDescription(
    std::uint32_t binding, std::uint32_t stride, vk::VertexInputRate inputRate) {
    createInfo.vertexInputBindingDescriptions.push_back({binding, stride, inputRate});
    return *this;
}

hammock::core::GraphicsPipelineBuilder & hammock::core::GraphicsPipelineBuilder::addColorAttachmentFormat(
    vk::Format format) {
    createInfo.colorAttachmentFormats.push_back(format);
    return *this;
}

hammock::core::GraphicsPipelineBuilder & hammock::core::GraphicsPipelineBuilder::setDepthStencilAttachmentFormat(
    vk::Format depth, vk::Format stencil) {
    createInfo.depthAttachmentFormat = depth;
    createInfo.stencilAttachmentFormat = stencil;
    return *this;
}

hammock::core::GraphicsPipelineBuilder & hammock::core::GraphicsPipelineBuilder::setRenderPass(
    vk::RenderPass renderPass) {
    createInfo.renderPass = renderPass;
    return *this;
}
