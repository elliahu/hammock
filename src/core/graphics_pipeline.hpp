#pragma once
#include <compare>
#include <vulkan/vulkan.hpp>
#include <string>
#include <utility>
#include <vector>
#include <memory>

#include "command_buffer.hpp"
#include "device.hpp"
#include "base_pipeline.hpp"
#include "descriptors.hpp"


namespace hammock::core {

    /// @brief Helper function for constructing VkPipelineColorBlendAttachmentState struct
    inline vk::PipelineColorBlendAttachmentState graphicsPipelineColorBlendAttachmentState(
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
    struct GraphicsPipelineCreateInfo {
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
    class GraphicsPipeline final : public BasePipeline {
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

        /// @brief Create the graphics pipeline unique ptr
        static std::unique_ptr<GraphicsPipeline> create(GraphicsPipelineCreateInfo createInfo);

        /// @brief Begin dynamic rendering
        static void beginRendering(CommandBuffer &commandBuffer, const vk::RenderingInfo *renderingInfo);

        /// @brief End dynamic rendering
        static void endRendering(CommandBuffer &commandBuffer);

        /// @brief Set Viewport (requires dynamic state enabled for viewport)
        static void setViewport(CommandBuffer &commandBuffer, float x, float y, float width, float height,
                                float minDepth, float maxDepth);

        /// @brief Set scissor (requires dynamic state enabled for scissor)
        static void setScissor(CommandBuffer &commandBuffer, vk::Offset2D offset, vk::Extent2D extent);

        /// @brief Bind this pipeline as active
        void bind(CommandBuffer &commandBuffer) override;

        /// @brief Bind descriptor set
        void bindDescriptorSet(
            CommandBuffer &commandBuffer, uint32_t firstSet, const vk::DescriptorSet *descriptorSet) override;

        /// @brief Push constant range
        void pushConstants(CommandBuffer &commandBuffer, vk::ShaderStageFlags stageFlags,
                           uint32_t offset, uint32_t size, const void *pValues) override;

    private:
        static void defaultRenderPipelineConfig(GraphicsPipelineConfig &configInfo);

        vk::ShaderModule vertShaderModule_;
        vk::ShaderModule fragShaderModule_;
    };

    /// @class GraphicsPipelineBuilder
    /// @brief This class can be used to easily create graphics pipeline in user-friendly manner
    class GraphicsPipelineBuilder {
    public:
        explicit GraphicsPipelineBuilder(Device &device) : createInfo{device} {
        }

        /// @brief Set vertex shader bytecode
        GraphicsPipelineBuilder &setVertexShader(const std::vector<char> &byteCode,
                                                 const std::string &entry = "main");

        /// @brief Set fragment shader bytecode
        GraphicsPipelineBuilder &setFragmentShader(const std::vector<char> &byteCode,
                                                   const std::string &entry = "main");

        /// @brief Add descriptor set layout (call this for each layout)
        GraphicsPipelineBuilder &
        addDescriptorSetLayout(const std::unique_ptr<DescriptorSetLayout> &descriptorSetLayout);

        /// @brief Add push constant range
        GraphicsPipelineBuilder &addPushConstantRange(vk::PushConstantRange pushConstantRange);

        /// @brief Setup the depth testing
        GraphicsPipelineBuilder &setDepthTest(vk::Bool32 depthTest = vk::True,
                                              vk::CompareOp compareOp = vk::CompareOp::eLessOrEqual);

        /// @brief Setup (back/front) face culling
        GraphicsPipelineBuilder &setCullMode(vk::CullModeFlags cullMode = vk::CullModeFlagBits::eBack,
                                             vk::FrontFace frontFace = vk::FrontFace::eCounterClockwise);

        /// @brief Add blend attachment state
        GraphicsPipelineBuilder &addBlendAttachmentState(vk::ColorComponentFlags colorWriteMask,
                                                         vk::Bool32 blendEnable);

        /// @brief Set vertex input attribute descriptions
        GraphicsPipelineBuilder &setVertexInputAttributeDescriptions(
            std::vector<vk::VertexInputAttributeDescription> vertexInputAttributeDescriptions);

        GraphicsPipelineBuilder &addVertexInputAttributeDescription(std::uint32_t location, std::uint32_t binding,
                                                                    vk::Format format, std::uint32_t offset);

        GraphicsPipelineBuilder &setVertexInputBindingDescriptions(
            std::vector<vk::VertexInputBindingDescription> vertexInputBindingDescriptions);

        GraphicsPipelineBuilder &addVertexInputBindingDescription(std::uint32_t binding, std::uint32_t stride,
                                                                  vk::VertexInputRate inputRate);

        GraphicsPipelineBuilder &addColorAttachmentFormat(vk::Format format);

        GraphicsPipelineBuilder &setDepthStencilAttachmentFormat(vk::Format depth = vk::Format::eUndefined,
                                                                 vk::Format stencil = vk::Format::eUndefined);

        /// @brief Set vulkan render pass.
        /// @note Setting render pass will disable dynamic rendering for this pipeline and dynamic rendering related settings will be ignored
        GraphicsPipelineBuilder &setRenderPass(vk::RenderPass renderPass);

        std::unique_ptr<GraphicsPipeline> build() {
            return std::make_unique<GraphicsPipeline>(createInfo);
        }

    private:
        GraphicsPipelineCreateInfo createInfo;
    };
} // namespace hammock::core
