#pragma once

#include <memory>
#include <string>
#include <vector>
#include <vulkan/vulkan.hpp>

#include "descriptors.hpp"
#include "resource_manager.hpp"
#include "device.hpp"

namespace hammock::core {

    // Enum to distinguish pipeline types
    enum class PipelineType { Graphics, Compute };

    /// @struct PipelineCreateInfo
    /// Info struct that describes either a graphics or compute pipeline
    struct PipelineCreateInfo {
        PipelineType type;
        Device& device;

        // Common fields
        std::vector<vk::DescriptorSetLayout> descriptorSetLayouts{};
        std::vector<vk::PushConstantRange> pushConstantRanges{};

        // Graphics-specific fields
        struct ShaderModuleInfo {
            std::vector<char> byteCode{};
            std::string entry = "main";
        };

        ShaderModuleInfo vertexShader{};
        ShaderModuleInfo fragmentShader{};
        ShaderModuleInfo computeShader{};

        // Graphics state
        vk::Bool32 depthTest = vk::True;
        vk::CompareOp depthTestCompareOp = vk::CompareOp::eLessOrEqual;
        vk::CullModeFlags cullMode = vk::CullModeFlagBits::eBack;
        vk::FrontFace frontFace = vk::FrontFace::eCounterClockwise;
        std::vector<vk::PipelineColorBlendAttachmentState> blendAttachmentStates{};
        std::vector<vk::VertexInputBindingDescription> vertexInputBindingDescriptions{};
        std::vector<vk::VertexInputAttributeDescription> vertexInputAttributeDescriptions{};

        // Dynamic state
        std::vector<vk::DynamicState> dynamicStateEnables{
            vk::DynamicState::eViewport, vk::DynamicState::eScissor};

        // Dynamic rendering
        std::vector<vk::Format> colorAttachmentFormats{};
        vk::Format depthAttachmentFormat = vk::Format::eUndefined;
        vk::Format stencilAttachmentFormat = vk::Format::eUndefined;

        // Normal rendering for backwards compatibility
        vk::RenderPass renderPass = nullptr;
    };

    /// @class Pipeline
    /// Unified class representing either a graphics or compute pipeline.
    /// Provides a common interface for both types while maintaining type-specific functionality.
    class Pipeline final {
       public:
        explicit Pipeline(const PipelineCreateInfo& config);

        Pipeline(const Pipeline&) = delete;
        Pipeline& operator=(const Pipeline&) = delete;

        ~Pipeline();

        /// @brief Create a pipeline as a unique pointer
        static std::unique_ptr<Pipeline> create(const PipelineCreateInfo& createInfo);

        /// @brief Get the pipeline type
        PipelineType getType() const { return type_; }

        /// @brief Get pipeline
        vk::Pipeline getPipeline() { return pipeline_; }

        /// @brief Get the pipeline layout
        vk::PipelineLayout getPipelineLayout() const { return pipelineLayout_; }

       private:
        void createComputePipeline(const PipelineCreateInfo& config);
        void createGraphicsPipeline(const PipelineCreateInfo& config);
        void createShaderModule(const std::vector<char>& code, vk::ShaderModule* shaderModule) const;
        static void defaultGraphicsRenderPipelineConfig(struct GraphicsPipelineInternalConfig& configInfo);

        PipelineType type_;
        Device& device_;
        vk::Pipeline pipeline_;
        vk::PipelineLayout pipelineLayout_;

        // Graphics-specific members
        vk::ShaderModule vertShaderModule_ = nullptr;
        vk::ShaderModule fragShaderModule_ = nullptr;

        // Compute-specific members
        vk::ShaderModule computeShaderModule_ = nullptr;
    };

    /// @class PipelineBuilder
    /// Builder class that can create either graphics or compute pipelines in a user-friendly manner
    class PipelineBuilder {
       public:
        explicit PipelineBuilder(Device& device);

        // ===== Type selection =====

        /// @brief Configure this builder to create a compute pipeline
        PipelineBuilder& asComputePipeline();

        /// @brief Configure this builder to create a graphics pipeline
        PipelineBuilder& asGraphicsPipeline();

        // ===== Common builder methods =====

        /// @brief Add descriptor set layout (call for each descriptor set layout)
        PipelineBuilder& addDescriptorSetLayout(
            ResourceRef<DescriptorSetLayout> descriptorSetLayout);

        /// @brief Add push constant range (call for each range)
        PipelineBuilder& addPushConstantRange(vk::PushConstantRange pushConstantRange);

        // ===== Compute-specific builder methods =====

        /// @brief Set compute shader bytecode and entry point (compute pipelines only)
        PipelineBuilder& setComputeShader(
            const std::vector<char>& bytecode, const std::string& entry = "main");

        // ===== Graphics-specific builder methods =====

        /// @brief Set vertex shader bytecode and entry point (graphics pipelines only)
        PipelineBuilder& setVertexShader(
            const std::vector<char>& byteCode, const std::string& entry = "main");

        /// @brief Set fragment shader bytecode and entry point (graphics pipelines only)
        PipelineBuilder& setFragmentShader(
            const std::vector<char>& byteCode, const std::string& entry = "main");

        /// @brief Setup depth testing (graphics pipelines only)
        PipelineBuilder& setDepthTest(
            vk::Bool32 depthTest = vk::True, vk::CompareOp compareOp = vk::CompareOp::eLessOrEqual);

        /// @brief Setup (back/front) face culling (graphics pipelines only)
        PipelineBuilder& setCullMode(vk::CullModeFlags cullMode = vk::CullModeFlagBits::eBack,
            vk::FrontFace frontFace = vk::FrontFace::eCounterClockwise);

        /// @brief Add blend attachment state (graphics pipelines only)
        PipelineBuilder& addBlendAttachmentState(
            vk::ColorComponentFlags colorWriteMask, vk::Bool32 blendEnable);

        /// @brief Set vertex input attribute descriptions (graphics pipelines only)
        PipelineBuilder& setVertexInputAttributeDescriptions(
            std::vector<vk::VertexInputAttributeDescription> vertexInputAttributeDescriptions);

        /// @brief Add individual vertex input attribute description (graphics pipelines only)
        PipelineBuilder& addVertexInputAttributeDescription(
            std::uint32_t location, std::uint32_t binding, vk::Format format, std::uint32_t offset);

        /// @brief Set vertex input binding descriptions (graphics pipelines only)
        PipelineBuilder& setVertexInputBindingDescriptions(
            std::vector<vk::VertexInputBindingDescription> vertexInputBindingDescriptions);

        /// @brief Add individual vertex input binding description (graphics pipelines only)
        PipelineBuilder& addVertexInputBindingDescription(
            std::uint32_t binding, std::uint32_t stride, vk::VertexInputRate inputRate);

        /// @brief Add color attachment format (graphics pipelines only)
        PipelineBuilder& addColorAttachmentFormat(vk::Format format);

        /// @brief Set depth and stencil attachment formats (graphics pipelines only)
        PipelineBuilder& setDepthStencilAttachmentFormat(
            vk::Format depth = vk::Format::eUndefined, vk::Format stencil = vk::Format::eUndefined);

        /// @brief Set Vulkan render pass (graphics pipelines only)
        /// @note Setting render pass will disable dynamic rendering for this pipeline
        PipelineBuilder& setRenderPass(vk::RenderPass renderPass);

        /// @brief Build the pipeline as a unique ptr
        std::unique_ptr<Pipeline> build();

        /// @brief Build the pipeline create info
        PipelineCreateInfo buildCreateInfo();

       private:
        PipelineCreateInfo createInfo_;
    };

}  // namespace hammock::core
