module;
#include <vulkan/vulkan.h>
#include <string>
#include <utility>
#include <vector>
#include <memory>


export module hammock.core.graphics_pipeline;

import hammock.core.command_buffer;
import hammock.core.device;
import hammock.core.base_pipeline;
import hammock.core.descriptor;


namespace hammock::core {
    /// Helper function for constructing VkPipelineColorBlendAttachmentState struct
    export VkPipelineColorBlendAttachmentState graphicsPipelineColorBlendAttachmentState(
        VkColorComponentFlags colorWriteMask,
        VkBool32 blendEnable) {
        VkPipelineColorBlendAttachmentState pipelineColorBlendAttachmentState{};
        pipelineColorBlendAttachmentState.colorWriteMask = colorWriteMask;
        pipelineColorBlendAttachmentState.blendEnable = blendEnable;
        pipelineColorBlendAttachmentState.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        pipelineColorBlendAttachmentState.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        pipelineColorBlendAttachmentState.colorBlendOp = VK_BLEND_OP_ADD;
        pipelineColorBlendAttachmentState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        pipelineColorBlendAttachmentState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        pipelineColorBlendAttachmentState.alphaBlendOp = VK_BLEND_OP_ADD;
        pipelineColorBlendAttachmentState.colorWriteMask =
                VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT |
                VK_COLOR_COMPONENT_A_BIT;
        return pipelineColorBlendAttachmentState;
    }

    /// @struct GraphicsPipelineCreateInfo
    /// Describes the pipeline to be created
    export struct GraphicsPipelineCreateInfo {
        Device &device;

        struct ShaderModuleInfo {
            std::vector<char> byteCode{};
            std::string entry = "main";
        };

        ShaderModuleInfo vertexShader{};
        ShaderModuleInfo fragmentShader{};

        std::vector<VkDescriptorSetLayout> descriptorSetLayouts{};
        std::vector<VkPushConstantRange> pushConstantRanges{};

        // Graphics state
        VkBool32 depthTest = VK_TRUE;
        VkCompareOp depthTestCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
        VkCullModeFlags cullMode = VK_CULL_MODE_BACK_BIT;
        VkFrontFace frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        std::vector<VkPipelineColorBlendAttachmentState> blendAtaAttachmentStates{};
        std::vector<VkVertexInputBindingDescription> vertexInputBindingDescriptions{};
        std::vector<VkVertexInputAttributeDescription> vertexInputAttributeDescriptions{};

        // Dynamic state
        std::vector<VkDynamicState> dynamicStateEnables{
            VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR
        };

        // Dynamic rendering
        std::vector<VkFormat> colorAttachmentFormats{};
        VkFormat depthAttachmentFormat = VkFormat::VK_FORMAT_UNDEFINED;
        VkFormat stencilAttachmentFormat = VK_FORMAT_UNDEFINED;

        // Normal rendering for backwards compatibility or if you need to use render pass for some reason
        VkRenderPass renderPass = VK_NULL_HANDLE;
    };

    /// @class GraphicsPipeline
    /// Class representing graphics pipeline object.
    /// Use this class to bind resource
    export class GraphicsPipeline final : public BasePipeline {
        struct GraphicsPipelineConfig {
            GraphicsPipelineConfig() = default;

            GraphicsPipelineConfig(const GraphicsPipelineConfig &) = delete;

            GraphicsPipelineConfig &operator=(const GraphicsPipelineConfig &) = delete;

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

    public:
        GraphicsPipeline(GraphicsPipelineCreateInfo &createInfo);

        GraphicsPipeline(const GraphicsPipeline &) = delete;

        GraphicsPipeline &operator=(const GraphicsPipeline &) = delete;

        ~GraphicsPipeline() override;

        static std::unique_ptr<GraphicsPipeline> create(GraphicsPipelineCreateInfo createInfo);

        static void beginRendering(CommandBuffer &commandBuffer, const VkRenderingInfo *renderingInfo) {
            vkCmdBeginRendering(commandBuffer.getCommandBuffer(), renderingInfo);
        }

        static void endRendering(CommandBuffer &commandBuffer) {
            vkCmdEndRendering(commandBuffer.getCommandBuffer());
        }

        static void setViewport(CommandBuffer &commandBuffer, float x, float y, float width, float height,
                                float minDepth, float maxDepth) {
            VkViewport viewport = {x, y, width, height, minDepth, maxDepth};
            vkCmdSetViewport(commandBuffer.getCommandBuffer(), 0, 1, &viewport);
        }

        static void setScissor(CommandBuffer &commandBuffer, VkOffset2D offset, VkExtent2D extent) {
            VkRect2D rec{offset, extent};
            vkCmdSetScissor(commandBuffer.getCommandBuffer(), 0, 1, &rec);
        }

