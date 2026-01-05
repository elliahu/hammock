module;

#include <vector>
#include <string>
#include <memory>
#include <compare>
#include <vulkan/vulkan.hpp>

export module hammock_core:compute_pipeline;

import :device;
import :command_buffer;
import :base_pipeline;
import :descriptor;



namespace hammock::core {

    /// @struct ComputePipelineCreateInfo
    /// Info struct that describes compute pipeline
    export struct ComputePipelineCreateInfo {
        // Current device
        Device &device;
        // Bytecode of the SPIR-V compute shader
        std::vector<char> byteCode{};
        // Shader entry function
        std::string entry{"computeMain"};
        // descriptor set layouts of the pipeline
        std::vector<vk::DescriptorSetLayout> descriptorSetLayouts{};
        // push constants
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

        /// @brief Create a compute pipeline as a unique pointer
        static std::unique_ptr<ComputePipeline> create(const ComputePipelineCreateInfo &createInfo);

        /// @brief Dispatch a workgroup
        void dispatch(CommandBuffer &commandBuffer, uint32_t x, uint32_t y, uint32_t z);

        /// @brief Bind the compute pipeline
        void bind(CommandBuffer &commandBuffer) override;

        /// @brief Bind a descriptor set to the pipeline
        void bindDescriptorSet(
            CommandBuffer &commandBuffer, uint32_t firstSet, const vk::DescriptorSet *descriptorSet) override;

        /// @brief Push constant data
        void pushConstants(CommandBuffer &commandBuffer, vk::ShaderStageFlags stageFlags,
                           uint32_t offset, uint32_t size, const void *pValues) override;

    private:
        vk::ShaderModule shaderModule_;
    };

    /// @class ComputePipelineBuilder
    /// This class can be used to easily build compute pipelines in user-friendly manner
    export class ComputePipelineBuilder {
    public:
        explicit ComputePipelineBuilder(Device &device);

        /// @brief Set shader bytecode
        ComputePipelineBuilder &setShaderBytecode(const std::vector<char> &bytecode, const std::string& entry = "computeMain");

        /// @brief Add descriptor set layout (call this for each descriptor set layout)
        ComputePipelineBuilder &addDescriptorSetLayout(const std::unique_ptr<DescriptorSetLayout> &descriptorSetLayout);

        /// @brief Add push constant range (call this for each range)
        ComputePipelineBuilder &addPushConstantRange(vk::PushConstantRange pushConstantRange);

        /// @brief Build the pipeline as a unique ptr
        std::unique_ptr<ComputePipeline> build();

    private:
        // Default create info that will be updated based on the builder calls
        ComputePipelineCreateInfo createInfo_;
    };
} // namespace Hammock
