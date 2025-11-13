#pragma once
#include <memory>
#include <string>
#include <vector>

#include "hammock/core/CommandBuffer.h"
#include "hammock/core/Device.h"
#include "hammock/core/Pipeline.h"

namespace Hammock {
    class GraphicsPipeline : public Pipeline {
        struct GraphicsPipelineConfig {
            GraphicsPipelineConfig() = default;

            GraphicsPipelineConfig(const GraphicsPipelineConfig&) = delete;

            GraphicsPipelineConfig& operator=(const GraphicsPipelineConfig&) = delete;

            VkPipelineViewportStateCreateInfo viewportInfo;
            VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo;
            VkPipelineRasterizationStateCreateInfo rasterizationInfo;
            VkPipelineMultisampleStateCreateInfo multisampleInfo;
            VkPipelineColorBlendAttachmentState colorBlendAttachment;
            VkPipelineColorBlendStateCreateInfo colorBlendInfo;
            VkPipelineDepthStencilStateCreateInfo depthStencilInfo;
            std::vector<VkDynamicState> dynamicStateEnables;
            VkPipelineDynamicStateCreateInfo dynamicStateInfo;
        };

        struct GraphicsPipelineCreateInfo {
            std::string debugName;
            Device& device;

            struct ShaderModuleInfo {
                const std::vector<char>& byteCode{};
                std::string entryFunc = "main";
            };

            ShaderModuleInfo vertexShader{};
            ShaderModuleInfo fragmentShader{};

            std::vector<VkDescriptorSetLayout> descriptorSetLayouts{};
            std::vector<VkPushConstantRange> pushConstantRanges{};

            struct GraphicsStateInfo {
                VkBool32 depthTest = VK_TRUE;
                VkCompareOp depthTestCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
                VkCullModeFlags cullMode = VK_CULL_MODE_BACK_BIT;
                VkFrontFace frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
                std::vector<VkPipelineColorBlendAttachmentState> blendAtaAttachmentStates;

                struct VertexBufferBindingsInfo {
                    std::vector<VkVertexInputBindingDescription> vertexBindingDescriptions;
                    std::vector<VkVertexInputAttributeDescription> vertexAttributeDescriptions;
                } vertexBufferBindings;
            } graphicsState;

            struct DynamicStateInfo {
                std::vector<VkDynamicState> dynamicStateEnables{
                    VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
            } dynamicState;

            struct DynamicRendering {
                bool enabled = true;
                uint32_t colorAttachmentCount = 0;
                std::vector<VkFormat> colorAttachmentFormats{};
                VkFormat depthAttachmentFormat = VK_FORMAT_UNDEFINED;
                VkFormat stencilAttachmentFormat = VK_FORMAT_UNDEFINED;
            } dynamicRendering;

            VkRenderPass renderPass = VK_NULL_HANDLE;
        };

       public:
        GraphicsPipeline(GraphicsPipelineCreateInfo& createInfo);

        GraphicsPipeline(const GraphicsPipeline&) = delete;
        GraphicsPipeline& operator=(const GraphicsPipeline&) = delete;

        ~GraphicsPipeline();

        static std::unique_ptr<GraphicsPipeline> create(GraphicsPipelineCreateInfo createInfo);

        void bind(CommandBuffer& commandBuffer,
            VkPipelineBindPoint pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS) {
            vkCmdBindPipeline(commandBuffer.getCommandBuffer(), pipelineBindPoint, pipeline);
        }

        void beginRendering(CommandBuffer& commandBuffer, const VkRenderingInfo* renderingInfo) {
            vkCmdBeginRendering(commandBuffer.getCommandBuffer(), renderingInfo);
        }

        void setViewport(CommandBuffer& commandBuffer, float x, float y, float width, float height,
            float minDepth, float maxDepth) {
            VkViewport viewport = {x, y, width, height, minDepth, maxDepth};
            vkCmdSetViewport(commandBuffer.getCommandBuffer(), 0, 1, &viewport);
        }

        void setScissor(CommandBuffer& commandBuffer, VkOffset2D offset, VkExtent2D extent) {
            VkRect2D rec{offset, extent};
            vkCmdSetScissor(commandBuffer.getCommandBuffer(), 0, 1, &rec);
        }

        void bind(CommandBuffer& commandBuffer) override {
            vkCmdBindPipeline(commandBuffer.getCommandBuffer(), VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
        }

        virtual void bindDescriptorSet(
            CommandBuffer& commandBuffer, uint32_t firstSet, const VkDescriptorSet* descriptorSet) override {}
        virtual void pushConstants(CommandBuffer& commandBuffer, VkShaderStageFlags stageFlags,
            uint32_t offset, uint32_t size, const void* pValues) override {}

       private:
        static void defaultRenderPipelineConfig(GraphicsPipelineConfig& configInfo);
        VkShaderModule vertShaderModule;
        VkShaderModule fragShaderModule;
    };
}  // namespace Hammock
