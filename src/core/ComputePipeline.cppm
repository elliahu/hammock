module;

#include <vector>
#include <string>
#include <memory>
#include <vulkan/vulkan.hpp>

export module hammock.core.compute_pipeline;

import hammock.core.device;
import hammock.core.command_buffer;
import hammock.core.base_pipeline;
import hammock.core.descriptor;



namespace hammock::core {
    /// @struct ComputePipelineCreateInfo
    /// Info struct that describes compute pipeline
    export struct ComputePipelineCreateInfo {
        Device &device;
        std::vector<char> byteCode{};
        std::string entry{"computeMain"};
        std::vector<vk::DescriptorSetLayout> descriptorSetLayouts{};
        std::vector<vk::PushConstantRange> pushConstantRanges{};
    };

    /// @class ComputePipeline
    /// Class representing async compute pipeline
    export class ComputePipeline final : public BasePipeline {
    public:
        ComputePipeline(const ComputePipelineCreateInfo &config);

        ComputePipeline(const ComputePipeline &) = delete;

        ComputePipeline &operator=(const ComputePipeline &) = delete;

        ~ComputePipeline() override;

        static std::unique_ptr<ComputePipeline> create(const ComputePipelineCreateInfo &createInfo) {
            return std::make_unique<ComputePipeline>(createInfo);
        }

        void dispatch(CommandBuffer &commandBuffer, uint32_t x, uint32_t y, uint32_t z) {
            commandBuffer.getCommandBuffer().dispatch(x,y,z);
        }

        void bind(CommandBuffer &commandBuffer) override {
            commandBuffer.getCommandBuffer().bindPipeline(vk::PipelineBindPoint::eCompute, pipeline);
        }

        void bindDescriptorSet(
            CommandBuffer &commandBuffer, uint32_t firstSet, const vk::DescriptorSet *descriptorSet) override {
            commandBuffer.getCommandBuffer().bindDescriptorSets(
                vk::PipelineBindPoint::eCompute,
                pipelineLayout,
                firstSet, 1,
                descriptorSet,
                0, nullptr
            );
        }

        void pushConstants(CommandBuffer &commandBuffer, vk::ShaderStageFlags stageFlags,
                           uint32_t offset, uint32_t size, const void *pValues) override {
            commandBuffer.getCommandBuffer().pushConstants(pipelineLayout, vk::ShaderStageFlagBits::eCompute,
                               offset, size, pValues);
        }

    private:
        vk::ShaderModule shaderModule;
    };

    /// @class ComputePipelineBuilder
    /// This class can be used to easily build compute pipelines in user-friendly manner
    export class ComputePipelineBuilder {
    public:
        explicit ComputePipelineBuilder(Device &device)
            : createInfo{device} {
        }

        ComputePipelineBuilder &addShaderBytecode(const std::vector<char> &bytecode, const std::string& entry = "computeMain") {
            createInfo.byteCode = bytecode;
            createInfo.entry= entry;
            return *this;
        }

        ComputePipelineBuilder &addDescriptorSetLayout(const std::unique_ptr<DescriptorSetLayout> &descriptorSetLayout) {
            createInfo.descriptorSetLayouts.push_back(descriptorSetLayout->getDescriptorSetLayout());
            return *this;
        }

        ComputePipelineBuilder &addPushConstantRange(vk::PushConstantRange pushConstantRange) {
            createInfo.pushConstantRanges.push_back(pushConstantRange);
            return *this;
        }

        std::unique_ptr<ComputePipeline> build() {
            return std::make_unique<ComputePipeline>(createInfo);
        }


    private:
        // Default create info that will be updated based on the builder calls
        ComputePipelineCreateInfo createInfo;
    };
} // namespace Hammock
