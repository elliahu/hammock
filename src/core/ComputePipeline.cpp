module;
#include <vulkan/vulkan.h>
#include <stdexcept>

module hammock.core.compute_pipeline;

hammock::core::ComputePipeline::ComputePipeline(const ComputePipelineCreateInfo &config) : BasePipeline(config.device){
    // Create a pipeline layout using the provided descriptor set layouts and push constant ranges.
    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(config.descriptorSetLayouts.size());
    pipelineLayoutInfo.pSetLayouts = config.descriptorSetLayouts.empty() ? nullptr : config.descriptorSetLayouts.data();
    pipelineLayoutInfo.pushConstantRangeCount = static_cast<uint32_t>(config.pushConstantRanges.size());
    pipelineLayoutInfo.pPushConstantRanges = config.pushConstantRanges.empty()
                                                 ? nullptr
                                                 : config.pushConstantRanges.data();

    if (vkCreatePipelineLayout(device.device(), &pipelineLayoutInfo, nullptr, &pipelineLayout) != VkResult::VK_SUCCESS) {
        throw std::runtime_error("failed to create compute pipeline layout");
    }

    // Create the compute shader module.
    createShaderModule(config.byteCode, &shaderModule);

    // Set up the compute shader stage.
    VkPipelineShaderStageCreateInfo shaderStageInfo{};
    shaderStageInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    shaderStageInfo.module = shaderModule;
    shaderStageInfo.pName = config.entry.c_str();

    // Create the compute pipeline.
    VkComputePipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    pipelineInfo.stage = shaderStageInfo;
    pipelineInfo.layout = pipelineLayout;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
    pipelineInfo.basePipelineIndex = -1;

    if (vkCreateComputePipelines(device.device(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline) != VkResult::VK_SUCCESS) {
        throw std::runtime_error("failed to create compute pipeline");
    }
}

hammock::core::ComputePipeline::~ComputePipeline() {
    vkDestroyPipeline(device.device(), pipeline, nullptr);
    vkDestroyShaderModule(device.device(), shaderModule, nullptr);
    vkDestroyPipelineLayout(device.device(), pipelineLayout, nullptr);
}