        void bind(CommandBuffer &commandBuffer) override {
            vkCmdBindPipeline(commandBuffer.getCommandBuffer(), VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
        }

        void bindDescriptorSet(
            CommandBuffer &commandBuffer, uint32_t firstSet, const VkDescriptorSet *descriptorSet) override {
            vkCmdBindDescriptorSets(
                commandBuffer.getCommandBuffer(),
                VK_PIPELINE_BIND_POINT_GRAPHICS,
                pipelineLayout,
                firstSet, 1,
                descriptorSet,
                0, nullptr
            );
        }

        void pushConstants(CommandBuffer &commandBuffer, VkShaderStageFlags stageFlags,
                           uint32_t offset, uint32_t size, const void *pValues) override {
            vkCmdPushConstants(commandBuffer.getCommandBuffer(), pipelineLayout, stageFlags, offset, size, pValues);
        }

    private:
        static void defaultRenderPipelineConfig(GraphicsPipelineConfig &configInfo);

        VkShaderModule vertShaderModule;
        VkShaderModule fragShaderModule;
    };

    /// @class GraphicsPipelineBuilder
    /// @brief This class can be used to easily create graphics pipeline in user-friendly manner
    export class GraphicsPipelineBuilder {
    public:
        explicit GraphicsPipelineBuilder(Device &device) : createInfo{device} {
        }

        GraphicsPipelineBuilder &setVertexShader(const std::vector<char> &byteCode,
                                                 const std::string &entry = "vertMain") {
            createInfo.vertexShader.byteCode = byteCode;
            createInfo.vertexShader.entry = entry;
            return *this;
        }

        GraphicsPipelineBuilder &setFragmentShader(const std::vector<char> &byteCode,
                                                   const std::string &entry = "fragMain") {
            createInfo.fragmentShader.byteCode = byteCode;
            createInfo.fragmentShader.entry = entry;
            return *this;
        }

        GraphicsPipelineBuilder &
        addDescriptorSetLayout(const std::unique_ptr<DescriptorSetLayout> &descriptorSetLayout) {
            createInfo.descriptorSetLayouts.push_back(descriptorSetLayout->getDescriptorSetLayout());
            return *this;
        }

        GraphicsPipelineBuilder &addPushConstantRange(VkPushConstantRange pushConstantRange) {
            createInfo.pushConstantRanges.push_back(pushConstantRange);
            return *this;
        }

        GraphicsPipelineBuilder &setDepthTest(VkBool32 depthTest = VK_TRUE,
                                              VkCompareOp compareOp = VK_COMPARE_OP_LESS_OR_EQUAL) {
            createInfo.depthTest = depthTest;
            createInfo.depthTestCompareOp = compareOp;
            return *this;
        }

        GraphicsPipelineBuilder &setCullMode(VkCullModeFlags cullMode = VK_CULL_MODE_BACK_BIT,
                                             VkFrontFace frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE) {
            createInfo.cullMode = cullMode;
            createInfo.frontFace = frontFace;
            return *this;
        }

        GraphicsPipelineBuilder &addBlendAttachmentState(VkColorComponentFlags colorWriteMask = 0xf,
                                                         VkBool32 blendEnable = VK_FALSE) {
            createInfo.blendAtaAttachmentStates.push_back(
                graphicsPipelineColorBlendAttachmentState(colorWriteMask, blendEnable));
            return *this;
        }

        GraphicsPipelineBuilder &setVertexInputAttributeDescriptions(
            std::vector<VkVertexInputAttributeDescription> vertexInputAttributeDescriptions) {
            createInfo.vertexInputAttributeDescriptions = std::move(vertexInputAttributeDescriptions);
            return *this;
        }

        GraphicsPipelineBuilder &addVertexInputAttributeDescription(std::uint32_t location, std::uint32_t binding,
                                                                    VkFormat format, std::uint32_t offset) {
            createInfo.vertexInputAttributeDescriptions.push_back({location, binding, format, offset});
            return *this;
        }

        GraphicsPipelineBuilder &setVertexInputBindingDescriptions(
            std::vector<VkVertexInputBindingDescription> vertexInputBindingDescriptions) {
            createInfo.vertexInputBindingDescriptions = std::move(vertexInputBindingDescriptions);
            return *this;
        }

        GraphicsPipelineBuilder &addVertexInputBindingDescription(std::uint32_t binding, std::uint32_t stride,
                                                                  VkVertexInputRate inputRate) {
            createInfo.vertexInputBindingDescriptions.push_back({binding, stride, inputRate});
            return *this;
        }

        GraphicsPipelineBuilder &addColorAttachmentFormat(VkFormat format) {
            createInfo.colorAttachmentFormats.push_back(format);
            return *this;
        }

        GraphicsPipelineBuilder &setDepthStencilAttachmentFormat(VkFormat depth = VK_FORMAT_UNDEFINED,
                                                                 VkFormat stencil = VK_FORMAT_UNDEFINED) {
            createInfo.depthAttachmentFormat = depth;
            createInfo.stencilAttachmentFormat = stencil;
            return *this;
        }

        /// @brief Set vulkan render pass.
        /// @note Setting render pass will disable dynamic rendering for this pipeline and dynamic rendering related settings will be ignored
        GraphicsPipelineBuilder &setRenderPass(VkRenderPass renderPass) {
            createInfo.renderPass = renderPass;
            return *this;
        }

        std::unique_ptr<GraphicsPipeline> build() {
            return std::make_unique<GraphicsPipeline>(createInfo);
        }

    private:
        GraphicsPipelineCreateInfo createInfo;
    };
} // namespace hammock::core
