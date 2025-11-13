#pragma once
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "hammock/core/CommandBuffer.h"
#include "hammock/core/Device.h"
#include "hammock/core/Pipeline.h"


namespace Hammock {
    class ComputePipeline : public Pipeline {
        struct ComputePipelineConfig {
            ComputePipelineConfig() = default;
            ComputePipelineConfig(const ComputePipelineConfig&) = delete;
            ComputePipelineConfig& operator=(const ComputePipelineConfig&) = delete;

            // Descriptor set layouts used by the compute pipeline.
            std::vector<VkDescriptorSetLayout> descriptorSetLayouts;
            // Push constant ranges used by the compute pipeline.
            std::vector<VkPushConstantRange> pushConstantRanges;
        };

        struct ComputePipelineCreateInfo {
            std::string debugName;
            Device& device;
            struct ShaderModuleInfo {
                const std::vector<char>& byteCode;
                std::string entryFunc = "main";
            } computeShader;
            std::vector<VkDescriptorSetLayout> descriptorSetLayouts;
            std::vector<VkPushConstantRange> pushConstantRanges;
        };

       public:
        ComputePipeline(const ComputePipelineCreateInfo& config);

        ComputePipeline(const ComputePipeline&) = delete;
        ComputePipeline& operator=(const ComputePipeline&) = delete;

        ~ComputePipeline();

        static std::unique_ptr<ComputePipeline> create(const ComputePipelineCreateInfo& createInfo) {
            return std::make_unique<ComputePipeline>(createInfo);
        }
        void dispatch(CommandBuffer& commandBuffer, uint32_t x, uint32_t y, uint32_t z) {
            vkCmdDispatch(commandBuffer.getCommandBuffer(), x, y, z);
        }

        void bind(CommandBuffer& commandBuffer) override {
            vkCmdBindPipeline(commandBuffer.getCommandBuffer(), VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);
        }

        virtual void bindDescriptorSet(
            CommandBuffer& commandBuffer, uint32_t firstSet, const VkDescriptorSet* descriptorSet) override{}
        virtual void pushConstants(CommandBuffer& commandBuffer, VkShaderStageFlags stageFlags,
            uint32_t offset, uint32_t size, const void* pValues) override {

            }

       private:
         VkShaderModule shaderModule;
    };
}  // namespace Hammock
