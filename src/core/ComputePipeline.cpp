module;
#include <memory>
#include <stdexcept>
#include <compare>
#include <vulkan/vulkan.hpp>

module hammock_core.compute_pipeline;

// *************** Compute pipeline ***********************
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
    createShaderModule(config.byteCode, &shaderModule_);

    // Set up the compute shader stage.
    vk::PipelineShaderStageCreateInfo shaderStageInfo{};
    shaderStageInfo.stage = vk::ShaderStageFlagBits::eCompute;
    shaderStageInfo.module = shaderModule_;
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
    device.device().destroyShaderModule(shaderModule_, nullptr);
    device.device().destroyPipelineLayout(pipelineLayout, nullptr);
}

std::unique_ptr<hammock::core::ComputePipeline> hammock::core::ComputePipeline::create(
    const ComputePipelineCreateInfo &createInfo) {
    return std::make_unique<ComputePipeline>(createInfo);
}

void hammock::core::ComputePipeline::dispatch(CommandBuffer &commandBuffer, uint32_t x, uint32_t y, uint32_t z) {
    commandBuffer.getCommandBuffer().dispatch(x,y,z);
}

void hammock::core::ComputePipeline::bind(CommandBuffer &commandBuffer) {
    commandBuffer.getCommandBuffer().bindPipeline(vk::PipelineBindPoint::eCompute, pipeline);
}

void hammock::core::ComputePipeline::bindDescriptorSet(CommandBuffer &commandBuffer, uint32_t firstSet,
    const vk::DescriptorSet *descriptorSet) {
    commandBuffer.getCommandBuffer().bindDescriptorSets(
        vk::PipelineBindPoint::eCompute,
        pipelineLayout,
        firstSet, 1,
        descriptorSet,
        0, nullptr
    );
}

void hammock::core::ComputePipeline::pushConstants(CommandBuffer &commandBuffer, vk::ShaderStageFlags stageFlags,
    uint32_t offset, uint32_t size, const void *pValues) {
    commandBuffer.getCommandBuffer().pushConstants(pipelineLayout, vk::ShaderStageFlagBits::eCompute,
                                                   offset, size, pValues);
}

// *************** Compute pipeline builder ***********************

hammock::core::ComputePipelineBuilder::ComputePipelineBuilder(Device &device): createInfo_{device} {
}

hammock::core::ComputePipelineBuilder & hammock::core::ComputePipelineBuilder::setShaderBytecode(
    const std::vector<char> &bytecode, const std::string &entry) {
    createInfo_.byteCode = bytecode;
    createInfo_.entry= entry;
    return *this;
}

hammock::core::ComputePipelineBuilder & hammock::core::ComputePipelineBuilder::addDescriptorSetLayout(
    const std::unique_ptr<DescriptorSetLayout> &descriptorSetLayout) {
    createInfo_.descriptorSetLayouts.push_back(descriptorSetLayout->getDescriptorSetLayout());
    return *this;
}

hammock::core::ComputePipelineBuilder & hammock::core::ComputePipelineBuilder::addPushConstantRange(
    vk::PushConstantRange pushConstantRange) {
    createInfo_.pushConstantRanges.push_back(pushConstantRange);
    return *this;
}

std::unique_ptr<hammock::core::ComputePipeline> hammock::core::ComputePipelineBuilder::build() {
    return std::make_unique<ComputePipeline>(createInfo_);
}
