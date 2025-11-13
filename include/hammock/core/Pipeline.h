#pragma once

#include <stdexcept>

#include "hammock/core/CommandBuffer.h"
#include "hammock/core/Device.h"

namespace Hammock {

    class Pipeline {
       public:
        Pipeline(Device& device)
            : device(device){}

        virtual void bind(CommandBuffer& commandBuffer) = 0;

        
        virtual void bindDescriptorSet(
            CommandBuffer& commandBuffer, uint32_t firstSet, const VkDescriptorSet* descriptorSet) = 0;
        virtual void pushConstants(CommandBuffer& commandBuffer, VkShaderStageFlags stageFlags,
            uint32_t offset, uint32_t size, const void* pValues) = 0;

        VkPipelineLayout getPipelineLayout() { return pipelineLayout; }

       protected:
        void createShaderModule(const std::vector<char>& code, VkShaderModule* shaderModule) const {
            VkShaderModuleCreateInfo createInfo{};
            createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
            createInfo.codeSize = code.size();
            createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

            if (vkCreateShaderModule(device.device(), &createInfo, nullptr, shaderModule) != VK_SUCCESS) {
                throw std::runtime_error("failed to create shader module");
            }
        }

        Device& device;
        VkPipeline pipeline = VK_NULL_HANDLE;
        VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
    };
}  // namespace Hammock