module;
#include <vector>
#include <cstdint>
#include <stdexcept>
#include <compare>
#include <vulkan/vulkan.hpp>

export module hammock.core.base_pipeline;

import hammock.core.command_buffer;
import hammock.core.device;



namespace hammock::core {

    /// @interface BasePipeline
    /// Base interface for pipeline objects.
    /// Pupe virtual - cannot be instanced, only inherited from
    export class BasePipeline {
       public:
        virtual ~BasePipeline() = default;

        explicit BasePipeline(Device& device)
            : device(device){}

        virtual void bind(CommandBuffer& commandBuffer) = 0;

        
        virtual void bindDescriptorSet(
            CommandBuffer& commandBuffer, std::uint32_t firstSet, const vk::DescriptorSet* descriptorSet) = 0;
        virtual void pushConstants(CommandBuffer& commandBuffer, vk::ShaderStageFlags stageFlags,
            std::uint32_t offset, std::uint32_t size, const void* pValues) = 0;

        vk::PipelineLayout getPipelineLayout() const { return pipelineLayout; }

       protected:
        void createShaderModule(const std::vector<char>& code, vk::ShaderModule* shaderModule) const {
            vk::ShaderModuleCreateInfo createInfo{};
            createInfo.codeSize = code.size();
            createInfo.pCode = reinterpret_cast<const std::uint32_t*>(code.data());

            if (device.device().createShaderModule(&createInfo, nullptr, shaderModule) != vk::Result::eSuccess) {
                throw std::runtime_error("failed to create shader module");
            }
        }

        Device& device;
        vk::Pipeline pipeline;
        vk::PipelineLayout pipelineLayout;
    };
}  // namespace hammock::core