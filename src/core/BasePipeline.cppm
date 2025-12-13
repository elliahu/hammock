module;
#include <vulkan/vulkan.h>
#include <vector>
#include <cstdint>
#include <stdexcept>

export module hammock.core.base_pipeline;

import hammock.core.command_buffer;
import hammock.core.device;



namespace hammock::core {

    export class BasePipeline {
       public:
        virtual ~BasePipeline() = default;

        explicit BasePipeline(Device& device)
            : device(device){}

        virtual void bind(CommandBuffer& commandBuffer) = 0;

        
        virtual void bindDescriptorSet(
            CommandBuffer& commandBuffer, std::uint32_t firstSet, const VkDescriptorSet* descriptorSet) = 0;
        virtual void pushConstants(CommandBuffer& commandBuffer, VkShaderStageFlags stageFlags,
            std::uint32_t offset, std::uint32_t size, const void* pValues) = 0;

        VkPipelineLayout getPipelineLayout() const { return pipelineLayout; }

       protected:
        void createShaderModule(const std::vector<char>& code, VkShaderModule* shaderModule) const {
            VkShaderModuleCreateInfo createInfo{};
            createInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
            createInfo.codeSize = code.size();
            createInfo.pCode = reinterpret_cast<const std::uint32_t*>(code.data());

            if (vkCreateShaderModule(device.device(), &createInfo, nullptr, shaderModule) != VkResult::VK_SUCCESS) {
                throw std::runtime_error("failed to create shader module");
            }
        }

        Device& device;
        VkPipeline pipeline = VK_NULL_HANDLE;
        VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
    };
}  // namespace hammock::core