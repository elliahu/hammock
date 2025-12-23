module;
#include <stdexcept>

module hammock.core.compute_pipeline;
import vulkan_hpp;

hammock::core::ComputePipeline::ComputePipeline(const ComputePipelineCreateInfo &config) : BasePipeline(config.device) {
    // Create a pipeline layout using the provided descriptor set layouts and push constant ranges.
    vk::PipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(config.descriptorSetLayouts.size());
    pipelineLayoutInfo.pSetLayouts = config.descriptorSetLayouts.empty() ? nullptr : config.descriptorSetLayouts.data();
    pipelineLayoutInfo.pushConstantRangeCount = static_cast<uint32_t>(config.pushConstantRanges.size());
    pipelineLayoutInfo.pPushConstantRanges = config.pushConstantRanges.empty()
                                                 ? nullptr
                                                 : config.pushConstantRanges.data();

    if (device.device().createPipelineLayout(&pipelineLayoutInfo, nullptr, &pipelineLayout) != vk::Result::eSuccess) {
        throw std::runtime_error("failed to create compute pipeline layout");
    }

    // Create the compute shader module.
    createShaderModule(config.byteCode, &shaderModule);

    // Set up the compute shader stage.
    vk::PipelineShaderStageCreateInfo shaderStageInfo{};
    shaderStageInfo.stage = vk::ShaderStageFlagBits::eCompute;
    shaderStageInfo.module = shaderModule;
    shaderStageInfo.pName = config.entry.c_str();

    // Create the compute pipeline.
    vk::ComputePipelineCreateInfo pipelineInfo{};
    pipelineInfo.stage = shaderStageInfo;
    pipelineInfo.layout = pipelineLayout;
    pipelineInfo.basePipelineIndex = -1;

    if (device.device().createComputePipelines({}, 1, &pipelineInfo, nullptr, &pipeline) != vk::Result::eSuccess) {
        throw std::runtime_error("failed to create compute pipeline");
    }
}

hammock::core::ComputePipeline::~ComputePipeline() {
    device.device().destroyPipeline(pipeline, nullptr);
    device.device().destroyShaderModule(shaderModule, nullptr);
    device.device().destroyPipelineLayout(pipelineLayout, nullptr);
}
