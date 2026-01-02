module;
#include <compare>
#include <vulkan/vulkan.hpp>
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
    export vk::PipelineColorBlendAttachmentState graphicsPipelineColorBlendAttachmentState(
        vk::ColorComponentFlags colorWriteMask,
        vk::Bool32 blendEnable) {
        vk::PipelineColorBlendAttachmentState pipelineColorBlendAttachmentState{};
        pipelineColorBlendAttachmentState.colorWriteMask = colorWriteMask;
        pipelineColorBlendAttachmentState.blendEnable = blendEnable;
        pipelineColorBlendAttachmentState.srcColorBlendFactor = vk::BlendFactor::eSrcAlpha;
        pipelineColorBlendAttachmentState.dstColorBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha;
        pipelineColorBlendAttachmentState.colorBlendOp = vk::BlendOp::eAdd;
        pipelineColorBlendAttachmentState.srcAlphaBlendFactor = vk::BlendFactor::eOne;;
        pipelineColorBlendAttachmentState.dstAlphaBlendFactor = vk::BlendFactor::eZero;
        pipelineColorBlendAttachmentState.alphaBlendOp = vk::BlendOp::eAdd;
        pipelineColorBlendAttachmentState.colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
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

        std::vector<vk::DescriptorSetLayout> descriptorSetLayouts{};
        std::vector<vk::PushConstantRange> pushConstantRanges{};

        // Graphics state
        vk::Bool32 depthTest = vk::True;
        vk::CompareOp depthTestCompareOp = vk::CompareOp::eLessOrEqual;
        vk::CullModeFlags cullMode = vk::CullModeFlagBits::eBack;
        vk::FrontFace frontFace = vk::FrontFace::eCounterClockwise;
        std::vector<vk::PipelineColorBlendAttachmentState> blendAtaAttachmentStates{};
        std::vector<vk::VertexInputBindingDescription> vertexInputBindingDescriptions{};
        std::vector<vk::VertexInputAttributeDescription> vertexInputAttributeDescriptions{};

        // Dynamic state
        std::vector<vk::DynamicState> dynamicStateEnables{
            vk::DynamicState::eViewport, vk::DynamicState::eScissor
        };

        // Dynamic rendering
        std::vector<vk::Format> colorAttachmentFormats{};
        vk::Format depthAttachmentFormat = vk::Format::eUndefined;
        vk::Format stencilAttachmentFormat = vk::Format::eUndefined;

        // Normal rendering for backwards compatibility or if you need to use render pass for some reason
        vk::RenderPass renderPass = nullptr;
    };

    /// @class GraphicsPipeline
    /// Class representing graphics pipeline object.
    /// Use this class to bind resource
    export class GraphicsPipeline final : public BasePipeline {
        struct GraphicsPipelineConfig {
            GraphicsPipelineConfig() = default;

            GraphicsPipelineConfig(const GraphicsPipelineConfig &) = delete;

            GraphicsPipelineConfig &operator=(const GraphicsPipelineConfig &) = delete;

            vk::PipelineViewportStateCreateInfo viewportInfo;
            vk::PipelineInputAssemblyStateCreateInfo inputAssemblyInfo;
            vk::PipelineRasterizationStateCreateInfo rasterizationInfo;
            vk::PipelineMultisampleStateCreateInfo multisampleInfo;
            vk::PipelineColorBlendAttachmentState colorBlendAttachment;
            vk::PipelineColorBlendStateCreateInfo colorBlendInfo;
            vk::PipelineDepthStencilStateCreateInfo depthStencilInfo;
            std::vector<vk::DynamicState> dynamicStateEnables;
            vk::PipelineDynamicStateCreateInfo dynamicStateInfo;
        };

    public:
        GraphicsPipeline(GraphicsPipelineCreateInfo &createInfo);

        GraphicsPipeline(const GraphicsPipeline &) = delete;

        GraphicsPipeline &operator=(const GraphicsPipeline &) = delete;

        ~GraphicsPipeline() override;

        static std::unique_ptr<GraphicsPipeline> create(GraphicsPipelineCreateInfo createInfo);

        static void beginRendering(CommandBuffer &commandBuffer, const vk::RenderingInfo *renderingInfo) {
            commandBuffer.getCommandBuffer().beginRendering(renderingInfo);
        }

        static void endRendering(CommandBuffer &commandBuffer) {
            commandBuffer.getCommandBuffer().endRendering();
        }

        static void setViewport(CommandBuffer &commandBuffer, float x, float y, float width, float height,
                                float minDepth, float maxDepth) {
            vk::Viewport viewport = {x, y, width, height, minDepth, maxDepth};
            commandBuffer.getCommandBuffer().setViewport(0, 1, &viewport);
        }

        static void setScissor(CommandBuffer &commandBuffer, vk::Offset2D offset, vk::Extent2D extent) {
            vk::Rect2D rec{offset, extent};
            commandBuffer.getCommandBuffer().setScissor(0, 1, &rec);
        }

        void bind(CommandBuffer &commandBuffer) override {
            commandBuffer.getCommandBuffer().bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline);
        }

        void bindDescriptorSet(
            CommandBuffer &commandBuffer, uint32_t firstSet, const vk::DescriptorSet *descriptorSet) override {
            commandBuffer.getCommandBuffer().bindDescriptorSets(
                vk::PipelineBindPoint::eGraphics,
                pipelineLayout,
                firstSet, 1,
                descriptorSet,
                0, nullptr
            );
        }

        void pushConstants(CommandBuffer &commandBuffer, vk::ShaderStageFlags stageFlags,
                           uint32_t offset, uint32_t size, const void *pValues) override {
            commandBuffer.getCommandBuffer().pushConstants(pipelineLayout, stageFlags, offset, size, pValues);
        }

    private:
        static void defaultRenderPipelineConfig(GraphicsPipelineConfig &configInfo);

        vk::ShaderModule vertShaderModule;
        vk::ShaderModule fragShaderModule;
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

        GraphicsPipelineBuilder &addPushConstantRange(vk::PushConstantRange pushConstantRange) {
            createInfo.pushConstantRanges.push_back(pushConstantRange);
            return *this;
        }

        GraphicsPipelineBuilder &setDepthTest(vk::Bool32 depthTest = vk::True,
                                              vk::CompareOp compareOp = vk::CompareOp::eLessOrEqual) {
            createInfo.depthTest = depthTest;
            createInfo.depthTestCompareOp = compareOp;
            return *this;
        }

        GraphicsPipelineBuilder &setCullMode(vk::CullModeFlags cullMode = vk::CullModeFlagBits::eBack,
                                             vk::FrontFace frontFace = vk::FrontFace::eCounterClockwise) {
            createInfo.cullMode = cullMode;
            createInfo.frontFace = frontFace;
            return *this;
        }

        GraphicsPipelineBuilder &addBlendAttachmentState(vk::ColorComponentFlags colorWriteMask,
                                                         vk::Bool32 blendEnable) {
            createInfo.blendAtaAttachmentStates.push_back(
                graphicsPipelineColorBlendAttachmentState(colorWriteMask, blendEnable));
            return *this;
        }

        GraphicsPipelineBuilder &setVertexInputAttributeDescriptions(
            std::vector<vk::VertexInputAttributeDescription> vertexInputAttributeDescriptions) {
            createInfo.vertexInputAttributeDescriptions = std::move(vertexInputAttributeDescriptions);
            return *this;
        }

        GraphicsPipelineBuilder &addVertexInputAttributeDescription(std::uint32_t location, std::uint32_t binding,
                                                                    vk::Format format, std::uint32_t offset) {
            createInfo.vertexInputAttributeDescriptions.push_back({location, binding, format, offset});
            return *this;
        }

        GraphicsPipelineBuilder &setVertexInputBindingDescriptions(
            std::vector<vk::VertexInputBindingDescription> vertexInputBindingDescriptions) {
            createInfo.vertexInputBindingDescriptions = std::move(vertexInputBindingDescriptions);
            return *this;
        }

        GraphicsPipelineBuilder &addVertexInputBindingDescription(std::uint32_t binding, std::uint32_t stride,
                                                                  vk::VertexInputRate inputRate) {
            createInfo.vertexInputBindingDescriptions.push_back({binding, stride, inputRate});
            return *this;
        }

        GraphicsPipelineBuilder &addColorAttachmentFormat(vk::Format format) {
            createInfo.colorAttachmentFormats.push_back(format);
            return *this;
        }

        GraphicsPipelineBuilder &setDepthStencilAttachmentFormat(vk::Format depth = vk::Format::eUndefined,
                                                                 vk::Format stencil = vk::Format::eUndefined) {
            createInfo.depthAttachmentFormat = depth;
            createInfo.stencilAttachmentFormat = stencil;
            return *this;
        }

        /// @brief Set vulkan render pass.
        /// @note Setting render pass will disable dynamic rendering for this pipeline and dynamic rendering related settings will be ignored
        GraphicsPipelineBuilder &setRenderPass(vk::RenderPass renderPass) {
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
